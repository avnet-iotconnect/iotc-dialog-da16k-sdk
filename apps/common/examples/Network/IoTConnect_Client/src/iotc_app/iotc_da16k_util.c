#include "iotconnect.h"

#include "da16x_system.h"
#include "da16200_ioconfig.h"
#include "user_nvram_cmd_table.h"
#include "mqtt_client.h"

#include "iotcl_util.h"
#include "iotc_da16k_util.h"
#include "iotc_da16k_mqtt_client_sample.h"

static char __dummy__[] = "dummy";

int iotc_da16k_read_config(IotConnectClientConfig *c) {
    char temp_str[256];
    int temp_int;

    if (c->env || c->cpid || c->duid || c->auth_info.data.symmetric_key) {
        PRINTF("ERROR: Configuration was not reset before calling %s.\n", __func__);
        goto cleanup;
    }

    c->env = NULL;
    c->cpid = NULL;
    c->duid = NULL;

    if(platform_get_iotconnect_connection_type(temp_str) != 0)
    {
        PRINTF("platform_get_iotconnect_connection_type() failed\n");
        goto cleanup;
    }
    if(sscanf(temp_str, "%d", &temp_int) != 1)
    {
        PRINTF("Failed to sscanf IoTC Connection Type \"%s\"\n", temp_str);
        goto cleanup;
    }
    c->connection_type = temp_int;

    /* Generic C SDK will check these but we don't use them on DA16k. 
       Cert handling is done directly on flash by DA16K SDK's MQTT client,
       so we put dummy values here. */

    c->auth_info.data.cert_info.device_cert = __dummy__;
    c->auth_info.data.cert_info.device_key = __dummy__;
    c->auth_info.trust_store = __dummy__;

    if(platform_get_iotconnect_env(temp_str) != 0)
    {
        PRINTF("platform_get_env() failed\n");
        goto cleanup;
    }

    c->env = iotcl_strdup(temp_str);
    if(c->env == NULL)
    {
        PRINTF("iotcl_strdup() failed\n");
        goto cleanup;
    }

    if(platform_get_iotconnect_cpid(temp_str) != 0)
    {
        PRINTF("platform_get_iotconnect_cpid() failed\n");
        goto cleanup;
    }
    c->cpid = iotcl_strdup(temp_str);
    if(c->cpid == NULL)
    {
        PRINTF("iotcl_strdup() failed\n");
        goto cleanup;
    }

    if(platform_get_iotconnect_duid(temp_str) != 0)
    {
        PRINTF("platform_get_iotconnect_duid() failed\n");
        goto cleanup;
    }
    c->duid = iotcl_strdup(temp_str);
    if(c->duid == NULL)
    {
        PRINTF("iotcl_strdup() failed\n");
        goto cleanup;
    }

    if(platform_get_iotconnect_auth_type(temp_str) != 0)
    {
        PRINTF("platform_get_iotconnect_auth_type() failed\n");
        goto cleanup;
    }
    if(sscanf(temp_str, "%d", &temp_int) != 1)
    {
        PRINTF("Failed to sscanf IOTC_AUTH_TYPE \"%s\"\n", temp_str);
        goto cleanup;
    }
    c->auth_info.type = temp_int;

    if(c->auth_info.type == IOTC_AT_SYMMETRIC_KEY)
    {
        if(platform_get_iotconnect_symmetric_key(temp_str) != 0)
        {
            PRINTF("platform_get_iotconnect_symmetric_key() failed\n");
            goto cleanup;
        }
        c->auth_info.data.symmetric_key = iotcl_strdup(temp_str);
        if(c->auth_info.data.symmetric_key == NULL)
        {
            PRINTF("iotcl_strdup() failed\n");
            goto cleanup;
        }
    }

    if (platform_get_mqtt_qos(&c->qos) != 0)
    {
        PRINTF("platform_get_iotconnect_mqtt_qos() failed\n");
        goto cleanup;
    }

    return 0;

cleanup:
    iotc_da16k_reset_config(c);
    return -1;
}

void  iotc_da16k_reset_config(IotConnectClientConfig *c) {
    if (c) {
        free(c->cpid);
        free(c->duid);
        free(c->env);
        if (c->auth_info.type == IOTC_AT_SYMMETRIC_KEY) {
            free(c->auth_info.data.symmetric_key);
        }

        // DA16K doesn't use the key/cert stuff so we don't have to free

        memset(c, 0, sizeof(IotConnectClientConfig));
    }
}


/**
 ****************************************************************************************
 * @brief MQTT Basic Configuration Function
 * @subsection Parameters
 * - Broker IP Address
 * - Broker Port Number
 * - Qos
 * - Subscriber Topic
 * - Publisher Topic
 * - SNTP Use (for TLS valid time)
 * - TLS Use
 * - TLS Root CA
 * - TLS Client Certificate
 * - TLS Client Private Key
 ****************************************************************************************
 */

int iotc_da16k_set_nvcache_int(int key, int value) {
    int test_int;
    if (da16x_get_config_int(key, &test_int) != CC_SUCCESS || test_int != value) {
        if (da16x_set_nvcache_int(key, value) != CC_SUCCESS) {
            MQTT_ERROR("da16x_set_nvcache_int(%d, %d) failed\n", key, value);
        }
        return 1;
    }
    return 0;
}

int iotc_da16k_set_nvcache_str(int key, const char *value) {
    if(value != NULL) {
        char test_str[MQTT_PASSWORD_MAX_LEN] = {0, }; // password has the max length in str type
        if (da16x_get_config_str(key, test_str) != CC_SUCCESS || strcmp(test_str, value) != 0) {
            if (da16x_set_nvcache_str(key, (char *) value) != CC_SUCCESS) {
                MQTT_ERROR("da16x_set_nvcache_str(%d, %s) failed\n", key, value);
            }
            return 1;
        }
    } else {
        if(da16x_set_nvcache_str(key, NULL) != CC_SUCCESS) {
            MQTT_ERROR("da16x_set_nvcache_str(%d, NULL) failed\n", key);
        }
        return 1;
    }
    return 0;
}

int platform_get_iotconnect_connection_type(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_CONNECTION_TYPE, string);
    if (status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_CONNECTION_TYPE\n");
        return -1;
    }
    if (*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_CONNECTION_TYPE value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_iotconnect_use_cmdack(int *value) {
    int status;
    char string[8];

    *value = 0;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK, string);
    if (status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK\n");
        return -1;
    }
    if (*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK value is empty\n");
        return -1;
    }
    *value = atoi(string);

    return 0;
}

int platform_set_iotconnect_use_cmdack(void) {
    int status = da16x_set_config_str(DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK, "0");
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to write DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK\n");
        return -1;
    }

    return 0;
}

int platform_get_iotconnect_use_otaack(int *value) {
    int status;
    char string[8];

    *value = 0;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK value is empty\n");
        return -1;
    }
    *value = atoi(string);

    return 0;
}

int platform_set_iotconnect_use_otaack(void) {
    int status = da16x_set_config_str(DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK, "0");
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to write DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK\n");
        return -1;
    }

    return 0;
}

int platform_get_iotconnect_cpid(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_CPID, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_CPID\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_CPID value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_iotconnect_env(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_ENV, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_ENV\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_ENV value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_iotconnect_duid(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_DUID, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_DUID\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_DUID value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_iotconnect_auth_type(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_iotconnect_symmetric_key(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_mqtt_qos(int *value) {
    int status;
    status = da16x_get_config_int(DA16X_CONF_INT_MQTT_QOS, value);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_INT_MQTT_QOS\n");
        return -1;
    }
    return 0;
}

int platform_get_mqtt_broker_ip(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_MQTT_BROKER_IP, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_MQTT_BROKER_IP\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_MQTT_BROKER_IP value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_mqtt_broker_username(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_MQTT_USERNAME, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_MQTT_USERNAME\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_MQTT_USERNAME value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_mqtt_broker_password(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_MQTT_PASSWORD, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_MQTT_PASSWORD\n");
        return -1;
    }
    /* note broker password can be empty */
    return 0;
}

int platform_get_mqtt_sub_topic0(char *string) {
    char topics[16] = {0, };
    char *tmp_str;

    *string = '\0';

    sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, 0);
    tmp_str = read_nvram_string(topics);

    if(tmp_str == NULL) {
        MQTT_ERROR("Failed to get %s\n", topics);
        return -1;
    }

    if(*tmp_str == '\0') {
        MQTT_ERROR("Failed to get %s\n", topics);
        return -1;
    }

    strcpy(string, tmp_str);
    return 0;
}

int platform_get_mqtt_pub_topic(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_MQTT_PUB_TOPIC, string);
    if(status != CC_SUCCESS) {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_MQTT_PUB_TOPIC\n");
        return -1;
    }
    if(*string == '\0') {
        MQTT_ERROR("DA16X_CONF_STR_MQTT_PUB_TOPIC value is empty\n");
        return -1;
    }
    return 0;
}
