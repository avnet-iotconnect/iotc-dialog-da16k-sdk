#ifndef IOTC_DA16K_UTIL_H
#define IOTC_DA16K_UTIL_H

#include "iotconnect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read IOTConnectClientConfig parameters from nvram environment */
int iotc_da16k_read_config(IotConnectClientConfig *c);
void iotc_da16k_reset_config(IotConnectClientConfig *c);

int iotc_da16k_set_nvcache_int(int key, int value);
int iotc_da16k_set_nvcache_str(int key, const char *value);

int platform_get_iotconnect_connection_type(char *string);
int platform_get_iotconnect_cpid(char *string);
int platform_get_iotconnect_env(char *string);
int platform_get_iotconnect_duid(char *string);
int platform_get_iotconnect_auth_type(char *string);
int platform_get_iotconnect_symmetric_key(char *string);
int platform_get_iotconnect_cd(char *string);

int platform_get_iotconnect_use_cmdack(int *value);
int platform_set_iotconnect_use_cmdack(void);

int platform_get_iotconnect_use_otaack(int *value);
int platform_set_iotconnect_use_otaack(void);

int platform_get_mqtt_qos(int *value);

int platform_get_mqtt_broker_ip(char *string);
int platform_get_mqtt_broker_username(char *string);
int platform_get_mqtt_broker_password(char *string);
int platform_get_mqtt_sub_topic0(char *string);
int platform_get_mqtt_pub_topic(char *string);

int mqtt_client_preinit_certs(IotConnectClientConfig *c);

#ifdef __cplusplus
}
#endif

#endif
