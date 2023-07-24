//
// Copyright: Avnet, Softweb Inc. 2021
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//

#define IOTC_USE_ALGORITHMS 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef IOTC_USE_ALGORITHMS
#include "iotc_algorithms.h"
#endif
#include "iotconnect_common.h"
#include "iotconnect_discovery.h"
#include "iotconnect.h"
#include "iotc_http_request.h"
#include "iotc_device_client.h"

#define HTTP_DISCOVERY_URL_FORMAT "https://%s/api/sdk/cpid/%s/lang/M_C/ver/2.0/env/%s"
#define HTTP_SYNC_URL_FORMAT "https://%s%ssync?"

static IotConnectClientConfig config = {0};

// cached discovery/sync response:
static IotclDiscoveryResponse *discovery_response = NULL;
static IotclSyncResponse *sync_response = NULL;

// cached TPM registration ID if TPM auth is used
// once (if) we support a dispose method, we should free this value
static char *tpm_registration_id = NULL;

static void on_mqtt_c2d_message(const unsigned char *message, size_t message_len) {
    char *str = malloc(message_len + 1);
    if(!str) {
        IOTC_ERROR("Unable to allocate memory\n");
        return;
    }

    memcpy(str, message, message_len);
    str[message_len] = 0;
    IOTC_DEBUG("event>>> %s\n", str);
    if (!iotcl_process_event(str)) {
        IOTC_ERROR("Error encountered while processing %s\n", str);
    }
    free(str);
}

static void set_device_client_config(IotConnectDeviceClientConfig *pc) {
    memset(pc, 0, sizeof(*pc));

    pc->sr = sync_response;
    pc->qos = config.qos;
    pc->status_cb = config.status_cb;
    pc->c2d_msg_cb = on_mqtt_c2d_message;
    pc->auth = &config.auth_info;
}

static void dump_response(const char *message, IotConnectHttpResponse *response) {
    if (response->data) {
        IOTC_DEBUG("%s", message);
        IOTC_DEBUG(" Response was:\n----\n%s\n----\n", response->data);
    } else {
        IOTC_WARN("%s", message);
        IOTC_WARN(" Response was empty\n");
    }
}

static void report_sync_error(const IotclSyncResponse *response, const char *sync_response_str) {
    if (NULL == response) {
        IOTC_ERROR("Failed to obtain sync response?\n");
        return;
    }
    switch (response->ds) {
        case IOTCL_SR_DEVICE_NOT_REGISTERED:
            IOTC_ERROR("IOTC_SyncResponse error: Not registered\n");
            break;
        case IOTCL_SR_AUTO_REGISTER:
            IOTC_ERROR("IOTC_SyncResponse error: Auto Register\n");
            break;
        case IOTCL_SR_DEVICE_NOT_FOUND:
            IOTC_ERROR("IOTC_SyncResponse error: Device not found\n");
            break;
        case IOTCL_SR_DEVICE_INACTIVE:
            IOTC_ERROR("IOTC_SyncResponse error: Device inactive\n");
            break;
        case IOTCL_SR_DEVICE_MOVED:
            IOTC_ERROR("IOTC_SyncResponse error: Device moved\n");
            break;
        case IOTCL_SR_CPID_NOT_FOUND:
            IOTC_ERROR("IOTC_SyncResponse error: CPID not found\n");
            break;
        case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
            IOTC_ERROR("IOTC_SyncResponse error: Unknown device status error from server\n");
            break;
        case IOTCL_SR_ALLOCATION_ERROR:
            IOTC_ERROR("IOTC_SyncResponse internal error: Allocation Error\n");
            break;
        case IOTCL_SR_PARSING_ERROR:
            IOTC_ERROR("IOTC_SyncResponse internal error: Parsing error. Please check parameters passed to the request.\n");
            break;
        default:
            IOTC_ERROR("WARN: report_sync_error called, but no error returned?\n");
            break;
    }
    IOTC_ERROR("Raw server response was:\n--------------\n%s\n--------------\n", sync_response_str);
}

static IotclDiscoveryResponse *run_http_discovery(const char *cpid, const char *env) {
    IotclDiscoveryResponse *ret = NULL;
    char *url_buff = malloc(sizeof(HTTP_DISCOVERY_URL_FORMAT) +
                            sizeof(IOTCONNECT_DISCOVERY_HOSTNAME) +
                            strlen(cpid) +
                            strlen(env) - 4 /* %s x 2 */
    );

    if(!url_buff) {
        IOTC_ERROR("Unable to allocate memory");
        return NULL;
    }

    sprintf(url_buff, HTTP_DISCOVERY_URL_FORMAT,
            IOTCONNECT_DISCOVERY_HOSTNAME, cpid, env
    );

    IotConnectHttpResponse response;
    iotconnect_https_request(&response,
                             url_buff,
                             NULL
    );

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response,\n", &response);
        goto cleanup;
    }
    const char *json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.\n", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.\n", &response);
    } else {
        dump_response("discovery reponse ok before parsing\n", &response);
    }

    ret = iotcl_discovery_parse_discovery_response(json_start);
    if (!ret) {
        IOTC_ERROR("Error: Unable to get discovery response for environment \"%s\". Please check the environment name in the key vault.\n", env);
    }

    dump_response("discovery reponse parsing is ok\n", &response);

    cleanup:
    free(url_buff);
    iotconnect_free_https_response(&response);
    // fall through
    return ret;
}

static IotclSyncResponse *run_http_sync(const char *cpid, const char *uniqueid) {
    IotclSyncResponse *ret = NULL;
    char *url_buff = malloc(sizeof(HTTP_SYNC_URL_FORMAT) +
                            strlen(discovery_response->host) +
                            strlen(discovery_response->path)
    );
    char *post_data = malloc(IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN + 1);

    if (!url_buff || !post_data) {
        IOTC_ERROR("run_http_sync: Out of memory!");
        free(url_buff); // one of them could have succeeded
        free(post_data);
        return NULL;
    }

    sprintf(url_buff, HTTP_SYNC_URL_FORMAT,
            discovery_response->host,
            discovery_response->path
    );
    snprintf(post_data,
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_MAX_LEN, /*total length should not exceed MTU size*/
             IOTCONNECT_DISCOVERY_PROTOCOL_POST_DATA_TEMPLATE,
             cpid,
             uniqueid
    );

    IotConnectHttpResponse response;
    iotconnect_https_request(&response,
                             url_buff,
                             post_data
    );

    free(url_buff);
    free(post_data);

    if (NULL == response.data) {
        dump_response("Unable to parse HTTP response.\n", &response);
        goto cleanup;
    }
    const char *json_start = strstr(response.data, "{");
    if (NULL == json_start) {
        dump_response("No json response from server.\n", &response);
        goto cleanup;
    }
    if (json_start != response.data) {
        dump_response("WARN: Expected JSON to start immediately in the returned data.\n", &response);
    }
    else {
        dump_response("sync reponse parsing is ok\n", &response);
    }

    IOTC_DEBUG("Start to parse the sync response\n");
    ret = iotcl_discovery_parse_sync_response(json_start);
    IOTC_DEBUG("Finished parsing the sync response\n");

    if (!ret || ret->ds != IOTCL_SR_OK) {
        if (config.auth_info.type == IOTC_AT_TPM && ret && ret->ds == IOTCL_SR_DEVICE_NOT_REGISTERED) {
            // malloc below will be freed when we iotcl_discovery_free_sync_response
            ret->broker.client_id = malloc(strlen(uniqueid) + 1 /* - */ + strlen(cpid) + 1);
            if(!ret->broker.client_id) {
                dump_response("Unable to allocate memory,\n", &response);
                goto cleanup;
            }

            sprintf(ret->broker.client_id, "%s-%s", cpid, uniqueid);
            IOTC_WARN("TPM Device is not yet enrolled. Enrolling...\n");
        } else {
            report_sync_error(ret, response.data);
            iotcl_discovery_free_sync_response(ret);
            ret = NULL;
        }
    }

    // fall through
    cleanup:
    iotconnect_free_https_response(&response);

    return ret;
}

void iotconnect_sdk_disconnect(void) {
    IOTC_DEBUG("Disconnecting...\n");
    if (0 == iotc_device_client_disconnect()) {
        IOTC_DEBUG("Disconnected.\n");
    }
}

bool iotconnect_sdk_is_connected(void) {
    return iotc_device_client_is_connected();
}

static void iotconnect_sdk_reset_config(void) {
    IOTC_DEBUG("%s\n", __func__);

    if(iotconnect_sdk_is_connected() == true) {
        iotconnect_sdk_disconnect();
    }

    tpm_registration_id = NULL;

    iotcl_discovery_free_sync_response(sync_response);
    sync_response = NULL;

    iotcl_discovery_free_discovery_response(discovery_response);
    discovery_response = NULL;

    free(config.cpid);
    free(config.duid);
    free(config.env);
    free((void *) config.auth_info.data.symmetric_key);
    memset(&config, 0, sizeof(config));
}

//
// Any allocated storage is free'd in iotconnect_sdk_reset_config() to avoid a leak
//
// Although there is no explicit deinit() routine
//
IotConnectClientConfig *iotconnect_sdk_init_and_get_config(void) {
    iotconnect_sdk_reset_config();

    if(iotc_platform_acquire_config_values(&config) != 0)
    {
        return NULL;
    }

    return &config;
}

//
// Any allocated storage is free'd in iotconnect_sdk_reset_config() to avoid a leak
//
// Although there is no explicit deinit() routine
//
IotConnectClientConfig *iotconnect_sdk_init_and_set_config(char *env, char *cpid, char *duid, int authentication_type, char *symmetric_key) {
    iotconnect_sdk_reset_config();

    config.env = iotcl_strdup(env);
    if(config.env == NULL)
    {
        IOTC_ERROR("iotcl_strdup() failed\n");
        goto cleanup;
    }

    config.cpid = iotcl_strdup(cpid);
    if(config.cpid == NULL)
    {
        IOTC_ERROR("iotcl_strdup() failed\n");
        goto cleanup;
    }

    config.duid = iotcl_strdup(duid);
    if(config.duid == NULL)
    {
        IOTC_ERROR("iotcl_strdup() failed\n");
        goto cleanup;
    }

    config.auth_info.type = authentication_type;
    if(config.auth_info.type == IOTC_AT_SYMMETRIC_KEY)
    {
        config.auth_info.data.symmetric_key = (const char *) iotcl_strdup(symmetric_key);
        if(config.auth_info.data.symmetric_key == NULL)
        {
            IOTC_ERROR("iotcl_strdup() failed\n");
            goto cleanup;
        }
    }

    return &config;

cleanup:
    iotconnect_sdk_reset_config();
    return NULL;
}

int iotconnect_sdk_preinit_certs(void) {
    IotConnectDeviceClientConfig pc;
    set_device_client_config(&pc);

    if (iotc_device_client_preinit_certs(&pc)) {
        IOTC_ERROR("iotc_device_client_preinit_certs() failed\n");
        return -1;
    }

    return 0;
}

int iotconnect_sdk_init(void) {
    IOTC_DEBUG("%s\n", __func__);

    if(iotconnect_sdk_setup_mqtt_client() != 0)
    {
        IOTC_ERROR("iotc_device_client_preinit_certs() failed\n");
        return -1;
    }

    return iotconnect_sdk_run_mqtt_client();
}

static void on_message_intercept(IotclEventData data, IotConnectEventType type) {
    switch (type) {
        case ON_FORCE_SYNC:
            iotconnect_sdk_disconnect();
            iotcl_discovery_free_discovery_response(discovery_response);
            discovery_response = NULL;
            iotcl_discovery_free_sync_response(sync_response);
            sync_response = NULL;
            discovery_response = run_http_discovery(config.cpid, config.env);
            if (NULL == discovery_response) {
                IOTC_ERROR("Unable to run HTTP discovery on ON_FORCE_SYNC\n");
                return;
            }
            sync_response = run_http_sync(config.cpid, config.duid);
            if (NULL == sync_response) {
                IOTC_ERROR("Unable to run HTTP sync on ON_FORCE_SYNC\n");
                return;
            }
            IOTC_DEBUG("Got ON_FORCE_SYNC. Disconnecting.\n");
            iotconnect_sdk_disconnect(); // client will get notification that we disconnected and will reinit
            break;

        case ON_CLOSE:
            IOTC_DEBUG("Got ON_CLOSE. Closing the mqtt connection. Device restart is required.\n");
            iotconnect_sdk_disconnect();
            break;

        default:
            break; // not handling nay other messages
    }

    if (NULL != config.msg_cb) {
        config.msg_cb(data, type);
    }
}

int iotconnect_sdk_send_packet(const char *data) {
    IOTC_DEBUG("Sending: %s\n", data);
    return iotc_device_client_send_message(data);
}

void iotconnect_sdk_receive(void) {
    iotc_device_client_receive();
}

///////////////////////////////////////////////////////////////////////////////////
// this the Initialization os IoTConnect SDK
int iotconnect_sdk_setup_mqtt_client(void) {
    IOTC_DEBUG("%s\n", __func__);

    if (config.auth_info.type == IOTC_AT_TPM) {
        if (!config.duid || strlen(config.duid) == 0) {
            if (!tpm_registration_id) {
                tpm_registration_id = iotc_device_client_get_tpm_registration_id();
            }
            config.duid = tpm_registration_id;
        }
    }

    if (!discovery_response) {
        discovery_response = run_http_discovery(config.cpid, config.env);
        if (NULL == discovery_response) {
            // get_base_url will print the error
            return -1;
        }
        IOTC_DEBUG("Discovery response parsing successful.\n");
    } else {
        IOTC_WARN("Reusing previous discovery_response\n");
    }

    if (!sync_response) {
        sync_response = run_http_sync(config.cpid, config.duid);
        if (NULL == sync_response) {
            // Sync_call will print the error
            return -2;
        }
        IOTC_DEBUG("Sync response parsing successful.\n");
    } else {
        IOTC_WARN("Reusing previous sync_response\n");
    }

    if (!config.env || !config.cpid || !config.duid) {
        IOTC_ERROR("Error: Device configuration is invalid. Configuration values for env, cpid and duid are required.\n");
        return -1;
    }

    // We want to print only first 4 characters of cpid
    char cpid_buff[5];
    strncpy(cpid_buff, config.cpid, 4);
    cpid_buff[4] = 0;
    IOTC_DEBUG("CPID: %s***\n", cpid_buff);
    IOTC_DEBUG("ENV:  %s\n", config.env);


    if (config.auth_info.type != IOTC_AT_TOKEN &&
        config.auth_info.type != IOTC_AT_X509 &&
        config.auth_info.type != IOTC_AT_TPM &&
        config.auth_info.type != IOTC_AT_SYMMETRIC_KEY
        ) {
        IOTC_ERROR("Error: Unsupported authentication type!\n");
        return -1;
    }

    if (config.auth_info.type == IOTC_AT_SYMMETRIC_KEY) {
        if (config.auth_info.data.symmetric_key && strlen(config.auth_info.data.symmetric_key) > 0) {
#ifdef IOTC_USE_ALGORITHMS
            // for paho and mosquitto we need to pass the generated sas token
            char *sas_token = gen_sas_token(sync_response->broker.host,
                                                  config.cpid,
                                                  config.duid,
                                                  config.auth_info.data.symmetric_key,
                                                  60
            );
	
	    if(sas_token == NULL) {
                IOTC_ERROR("gen_sas_token returns NULL\n");
	    }
	
	    if(sync_response->broker.pass) {
                free (sync_response->broker.pass);
	    }
            // a bit of a hack - the token will be freed when freeing the sync response
            // paho will use the SAS token as the broker password
            sync_response->broker.pass = sas_token;
#endif
        } else {
            IOTC_ERROR("Error: Configuration symmetric key is missing.\n");
            return -1;
        }
    }

    if (config.auth_info.type == IOTC_AT_TOKEN) {
        if (!(sync_response->broker.pass || strlen(config.auth_info.data.symmetric_key) == 0)) {
            IOTC_ERROR("Error: Unable to obtainm .\n");
            return -1;
        }
    }

    IotConnectDeviceClientConfig pc;
    set_device_client_config(&pc);
    int ret = iotc_device_client_setup(&pc);
    if (ret) {
        IOTC_ERROR("Error: Unable to setup MQTT client.\n");
        return ret;
    }

    // initialize the IoTConnect Lib - needed for telemetry messages
    IotclConfig lib_config;
    memset(&lib_config, 0, sizeof(lib_config));

    lib_config.device.env = config.env;
    lib_config.device.cpid = config.cpid;
    lib_config.device.duid = config.duid;

    lib_config.event_functions.ota_cb = config.ota_cb;
    lib_config.event_functions.cmd_cb = config.cmd_cb;
    lib_config.event_functions.msg_cb = on_message_intercept;

    lib_config.telemetry.dtg = sync_response->dtg;

    if (!iotcl_init(&lib_config)) {
        IOTC_ERROR("Error: Failed to initialize the IoTConnect Lib\n");
        return -1;
    }

    return 0;
}

int iotconnect_sdk_run_mqtt_client(void) {
    int ret = 0;

    IotConnectDeviceClientConfig pc;
    set_device_client_config(&pc);
    ret = iotc_device_client_run(&pc);
    if (ret) {
        IOTC_ERROR("Failed to connect!\n");
        return ret;
    }

    // Workaround: upon first time TPM registration, the information returned from sync will be partial,
    // so update dtg with new sync call
    if (config.auth_info.type == IOTC_AT_TPM && sync_response->ds == IOTCL_SR_DEVICE_NOT_REGISTERED) {
        iotcl_discovery_free_sync_response(sync_response);
        sync_response = run_http_sync(config.cpid, config.duid);
        if (NULL == sync_response) {
            // Sync_call will print the error
            return -2;
        }
        IOTC_DEBUG("Secondary Sync response parsing successful. DTG is: %s.\n", sync_response->dtg);
    }

    return ret;
}
