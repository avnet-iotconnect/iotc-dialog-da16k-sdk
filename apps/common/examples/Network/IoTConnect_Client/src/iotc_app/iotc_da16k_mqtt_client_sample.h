#ifndef MQTT_CLIENT_SAMPLE_H
#define MQTT_CLIENT_SAMPLE_H

#include <stdbool.h>

#define MQTT_DEBUG(...) do{ PRINTF(GREEN_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)
#define MQTT_WARN(...) do{ PRINTF(YELLOW_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)
#define MQTT_ERROR(...) do{ PRINTF(RED_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)

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
int mqtt_sample_client_send(const char *topic, const char *pub_message, bool copy);
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

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_SAMPLE_H
