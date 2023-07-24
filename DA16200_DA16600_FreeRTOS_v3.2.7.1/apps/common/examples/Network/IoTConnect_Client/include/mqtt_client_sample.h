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
int platform_get_cpid(char *cpid);
int platform_get_env(char *env);
int platform_get_duid(char *duid);
int platform_get_auth_type(char *at);
int platform_get_symmetric_key(char *symmetric_key);

int platform_set_dtg(char *string);
int platform_get_dtg(char *string);

int platform_poll_iotconnect_mode(int *mode);
int platform_reset_iotconnect_mode(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_SAMPLE_H
