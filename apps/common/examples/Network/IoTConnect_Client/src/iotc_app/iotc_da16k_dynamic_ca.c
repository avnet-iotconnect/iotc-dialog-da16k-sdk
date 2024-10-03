/* 
    Dynamic certificate store for IoTC on DA16K.

    Author: evoirin
    (C) Avnet 2024
*/

#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"

#include "iotcl_certs.h"
#include "iotc_da16k_dynamic_ca.h"

static const char s_mqtt_ca_aws[]       = IOTCL_AMAZON_ROOT_CA1;
static const char s_mqtt_ca_azure[]     = IOTCL_CERT_DIGICERT_GLOBAL_ROOT_G2;
static const char s_http_ca_aws_azure[] = IOTCL_CERT_GODADDY_SECURE_SERVER_CERTIFICATE_G2;

static const char *s_mqtt_root_ca       = NULL;
static const char *s_http_root_ca       = NULL;

void iotc_da16k_dynamic_ca_set(IotConnectConnectionType type) {
    switch (type) {
        case IOTC_CT_AWS:
            s_mqtt_root_ca = s_mqtt_ca_aws;
            s_http_root_ca = s_http_ca_aws_azure;
            return;
        case IOTC_CT_AZURE:
            s_mqtt_root_ca = s_mqtt_ca_azure;
            s_http_root_ca = s_http_ca_aws_azure;
            return;
        default:
            iotc_da16k_dynamic_ca_clear();
    }
}

void iotc_da16k_dynamic_ca_clear(void) {
    s_mqtt_root_ca = NULL;
    s_http_root_ca = NULL;
}

const char *iotc_da16k_dynamic_ca_mqtt_get(void) {
    return s_mqtt_root_ca;
}

const char *iotc_da16k_dynamic_ca_http_get(void) {
    return s_http_root_ca;
}
