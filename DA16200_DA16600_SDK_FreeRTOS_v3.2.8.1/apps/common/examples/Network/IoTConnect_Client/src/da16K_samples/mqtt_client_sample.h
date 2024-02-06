#ifndef MQTT_CLIENT_SAMPLE_H
#define MQTT_CLIENT_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*msg_cb_t)(const char *buf, int len, const char *topic);
typedef void (*pub_cb_t)(int mid);
typedef void (*conn_cb_t)(void);
typedef void (*disconn_cb_t)(void);
typedef void (*sub_cb_t)(void);
typedef void (*unsub_cb_t)(void);

int mqtt_sample_client_init(msg_cb_t, pub_cb_t, conn_cb_t, disconn_cb_t, sub_cb_t, unsub_cb_t);
void mqtt_sample_client_deinit(void);
int mqtt_sample_client_pub_send(const char *pub_message, bool copy);
bool mqtt_sample_client_is_running(void);

/*
 * password can be NULL
 */
void mqtt_sample_client_nvram_config(const char *broker,
		              int port,
			      const char *username,
			      const char *password,
			      const char *clientid, 
			      const char *pub,
			      const char *sub,
                              int qos);

void mqtt_broker_cert_config(const char *root_ca, unsigned int root_ca_len);

void mqtt_device_cert_config(const char *device_cert, unsigned int device_cert_len,
			     const char *device_private_key, unsigned int device_private_key_len);

/*
 * Not so much MQTT as DA16200 platform-related -- included here for convenience as mqtt_client_sample.c is already using "device" routines
 */ 
int platform_get_iotconnect_cpid(char *string);
int platform_get_iotconnect_env(char *string);
int platform_get_iotconnect_duid(char *string);
int platform_get_iotconnect_auth_type(char *string);
int platform_get_iotconnect_symmetric_key(char *string);
int platform_get_iotconnect_cd(char *string);

int platform_set_iotconnect_dtg(char *string);
int platform_get_iotconnect_dtg(char *string);

int platform_get_iotconnect_use_cmdack(int *string);
int platform_set_iotconnect_use_cmdack(void);

int platform_get_iotconnect_use_otaack(int *string);
int platform_set_iotconnect_use_otaack(void);

int platform_get_mqtt_broker_ip(char *string);
int platform_get_mqtt_broker_username(char *string);
int platform_get_mqtt_broker_password(char *string);
int platform_get_mqtt_sub_topic0(char *string);
int platform_get_mqtt_pub_topic(char *string);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_SAMPLE_H
