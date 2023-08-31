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
#include "basic_sample.h"

#include "atcmd.h"

//
// This is a standalone example that directly communicates with the mqtt_client and sends it's own telemetry information
//
// The HTTPS and MQTTS certificates (and device certificate / private key) are saved to NVRAM explicitly in the executable
//
#undef STANDALONE
//#define STANDALONE 1

#define    EVT_IOTC_STOP      (1UL << 0x00)
#define    EVT_IOTC_SETUP     (1UL << 0x01)
#define    EVT_IOTC_START     (1UL << 0x02)
#define    EVT_IOTC_RESET     (1UL << 0x03)

static EventGroupHandle_t my_app_event_group = NULL;

void stop_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_STOP);
}

void setup_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_SETUP);
}

void start_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_START);
}

void reset_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_RESET);
}

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
    platform_get_iotconnect_use_cmdack(&iotc_cmdack);

    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        goto cleanup;
    }

    if(iotc_cmdack != 1) {
        IOTC_DEBUG("iotc_cmdack %d -- ignoring command\n", iotc_cmdack);

        //
        // If DA16X_CONF_STR_MQTT_IOTCONNECT_USE_CMD_ACK is not 1 a validly treated command will immediately respond false
        //
        iotconnect_command_status(type, ack_id, false, "Not implemented");
        goto cleanup_ack_id;
    }

    //
    // When DA16X_CONF_STR_MQTT_IOTCONNECT_USE_CMD_ACK is 1, the response to a validly treated command is at the discretion of the AT user.
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

    IOTC_DEBUG("\n\n%s type: %d\n", __func__, type);
    IOTC_DEBUG("%s ack_id: %s\n", __func__, ack_id);
    IOTC_DEBUG("%s command: %s\n\n", __func__, command);
    IOTC_DEBUG("iotconnect_client cmd_ack %d %s 1 \"message\" - if ok\n", type, ack_id);
    IOTC_DEBUG("iotconnect_client cmd_ack %d %s 0 \"message\" - if failed\n\n", type, ack_id);

    atcmd_asynchony_event_for_iccmd(type, ack_id, command);
    free((void *) command);

cleanup_ack_id:
    free((void *) ack_id);
cleanup:
    iotcl_destroy_event(data);
}

static void on_ota(IotclEventData data) {
    int iotc_otaack;
    platform_get_iotconnect_use_otaack(&iotc_otaack);

    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        goto cleanup;
    }

    if(iotc_otaack != 1) {
        IOTC_DEBUG("iotc_otaack %d -- ignoring command\n", iotc_otaack);

        //
        // If DA16X_CONF_STR_MQTT_IOTCONNECT_USE_OTA_ACK is not 1 a validly treated OTA message will immediately respond false
        //
        iotconnect_command_status(type, ack_id, false, "Not implemented");
        goto cleanup_ack_id;
    }

    //
    // When DA16X_CONF_STR_MQTT_IOTCONNECT_USE_OTA_ACK is 1, the response to a validly treated OTA message is at the discretion of the AT user.
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

    IOTC_DEBUG("\n\n%s ack_id: %s\n", __func__, ack_id);
    IOTC_DEBUG("%s version: %s\n", __func__, version);
    IOTC_DEBUG("%s url: %s\n\n", __func__, url);
    IOTC_DEBUG("iotconnect_client ota_ack %s 1 \"message\" - if ok\n", ack_id);
    IOTC_DEBUG("iotconnect_client ota_ack %s 0 \"message\" - if failed\n\n", ack_id);

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
extern void http_broker_cert_config(const char *root_ca, int root_ca_len);

/* DigiCert Global Root G2 */
static const char *http_root_ca_buffer0 =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"
"MrY=\r\n"
"-----END CERTIFICATE-----\r\n";

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

#if 0
#define IOTCONNECT_DUID "binarytestdevice"
#define IOTCONNECT_AUTH_TYPE IOTC_AT_SYMMETRIC_KEY
static const char *iotc_base64_symmetric_key = "YzlgdRbYcreYW1fhjwxO4b3X7hBlDY3OVuw6q9wDbAo="; // note this is base64 encoded
static const char *mqtt_device_cert0 = NULL;
static const char *mqtt_device_private_key0 = NULL;
#endif

#if 0
#define IOTCONNECT_DUID "justatoken"
#define IOTCONNECT_AUTH_TYPE IOTC_AT_TOKEN
static const char *iotc_base64_symmetric_key = NULL;
static const char *mqtt_device_cert0 = NULL;
static const char *mqtt_device_private_key0 = NULL;
#endif

#if 0
#define IOTCONNECT_DUID "left2myowndevice"
#define IOTCONNECT_AUTH_TYPE IOTC_AT_SYMMETRIC_KEY
static const char *iotc_base64_symmetric_key = "cGFzc3dvcmRwYXNzd29yZAo="; // note this is base64 encoded
static const char *mqtt_device_cert0 = NULL;
static const char *mqtt_device_private_key0 = NULL;
#endif

#if 0
#define IOTCONNECT_DUID "x509device"
#define IOTCONNECT_AUTH_TYPE IOTC_AT_X509
static const char *iotc_base64_symmetric_key = NULL;

/*
 * ecc-certs/avtds/x509device-crt.pem
 */
static const char *mqtt_device_cert0 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIByzCCAXACAQEwCgYIKoZIzj0EAwIwczELMAkGA1UEBhMCVUsxEDAOBgNVBAgM\n"
    "B0JyaXN0b2wxEDAOBgNVBAcMB0JyaXN0b2wxEDAOBgNVBAoMB1dpdGVraW8xEDAO\n"
    "BgNVBAsMB0JyaXN0b2wxHDAaBgNVBAMME1VLQnJpc3RvbFdpdGVraW9ORE0wIBcN\n"
    "MjMwNjA2MTQ0OTMxWhgPMjA1MDEwMjExNDQ5MzFaMHAxCzAJBgNVBAYTAlVLMRAw\n"
    "DgYDVQQIDAdCcmlzdG9sMRAwDgYDVQQHDAdCcmlzdG9sMRAwDgYDVQQKDAdXaXRl\n"
    "a2lvMRAwDgYDVQQLDAdCcmlzdG9sMRkwFwYDVQQDDBBhdnRkcy14NTA5ZGV2aWNl\n"
    "MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEJSSSLBrIyBe4TJgKC4pwjn/ZWoNH1CU2\n"
    "dFXHuHj3UYiUJyrZ2P/cMWmOeWQDxBwCUxwiuJ6lr714bkmbVSFZMzAKBggqhkjO\n"
    "PQQDAgNJADBGAiEAx2ixvk+8jvpHG0c3xt4IY6vky9DNzcixNYKQihqDUb4CIQDG\n"
    "HGBV/JfCuOj1fFIdVZbF/dJQTOzy4GIYuLnO8Z0L9A==\n"
    "-----END CERTIFICATE-----\n";
/*
 * ecc-certs/avtds/x509device-key.pem
 */
static const char *mqtt_device_private_key0 =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MHQCAQEEIA/3tFBwrmgBY491Rpdv60Cp4wt+MVFejSa+5U+jlS+HoAcGBSuBBAAK\n"
    "oUQDQgAEJSSSLBrIyBe4TJgKC4pwjn/ZWoNH1CU2dFXHuHj3UYiUJyrZ2P/cMWmO\n"
    "eWQDxBwCUxwiuJ6lr714bkmbVSFZMw==\n"
    "-----END EC PRIVATE KEY-----\n";
#endif

#if 1
#define IOTCONNECT_DUID "selfsignedv1"
#define IOTCONNECT_AUTH_TYPE IOTC_AT_X509 // FIXME TODO actually IOTC_AT_SELF_SIGNED
static const char *iotc_base64_symmetric_key = NULL;

/*
 * E119D239D166719652EFE06A5A7004DCE421C959/DeviceCertificate.pem 
 */
static const char *mqtt_device_cert0 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIICxzCCAa8CFDuST50yWFK3bOGaaesggUXeyWpGMA0GCSqGSIb3DQEBCwUAMCIx\n"
    "IDAeBgNVBAMMF0lvdEh1YiB0ZXN0IGNlcnRpZmljYXRlMB4XDTIzMDYxNDE2MTQx\n"
    "N1oXDTI0MDYxMzE2MTQxN1owHjEcMBoGA1UEAwwTYXZ0ZHMtc2VsZnNpZ25lZF92\n"
    "MTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALjszhOwdeJunimxl7P2\n"
    "tmomYwYm2DR6c9EtAIl2ycTbdYJq78RCn+BA/to5U8LIf8q6Be4OYGJzuQcpJirn\n"
    "sy7HqTD0mqgsStN4rMP8paB08rfHM+6c8H67oW53CK3J3l6nPTE8cnV34Ch3bIzy\n"
    "CiJVP/DJ7AB7NkQCxjda+shuOW+hAOrXRXiAUyiMUp5GOR05R2Erad73YpQNXyRR\n"
    "5PcMGG2teyZBonHk/f3aXFncTlLPzEQyrM1wZ3B2jaUJ6dsBzURnftiraZ+hhNXI\n"
    "3WsHdd58wq9jkCbYtkqa/W8rbhvXu+O8o1AMrTwulCDrZ1/njHhUnCApeXD8b7qB\n"
    "kwUCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAU3sQK9+IAXb3hqO4rkrM61H1Ozb5\n"
    "y/a3xOSt1c2/6BwdmiP239VOfeIQ+p1KNMhoz4emiXT9cJlkzSBzl3bZw01MCGEb\n"
    "Ub0QP7G0IdUvxnyyai95OWiGkzoXG8HIBHeD11gQc9KVXw0hnQT/UJ9bQYQ6S9yL\n"
    "NqziL8wf/+BATRm7YPDs8NFVtjd4A0drp36zbYo23zZJ7ud/QHZdO6qwFzrqC9dB\n"
    "N0sMH8gYzQUBnkksuR3QOTSMnTrqY3LjiSZfgq5iFgF0J7OPerfNCum1TCV7aDeW\n"
    "8JoBQQc3SJY4bq9gUvcrERW9JWHqIHzIf2ljy0hOr7PBVP0ZyqjtyYo1Bg==\n"
    "-----END CERTIFICATE-----\n";
/*
 * E119D239D166719652EFE06A5A7004DCE421C959/device.key 
 */
static const char *mqtt_device_private_key0 =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC47M4TsHXibp4p\n"
    "sZez9rZqJmMGJtg0enPRLQCJdsnE23WCau/EQp/gQP7aOVPCyH/KugXuDmBic7kH\n"
    "KSYq57Mux6kw9JqoLErTeKzD/KWgdPK3xzPunPB+u6Fudwityd5epz0xPHJ1d+Ao\n"
    "d2yM8goiVT/wyewAezZEAsY3WvrIbjlvoQDq10V4gFMojFKeRjkdOUdhK2ne92KU\n"
    "DV8kUeT3DBhtrXsmQaJx5P392lxZ3E5Sz8xEMqzNcGdwdo2lCenbAc1EZ37Yq2mf\n"
    "oYTVyN1rB3XefMKvY5Am2LZKmv1vK24b17vjvKNQDK08LpQg62df54x4VJwgKXlw\n"
    "/G+6gZMFAgMBAAECggEAIKglDmITt1yMjtVfcncHqMBFrYDvnnjfehml5iC1qg+N\n"
    "dW4YEIMpg6tGtlf4XEYXF0B5qpwcVlaUXtjb9ii7wm+dB9mydC96OVUuwV+dCjSM\n"
    "5nqFx8YsTF0rOXxI6pPjMrj6+/ZuSiNU0SCh6VQXCRAf/hBSnpUjtKf+xCjLb9iF\n"
    "u8owuREOGeOLeILWUxZX0RVj2TBedxVsPwbZ0GgXbxie40CQlU6J+rbvs4A2ICDn\n"
    "mGBrcKCXMaBFxVmiwgOBhOSZP2fZwg0vvxws/vNtb3LPoPlnmc6+DS/dCsAwd1lD\n"
    "N5PSUvGl77mh0c7lmZpOpIJNVxfJd+UP4JhkU/U+cQKBgQC+DdlhelvY1hqU+1Vr\n"
    "ejovYrZSHTSWrDVZaoOOiw6OgudmEyWrj7ifcdStUgpoPji7IPa5hSzkqyVhsFiL\n"
    "NudgoAkVXHNoQZ0PuR+8IkBGRQXH2SWdFVEdY+cQb2MqOyf3a4xfe+2HsrZH1obZ\n"
    "NieMljeR98qnHJrklErlvo6eeQKBgQD5F1lvK0DFUt/moccD2oSiuD1fI4KSk8P2\n"
    "xWENWBy1pAq6kBJh0qc+JGWZRMrt/AHmDFJnGQxa7680NbIKte3QQh/mY91eLpY/\n"
    "LGrX/n+Lyvfj4vdKVm8h0RdbRyUhFcg8umFIl3R8p9e9JX4omLnUPpnq69Z3kfkA\n"
    "4RE1IESF7QKBgHqC5uwFOguvCHedBFVB9xvwn+KS3QF7hPBczu0mCn4nOA7+rLvI\n"
    "65QshpIrXnYQFfXaq/CvPl8xS+mLCajD/aa1wuU4MVS1Zw9poGFgGtqxR6ap/asi\n"
    "wKUXby2S//OLKpo9g8FRW66rrwDj8w018YyYkL3RY5sRv281gIpUqg7RAoGASnQN\n"
    "OmpeSNzVqfUvLFqzjIOvbHGLxM5AI9GpibiNlEl9H3iS1gSGEtAEQkTKt0m9M4r5\n"
    "UnGtPL0pzFxEZGkutTIeoNm2wEECjc1z/i3G5/z6DXa43dJqE1yRM6pXUcVV/bjj\n"
    "/TOwENaGaLX9OJs16Ffx38MwbrsGB5o+b3e+o50CgYAjrP0pGonOBXO351/IXciY\n"
    "64fYuKiTc261ovbqas2YyFPSEhsRU+CpUY1L6aoF1z+xJP9OZnX4gm83MtP+5Ho/\n"
    "8rxhr13/gPJIwLGDUb6HgnvRwYQGBkLKwnwYFer/ZI0x6iuBEcEl8QP3OhSmdmZw\n"
    "T5VocTUIZufG5RXYT6DGWQ==\n"
    "-----END PRIVATE KEY-----\n";
#endif

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

    my_app_event_group = xEventGroupCreate();
    if (my_app_event_group == NULL) {
        IOTC_ERROR("[%s] Event group Create Error!", __func__);
        return -1;
    }

    /* clear wait bits here */
    xEventGroupClearBits( my_app_event_group, (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET) );

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
        IOTC_ERROR("iotconnect_sdk_init_and_set_config() failed\n");
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

    http_broker_cert_config(http_root_ca_buffer0, strlen(http_root_ca_buffer0));

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

    if(my_app_event_group) {
        vEventGroupDelete(my_app_event_group);
        my_app_event_group = NULL;
    }
    
    iotconnect_sdk_reset_config();
    config = NULL;

    IOTC_DEBUG("exiting basic_sample()\n");
    return 0;
}
#else

//
// a wrapper to send +NWICSETUPBEGIN / +NWICSETUPEND messages
//
IotConnectClientConfig *setup_wrapper(void)
{
    atcmd_asynchony_event_for_icsetup_begin();

    IotConnectClientConfig *config = NULL;

    //
    // Try a small number of times - in case of an intermitent failure
    //
    for(int i = 0;i < 5;i++) {
        //
        // Load up config with new IOTC_XXXX values
        //
        // Clear any previous discovery/sync responses
        //
        // note iotconnect_sdk_init_and_get_config() here should free any dynamic memory that discovery_response / sync_response / config may be holding onto
        //
        config = iotconnect_sdk_init_and_get_config();
        if(config != NULL) {
            break;
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    if(config == NULL) {
        IOTC_ERROR("iotconnect_sdk_init_and_get_config() failed\n");
    } else {
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
    }

    if(config == NULL) {
        IOTC_ERROR("setup_wrapper() failed - no config available\n");
    }

    atcmd_asynchony_event_for_icsetup_end(config != NULL);
    return config;
}

//
// a wrapper to send +NWICSTARTBEGIN / +NWICSTARTEND messages
//
int start_wrapper(IotConnectClientConfig *config)
{
    int ret = -1;

    atcmd_asynchony_event_for_icstart_begin();

    if(config == NULL) {
        IOTC_ERROR("start_wrapper() failed - no config available\n");
        goto end;
    }

    //
    // Try a small number of times - in case of an intermitent failure
    //
    for(int i = 0;i < 5;i++) {
        //
        // Run discovery/sync
        // Startup mqtt_client with new MQTT_XXXX values
        //
        // Because discovery/sync responses are cleared below -- the discovery/sync will also run (with up to date values).
        //
        ret = iotconnect_sdk_init();
        if(ret == 0) {
            break;
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

end:
    if (ret != 0) {
        IOTC_ERROR("start_wrapper() failed: %d\n", ret);
    }

    atcmd_asynchony_event_for_icstart_end(ret == 0);
    return ret;
}

//
// a wrapper to send +NWICSTOPBEGIN / +NWICSTOPEND messages
//
static void stop_wrapper(void)
{
    atcmd_asynchony_event_for_icstop_begin();
    if(iotconnect_sdk_is_connected() == true) {
        iotconnect_sdk_disconnect();
    }
    atcmd_asynchony_event_for_icstop_end(true);
}

//
// a wrapper to send +NWICRESETBEGIN / +NWICRESETEND messages
//
static void reset_wrapper(void)
{
    atcmd_asynchony_event_for_icreset_begin();
    iotconnect_sdk_reset_config();
    atcmd_asynchony_event_for_icreset_end(true);
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
    IOTC_WARN("\n\n\nRunning in AT command mode\n\n\n");

    /*
     * Explicity *NOT* setting any certificates for "AT command" version
     *
     * Certificates can be set see: Table 16, Page 50, "UM-WI-003 DA16200 DA16600 Host Interface and AT Command User Manual" 
     */

    IotConnectClientConfig *config = NULL;

    my_app_event_group = xEventGroupCreate();
    if (my_app_event_group == NULL) {
        IOTC_ERROR("[%s] Event group Create Error!", __func__);
        return -1;
    }

    /* clear wait bits here */
    xEventGroupClearBits( my_app_event_group, (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET) );

    //
    // Initially run the setup and start iotconnect
    //
    xEventGroupSetBits(my_app_event_group, (EVT_IOTC_SETUP | EVT_IOTC_START));

    while(1)
    {
        EventBits_t events = xEventGroupWaitBits(my_app_event_group,
                                     (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET),
                                     pdFALSE,
                                     pdFALSE,
                                     30000 / portTICK_PERIOD_MS);

        bool reset = events & EVT_IOTC_RESET;
        bool start = events & EVT_IOTC_START;
        bool setup = events & EVT_IOTC_SETUP;
        bool stop = events & EVT_IOTC_STOP;

        xEventGroupClearBits( my_app_event_group, (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET) );

        if(start == false && setup == false && stop == false && reset == false) {
            // just timed out
            continue;
        }

        //
        // disconnect from iotconnect, 
        // teardown the current config and deallocate any memory
        //
        if(reset) {
            reset_wrapper();
            config = NULL; // config will no longer be valid after reset_wrapper();
            continue;
        }

        //
        // disconnect from iotconnect, but leave the config intact
        //
        if(stop) {
            stop_wrapper();
            continue;
        }

        if(setup) {
            config = setup_wrapper();
        }

        if(start) {
            start_wrapper(config);
        }
    }

    if(my_app_event_group) {
        vEventGroupDelete(my_app_event_group);
        my_app_event_group = NULL;
    }
    
    iotconnect_sdk_reset_config();
    config = NULL;
    
    IOTC_DEBUG("exiting basic_sample()\n" );
    return 0;
}
#endif
