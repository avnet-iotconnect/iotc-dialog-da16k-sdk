//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotconnect_common.h"
#include "iotconnect.h"

#include "mqtt_client_sample.h"

//
// This is a standalone example that directly communicates with the mqtt_client and sends it's own telemetry information
//
// The HTTPS and MQTTS certificates (and device certificate / private key) are saved to NVRAM explicitly in the executable
//
#define STANDALONE 1

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

#define APP_VERSION "00.01.00"

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
}

static void command_status(IotclEventData data, bool status, const char *command_name, const char *message) {
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, status, message);
    IOTC_DEBUG("command: %s status=%s: %s\n", command_name, status ? "OK" : "Failed", message);
    IOTC_DEBUG("Sent CMD ack: %s\n", ack);
    iotconnect_sdk_send_packet(ack);
    free((void *) ack);
}

static void on_command(IotclEventData data) {
    char *command = iotcl_clone_command(data);
    if (NULL != command) {
        command_status(data, false, command, "Not implemented");
        free((void *) command);
    } else {
        command_status(data, false, "?", "Internal error");
    }
}

static bool is_app_version_same_as_ota(const char *version) {
    return strcmp(APP_VERSION, version) == 0;
}

static bool app_needs_ota_update(const char *version) {
    return strcmp(APP_VERSION, version) < 0;
}

static void on_ota(IotclEventData data) {
    const char *message = NULL;
    char *url = iotcl_clone_download_url(data, 0);
    bool success = false;
    if (NULL != url) {
        IOTC_DEBUG("Download URL is: %s\n", url);
        const char *version = iotcl_clone_sw_version(data);
        if (is_app_version_same_as_ota(version)) {
            IOTC_DEBUG("OTA request for same version %s. Sending success\n", version);
            success = true;
            message = "Version is matching";
        } else if (app_needs_ota_update(version)) {
            IOTC_DEBUG("OTA update is required for version %s.\n", version);
            success = false;
            message = "Not implemented";
        } else {
            IOTC_DEBUG("Device firmware version %s is newer than OTA version %s. Sending failure\n", APP_VERSION,
                   version);
            // Not sure what to do here. The app version is better than OTA version.
            // Probably a development version, so return failure?
            // The user should decide here.
            success = false;
            message = "Device firmware version is newer";
        }

        free((void *) url);
        free((void *) version);
    } else {
        // compatibility with older events
        // This app does not support FOTA with older back ends, but the user can add the functionality
        const char *command = iotcl_clone_command(data);
        if (NULL != command) {
            // URL will be inside the command
            IOTC_DEBUG("Command is: %s\n", command);
            message = "Old back end URLS are not supported by the app";
            free((void *) command);
        }
    }
    const char *ack = iotcl_create_ack_string_and_destroy_event(data, success, message);
    if (NULL != ack) {
        IOTC_DEBUG("Sent OTA ack: %s\n", ack);
        iotconnect_sdk_send_packet(ack);
        free((void *) ack);
    }
}


static void publish_telemetry() {
    IotclMessageHandle msg = NULL;
    const char *str = NULL;
    const char *timestamp = NULL;

    msg = iotcl_telemetry_create();
    if(msg == NULL) {
        IOTC_ERROR("iotcl_telemetry_create() failed\n");
        goto cleanup;
    }

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    timestamp = iotcl_iso_timestamp_now();
    if(timestamp == NULL) {
        IOTC_ERROR("iotcl_iso_timestamp_now() failed\n");
        goto cleanup;
    }

    if(iotcl_telemetry_add_with_iso_time(msg, timestamp) == false) {
        IOTC_ERROR("iotcl_telemetry_add_with_iso_time() failed\n");
        goto cleanup;
    }

    if(iotcl_telemetry_set_string(msg, "version", APP_VERSION) == false) {
        IOTC_ERROR("iotcl_telemetry_set_string() failed\n");
        goto cleanup;
    }

    // test floating point numbers
    if(iotcl_telemetry_set_number(msg, "cpu", 3.123) == false) {
        IOTC_ERROR("iotcl_telemetry_set_number() failed\n");
        goto cleanup;
    }

    str = iotcl_create_serialized_string(msg, false);
    if(str == NULL) {
        IOTC_ERROR("iotcl_create_serialized_string() failed\n");
        goto cleanup;
    }

    // partial cleanup
    iotcl_telemetry_destroy(msg);
    msg = NULL;

    IOTC_DEBUG("Sending: %s\n", str);
    iotconnect_sdk_send_packet(str); // underlying code will report an error
cleanup:
    if(msg) {
        iotcl_telemetry_destroy(msg);
    }

    if(str) {
        iotcl_destroy_serialized(str);
    }
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

    IotConnectClientConfig *config = iotconnect_sdk_init_and_get_config();
    if(config == NULL) {
        IOTC_ERROR("iotconnect_sdk_init_and_get_config() failed\n");
        return -1;
    }

    config->auth_info.trust_store = mqtt_root_ca_buffer0;

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

    config->status_cb = on_connection_status;
    config->ota_cb = on_ota;
    config->cmd_cb = on_command;

    // run a dozen connect/send/disconnect cycles with each cycle being about a minute
    for (int j = 0; j < 10; j++) {
        int ret = iotconnect_sdk_init();
        if (0 != ret) {
            IOTC_ERROR("IoTConnect exited with error code %d\n", ret);
            return ret;
        }
        IOTC_DEBUG("iotconnect_sdk_init() %d\n", j);

        // send 10 messages
        for (int i = 0; i < 10; i++) {
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

    iotconnect_sdk_deinit();

    IOTC_DEBUG("exiting basic_sample()\n" );
    return 0;
}
#else
//
// This is a cutdown version that monitors the DA16X_CONF_STR_MQTT_CLIENT_IOTCONNECT_MODE environment variable
// which is set by AT+NWMQCLIC command
// 
// When the value changes to 1 it will perform IoTConnect discovery and write NVRAM values, the MQTT client
// will be restarted and the new values from IoTConnect discovery should be used for MQTT.
//
// So sequence of events is:
// - set IoTConnect NVRAM values
// - set DA16X_CONF_STR_MQTT_CLIENT_IOTCONNECT_MODE to 1
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

    if(iotconnect_sdk_init_and_get_config() == NULL) {
        IOTC_ERROR("iotconnect_sdk_init_and_get_config() failed\n");
        return -1;
    }

    platform_poll_iotconnect_mode(&iotc_mode);
    IOTC_DEBUG("DA16X_CONF_STR_MQTT_IOTCONNECT_MODE initial value %d\n", iotc_mode);
    if(iotc_mode != 0)
    {
        //
        // This only sets up the MQTT NVRAM settings, interaction with the mqtt_client is assumed to be done via AT commands
        //
        iotconnect_sdk_setup_mqtt_client();
    }

    while(1)
    {
        //
        // Horrid polling mechanism -- maybe need to have some sort of waitQueue in the future
        //
        vTaskDelay(1000/portTICK_PERIOD_MS);

        platform_poll_iotconnect_mode(&iotc_mode);
        if(iotc_mode == 0)
        {
            continue;
        }
        IOTC_DEBUG("DA16X_CONF_STR_MQTT_IOTCONNECT_MODE changed to %d\n", iotc_mode);

        //
        // This only sets up the MQTT NVRAM settings, interaction with the mqtt_client is assumed to be done via AT commands
        //
        iotconnect_sdk_deinit();
        iotconnect_sdk_init_and_get_config();

        iotconnect_sdk_setup_mqtt_client();
    }

    iotconnect_sdk_deinit();

    IOTC_DEBUG("exiting basic_sample()\n" );
    return 0;
}
#endif
