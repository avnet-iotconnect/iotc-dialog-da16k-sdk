//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotconnect_common.h"
#include "iotconnect_discovery.h"
#include "iotconnect.h"

#include "mqtt_client_sample.h"

#include "atcmd.h"

#define APP_OTA_VERSION "00.01.00"

//
// This is a standalone example that directly communicates with the mqtt_client and sends it's own telemetry information
//
// The HTTPS and MQTTS certificates (and device certificate / private key) are saved to NVRAM explicitly in the executable
//
#undef STANDALONE
//#define STANDALONE 1

static void on_connection_status(IotConnectConnectionStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            IOTC_DEBUG("IoTConnect Client Connected\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            IOTC_DEBUG("IoTConnect Client Disconnected\n");
            break;
        default:
            IOTC_DEBUG("IoTConnect Client ERROR\n");
            break;
    }

    //
    // reflect the state of the IoTConnect MQTT connection
    //
    atcmd_asynchony_event_for_icmqtt(status == IOTC_CS_MQTT_CONNECTED ? 1 : 0);
}

static void on_command(IotclEventData data) {
    int iotc_cmdack;
    platform_poll_iotconnect_use_cmdack(&iotc_cmdack);

    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        goto cleanup;
    }

    if(iotc_cmdack != 1) {
        IOTC_DEBUG("iotc_cmdack %d -- ignoring command\n", iotc_cmdack);

        //
        // If DA16X_CONF_STR_MQTT_IOTCONNECT_USE_CMDACK is not 1 a validly treated command will immediately respond false
        //
        iotconnect_command_status(type, ack_id, false, "Not implemented");
        goto cleanup_ack_id;
    }

    //
    // When DA16X_CONF_STR_MQTT_IOTCONNECT_USE_CMDACK is 1, the response to a validly treated command is at the discretion of the AT user.
    // Simply provide information.
    //
    char *command = iotcl_clone_command(data);
    if (NULL == command) {
        IOTC_ERROR("%s: command == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");

        atcmd_asynchony_event_for_iccmd(UNKNOWN_EVENT, NULL, NULL);
        goto cleanup_ack_id;
    }
    
    // send command asynchronously here?
    // replace any spaces with a comma
    for(unsigned int i = 0;i < strlen(command);i++) {
        command[i] = (command[i] == ' ') ? ',' : command[i];
    }

    IOTC_DEBUG("%s command: %s\n", __func__, command);

    atcmd_asynchony_event_for_iccmd(type, ack_id, command);
    free((void *) command);

cleanup_ack_id:
    free((void *) ack_id);
cleanup:
    iotcl_destroy_event(data);
}

static void on_ota(IotclEventData data) {
    int iotc_otaack;
    platform_poll_iotconnect_use_otaack(&iotc_otaack);

    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        goto cleanup;
    }

    if(iotc_otaack != 1) {
        IOTC_DEBUG("iotc_otaack %d -- ignoring command\n", iotc_otaack);

        //
        // If DA16X_CONF_STR_MQTT_IOTCONNECT_USE_OTAACK is not 1 a validly treated OTA message will immediately respond false
        //
        iotconnect_command_status(type, ack_id, false, "Not implemented");
        goto cleanup_ack_id;
    }

    //
    // When DA16X_CONF_STR_MQTT_IOTCONNECT_USE_OTAACK is 1, the response to a validly treated OTA message is at the discretion of the AT user.
    // Simply provide information.
    //
    const char *version = iotcl_clone_sw_version(data);
    if (NULL == version) {
        IOTC_ERROR("%s: version == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");

        atcmd_asynchony_event_for_icota(NULL, NULL, NULL);
        goto cleanup_ack_id;
    }

    const char *url = iotcl_clone_download_url(data, 0);
    if (NULL == url) {
        // FIXME TODO original code had comments that "url" couuld be null, and somehoe inside a "data" item instead does that need more implementation here?


        IOTC_ERROR("%s: url == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");

        atcmd_asynchony_event_for_icota(NULL, NULL, NULL);
        goto cleanup_version;
    }

    IOTC_DEBUG("Download version is: %s\n", version);
    IOTC_DEBUG("Download URL is: %s\n", url);

    atcmd_asynchony_event_for_icota(ack_id, version, url);

    free((void *) url);
cleanup_version:
    free((void *) version);
cleanup_ack_id:
    free((void *) ack_id);
cleanup:
    iotcl_destroy_event(data);
}

#ifdef STANDALONE
/*
 * BaltimoreCyberTrustRoot.crt.pem
 */
static const char *mqtt_root_ca_buffer0 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
    "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
    "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
    "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
    "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
    "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
    "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
    "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
    "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
    "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
    "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
    "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
    "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
    "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
    "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
    "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
    "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
    "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
    "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
    "-----END CERTIFICATE-----\n";

#define IOTCONNECT_CPID "avtds"
#define IOTCONNECT_ENV "avnetpoc"

#warning Please specify IOTCONNECT_DUID, IOTCONNECT_AUTH_TYPE, iotc_base64_symmetric_key, mqtt_device_cert0 and mqtt_device_private_key0.

#define IOTCONNECT_DUID ""
#define IOTCONNECT_AUTH_TYPE IOTC_AT_SYMMETRIC_KEY
static const char *iotc_base64_symmetric_key = ""; // note this is base64 encoded
static const char *mqtt_device_cert0 = NULL;
static const char *mqtt_device_private_key0 = NULL;

static int iotc_usleep(unsigned long usec) {
    unsigned long delay = usec/1000; // convert from microseconds to milliseconds

    // IOTC_DEBUG("iotc_usleep()\n");
    delay = delay/portTICK_PERIOD_MS + 1; // ensure delay is non-zero number of ticks
    vTaskDelay( delay ); // delay in ticks
    return 0;
}

static void publish_telemetry(void) {
    const char *str = NULL;

    char *number_names[] = { "cpu" };
    double number_values[] = { 3.123 };

    str = iotcl_serialise_telemetry_strings(0, NULL, NULL,
                                            1, number_names, number_values,
                                            0, NULL, NULL,
                                            0, NULL);
    if(str == NULL) {
        IOTC_ERROR("iotcl_serialise_telemetry_strings() failed\n");
        return;
    }

    iotconnect_sdk_send_packet(str); // underlying code will report an error
    iotcl_destroy_serialized(str);
}

int iotconnect_basic_sample_main(void)
{
    IOTC_WARN("\n\n\nRunning in STANDALONE mode\n\n\n");

#if 0
    if (access(IOTCONNECT_SERVER_CERT, F_OK) != 0) {
        IOTC_ERROR("Unable to access IOTCONNECT_SERVER_CERT. "
               "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
               IOTCONNECT_SERVER_CERT);
    }

    if (IOTCONNECT_AUTH_TYPE == IOTC_AT_X509) {
        if (access(IOTCONNECT_IDENTITY_CERT, F_OK) != 0 ||
            access(IOTCONNECT_IDENTITY_KEY, F_OK) != 0
                ) {
            IOTC_ERROR("Unable to access device identity private key and certificate. "
                   "Please change directory so that %s can be accessed from the application or update IOTCONNECT_CERT_PATH\n",
                   IOTCONNECT_SERVER_CERT);
        }
    }
#endif

    IotConnectClientConfig *config = iotconnect_sdk_init_and_set_config(IOTCONNECT_ENV, IOTCONNECT_CPID, IOTCONNECT_DUID, IOTCONNECT_AUTH_TYPE, (char *) iotc_base64_symmetric_key);
    if(config == NULL) {
        IOTC_ERROR("iotconnect_sdk_init_and_get_config() failed\n");
        return -1;
    }

    IOTC_DEBUG("IOTC_ENV = %s\n", config->env);
    IOTC_DEBUG("IOTC_CPID = %s\n", config->cpid);
    IOTC_DEBUG("IOTC_DUID = %s\n", config->duid);
    IOTC_DEBUG("IOTC_AUTH_TYPE = %d\n", config->auth_info.type);
    IOTC_DEBUG("IOTC_AUTH_SYMMETRIC_KEY = %s\n", config->auth_info.data.symmetric_key ? config->auth_info.data.symmetric_key : "(null)");

    config->auth_info.trust_store = mqtt_root_ca_buffer0;
    config->status_cb = on_connection_status;
    config->ota_cb = on_ota;
    config->cmd_cb = on_command;

    if (config->auth_info.type == IOTC_AT_X509) {
        // device certificate and private key are assumed to be written to NVRAM
        // but leave in here, because of tests in the library, etc.
        // there's no dynamic memory involved.
        config->auth_info.data.cert_info.device_cert = mqtt_device_cert0;
        config->auth_info.data.cert_info.device_key = mqtt_device_private_key0;
    } else if (config->auth_info.type == IOTC_AT_TPM) {
        IOTC_WARN("IOTC_AT_TPM is not implemented\n");
    } else if (config->auth_info.type == IOTC_AT_SYMMETRIC_KEY){
        // symmetric key will be read from NVRAM
        // but leave in here, because of tests in the library, etc.
        if(iotc_base64_symmetric_key == NULL)
        {
            config->auth_info.data.symmetric_key = NULL;
        }
        else
        {
            config->auth_info.data.symmetric_key = (const char *) iotcl_strdup(iotc_base64_symmetric_key);
            if(config->auth_info.data.symmetric_key == NULL)
            {
                IOTC_ERROR("iotcl_strdup() failed\n");
                return -1;
            }
        }
    } else if (config->auth_info.type == IOTC_AT_TOKEN) {
        // token type does not need any secret or info
    } else {
        // none of the above
        IOTC_ERROR("IOTCONNECT_AUTH_TYPE is invalid\n");
        return -1;
    }

    // MQTT Root CA and/or device certificate and private key will be written to NVRAM
    int status = iotconnect_sdk_preinit_certs();
    if (0 != status) {
        IOTC_ERROR("iotconnect_sdk_preinit_certs() exited with error code %d\n", status);
        return status;
    }

    // run a dozen connect/send/disconnect cycles with each cycle being about a minute
    for (int j = 0; j < 3; j++) {
        IOTC_DEBUG("iotconnect_sdk_init() %d ->\n", j);

        //
        // Run discovery/sync
        // Startup mqtt_client with new MQTT_XXXX values
        //
        // Only runs discovery/sync once -- multiple calls will re-use previous values
        //
        int ret = iotconnect_sdk_init();
        if (0 != ret) {
            IOTC_ERROR("iotconnect_sdk_init() exited with error code %d\n", ret);
            return ret;
        }
        IOTC_DEBUG("<- iotconnect_sdk_init() %d\n", j);

        // send 10 messages
        for (int i = 0; i < 3; i++) {
            if  (!iotconnect_sdk_is_connected()) {
                IOTC_ERROR("iotconnect_sdk_is_connected() returned false\n");
                break;
            }

            publish_telemetry();
            // repeat approximately evey ~5 seconds
            for (int k = 0; k < 500; k++) {
                iotconnect_sdk_receive();
                iotc_usleep(10000); // 10ms
            }
        }

        iotconnect_sdk_disconnect();
    }

    // there is a slight "leak" here that discovery_response / sync_response / config may be holding onto dynamic memory

    IOTC_DEBUG("exiting basic_sample()\n");
    return 0;
}
#else

//
// a wrapper to send +NWICCONFIGSTART / +NWICCONFIGEND messages
//
IotConnectClientConfig *read_environment(void)
{
    atcmd_asynchony_event_for_icconfig_start();
    IOTC_DEBUG("calling iotconnect_sdk_init_and_get_config()\n" );

    //
    // Load up config with new IOTC_XXXX values
    //
    // Clear any previous discovery/sync responses
    //
    // note iotconnect_sdk_init_and_get_config() here should free any dynamic memory that discovery_response / sync_response / config may be holding onto
    //
    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    if(config == NULL) {
        IOTC_ERROR("iotconnect_sdk_init_and_get_config() failed\n");
        goto exit;
    }

    IOTC_DEBUG("IOTC_ENV = %s\n", config->env);
    IOTC_DEBUG("IOTC_CPID = %s\n", config->cpid);
    IOTC_DEBUG("IOTC_DUID = %s\n", config->duid);
    IOTC_DEBUG("IOTC_AUTH_TYPE = %d\n", config->auth_info.type);
    IOTC_DEBUG("IOTC_AUTH_SYMMETRIC_KEY = %s\n", config->auth_info.data.symmetric_key ? config->auth_info.data.symmetric_key : "(null)");

    /*
     * TODO FIXME CHECK Haven't set up any callbacks here -- not sure how "AT command"-style reflects MQTT state changes -- ugghh!
     */
    config->status_cb = on_connection_status;
    config->ota_cb = on_ota;
    config->cmd_cb = on_command;

exit:
    atcmd_asynchony_event_for_icconfig_end(config != NULL);
    return config;
}

//
// a wrapper to send +NWICSYNCSTART / +NWICSYNCEND messages
//
int discover_and_sync()
{
    //
    // Run discovery/sync
    // Startup mqtt_client with new MQTT_XXXX values
    //
    // Because discovery/sync responses are cleared below -- the discovery/sync will also run (with up to date values).
    //
    atcmd_asynchony_event_for_icsync_start();
    int ret = iotconnect_sdk_init();
    atcmd_asynchony_event_for_icsync_end(ret == 0);

    if (0 != ret) {
        IOTC_ERROR("iotconnect_sdk_init() exited with error code %d\n", ret);
    }

    return ret;
}

//
// This is a cutdown version that monitors the DA16X_CONF_STR_IOTCONNECT_SYNC environment variable
// which is set by AT+NWICSYNC command
// 
// When the value changes to 1 it will perform IoTConnect discovery and write NVRAM values, the MQTT client
// will be restarted and the new values from IoTConnect discovery should be used for MQTT.
//
// So sequence of events is:
// - set IoTConnect NVRAM values
// - set DA16X_CONF_STR_IOTCONNECT_SYNC to 1
// - ensure MQTT client is running
//
//
// This relies on the standard certificate mechanisms built into the DA16200
// 
// AT commands to set the HTTP / MQQT certificates [and, if required, to set the device certificate / private key] need to be sent.
//
int iotconnect_basic_sample_main(void)
{
    int iotc_mode = 0;
        
    IOTC_WARN("\n\n\nRunning in AT command mode\n\n\n");

    /*
     * Explicity *NOT* setting any certificates for "AT command" version
     *
     * Certificates can be set see: Table 16, Page 50, "UM-WI-003 DA16200 DA16600 Host Interface and AT Command User Manual" 
     */

    IotConnectClientConfig *config = read_environment();
    if(config == NULL) {
        IOTC_ERROR("read_environment() failed\n");
        // FIXME TODO CHECK not sure what to do here?
        iotc_mode = 2; // an error state 
    }

    while(1)
    {
        int new_iotc_mode = 0;

        //
        // Horrid polling mechanism -- maybe need to have some sort of waitQueue in the future
        //
        vTaskDelay(1000/portTICK_PERIOD_MS);

        platform_poll_iotconnect_sync_mode(&new_iotc_mode);
        if(new_iotc_mode == iotc_mode)
        {
            continue;
        }

        IOTC_DEBUG("DA16X_CONF_STR_MQTT_IOTCONNECT_MODE is %d (was %d)\n", new_iotc_mode, iotc_mode);

        if(new_iotc_mode == 1)
        {
            if(discover_and_sync() != 0)
            {
                // FIXME TODO CHECK not sure what to do here?

                iotc_mode = 3; // an error state 
                continue; 
            }
            
            iotc_mode = 1;
            continue;
        }

        if(new_iotc_mode == 0)
        {
            config = read_environment();
            if(config == NULL) {
                IOTC_ERROR("read_environment() failed\n");

                iotc_mode = 2; // an error state 
                continue; 
            }

            iotc_mode = 0;
            continue;
        }
    }

    // there is a slight "leak" here that discovery_response / sync_response / config may be holding onto dynamic memory
    
    IOTC_DEBUG("exiting basic_sample()\n" );
    return 0;
}
#endif
