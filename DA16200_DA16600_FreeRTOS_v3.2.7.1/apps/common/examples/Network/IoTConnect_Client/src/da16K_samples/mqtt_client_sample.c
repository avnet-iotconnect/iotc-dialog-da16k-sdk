/**
 ****************************************************************************************
 *
 * @file mqtt_client_sample.c
 *
 * @brief MQTT Client Sample Application
 *
 * Copyright (c) 2016-2022 Renesas Electronics. All rights reserved.
 *
 * This software ("Software") is owned by Renesas Electronics.
 *
 * By using this Software you agree that Renesas Electronics retains all
 * intellectual property and proprietary rights in and to this Software and any
 * use, reproduction, disclosure or distribution of the Software without express
 * written permission or a license agreement from Renesas Electronics is
 * strictly prohibited. This Software is solely for use on or in conjunction
 * with Renesas Electronics products.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE
 * SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. EXCEPT AS OTHERWISE
 * PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * RENESAS ELECTRONICS BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 *
 ****************************************************************************************
 */


#define __MQTT_CLIENT_SAMPLE__
#if defined (__MQTT_CLIENT_SAMPLE__)

#include "sdk_type.h"
#include "da16x_system.h"
#include "task.h"
#include "net_bsd_sockets.h"
#include "da16x_network_common.h"
#include "user_dpm.h"
#include "user_dpm_api.h"
#include "common_def.h"
#include "da16200_ioconfig.h"
#include "command_net.h"
#include "mqtt_client.h"
#include "user_nvram_cmd_table.h"
#include "util_api.h"
#include "mqtt_client_sample.h"

// or specify as functions
#define MQTT_DEBUG(...) do{ PRINTF(GREEN_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)
#define MQTT_WARN(...) do{ PRINTF(YELLOW_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)
#define MQTT_ERROR(...) do{ PRINTF(RED_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)

#define	SAMPLE_MQTT_CLIENT		"MQTT_CLIENT"
#define	NAME_MY_APP_EVENT_HANDLER	"MY_APP_EVENT_HANDLER"
#define	NAME_MY_APP_Q_HANDLER		"MY_APP_Q_HANDLER"

/// Wait count for mqtt send: in multiple of 100 ms
#define MQTT_PUB_MAX_WAIT_CNT 100

/// The Message Queue size of my application queue
#define MSG_Q_SIZE 5

/// If mqtt_client_send_message_with_qos() is used for mqtt publish, enable.
#undef USE_MQTT_SEND_WITH_QOS_API

/*
 * FIXME TODO tx_publish is used at present to indicate that a message is being published - akin to a reference count.
 *
 * Ideally, the message Id's would be kept - if it is crucial to determine if/when a message has been sent.
 */
static int tx_publish = 0;

static TaskHandle_t _mqtt_q_hdler = 0;
QueueHandle_t _mqtt_q = 0;

static msg_cb_t g_msg_cb = NULL;
static pub_cb_t g_pub_cb = NULL;
static conn_cb_t g_conn_cb = NULL;
static disconn_cb_t g_disconn_cb = NULL;
static sub_cb_t g_sub_cb = NULL;
static unsub_cb_t g_unsub_cb = NULL;

static BaseType_t _mqtt_send_to_q(const char *buf, bool copy)
{
    BaseType_t status;

    if (!_mqtt_q) {
        MQTT_ERROR("[%s] Msg Q doesn't exist!", __func__);
	return -1;
    }

    int user_msg_buf_size = strlen(buf) + 1;
    char *user_msg_buf = NULL;
    if(copy) {
        /* Take a copy of the message, as the original may be freed before the Q entry is dealt with? */
        user_msg_buf = malloc(user_msg_buf_size);
    } else {
        // assume buf can use free() on buf once it has been sent - original argument should not be deallocated
        user_msg_buf = (char *) buf;
    }

    if (!user_msg_buf) {
        MQTT_ERROR("[%s] failed to allocate user_msg_buf!", __func__);
    	return -1;
    }

    sprintf(user_msg_buf, "%s", buf);
    tx_publish++;

    status = xQueueSendToBack(_mqtt_q, &user_msg_buf, 10);
    if(status != pdTRUE) {
        free(user_msg_buf);
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief mqtt_client sample callback function for processing PUBLISH messages
 * Users register a callback function to process a PUBLISH message.
 * @param[in] buf the message paylod
 * @param[in] len the message paylod length
 * @param[in] topic the topic this mqtt_client subscribed to
 ****************************************************************************************
 */
static void _mqtt_msg_cb(const char *buf, int len, const char *topic)
{
    MQTT_DEBUG("\n\n[MQTT_SAMPLE] Msg Recv: topic=%s, msg=%s, len = %d\n\n", topic, buf, len);

    if(g_msg_cb) {
        g_msg_cb(buf, len, topic);
    }
}

static void _mqtt_pub_cb(int mid)
{
    MQTT_DEBUG("\n\n[%s] message id %d is being published!\n\n", __func__, mid);

    if (tx_publish > 0) {
        MQTT_DEBUG("[MQTT_SAMPLE] Sending a message complete.\n");
        tx_publish--;
    } else {
        MQTT_WARN("Tx PUB_COMPLETE source unknown, debug needed\n");
    }

    if(g_pub_cb) {
        g_pub_cb(mid);
    }
}

static void _mqtt_conn_cb(void)
{
    MQTT_DEBUG("\n\n[%s] MQTT connection callback!\n\n", __func__);

    if(g_conn_cb) {
        g_conn_cb();
    }
}

static void _mqtt_disconn_cb(void)
{
    MQTT_DEBUG("\n\n[%s] MQTT disconnection callback!\n\n", __func__);

    if(g_disconn_cb) {
        g_disconn_cb();
    }
}

static void _mqtt_sub_cb(void)
{
    MQTT_DEBUG("\n\n[%s] topic subscribed!\n\n", __func__);

    if(g_sub_cb) {
        g_sub_cb();
    }
}

static void _mqtt_unsub_cb(void)
{
    MQTT_DEBUG("\n\n[%s] topic unsubscribed!\n\n", __func__);

    if(g_unsub_cb) {
        g_unsub_cb();
    }
}

int mqtt_sample_client_pub_send(const char *pub_message, bool copy)
{
    BaseType_t ret;

    if (!mqtt_client_is_running()) {
	if(!is_mqtt_client_thd_alive()) {
            MQTT_ERROR("[MQTT_SAMPLE] Mqtt_client is in terminated state, terminating my app ...\n");

            mqtt_sample_client_deinit();
	} else {
            MQTT_WARN("[MQTT_SAMPLE] Mqtt_client may be trying to reconnect ... cancelling the job this time\n");
        }

        return -1;
    } 

    if ((ret = _mqtt_send_to_q(pub_message, copy)) != pdPASS ) {
        MQTT_ERROR("[%s] Failed to add a message to Q (%d)\r\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int _mqtt_pub_msg(const char *buffer)
{
    int ret;
#if !defined (USE_MQTT_SEND_WITH_QOS_API)
    int wait_cnt = 0;

LBL_SEND_RETRY:
    ret = mqtt_client_send_message(NULL, (char *) buffer);
    if (ret != 0) {
        if (ret == -2) { // previous message Tx is not finished
            vTaskDelay(10 / portTICK_PERIOD_MS);

            wait_cnt++;
            if (wait_cnt == MQTT_PUB_MAX_WAIT_CNT) {
                MQTT_WARN("[MQTT_SAMPLE] System is busy (max wait=%d), try next time\n", MQTT_PUB_MAX_WAIT_CNT);
            } else {
                goto LBL_SEND_RETRY;
            }
        }
    }
#else
    ret = mqtt_client_send_message_with_qos(NULL, buffer, MQTT_PUB_MAX_WAIT_CNT);
    if (ret != 0) {
        if (ret == -2) {
            MQTT_ERROR("[MQTT_SAMPLE] Mqtt send not successfully delivered, timtout=%d\n", MQTT_PUB_MAX_WAIT_CNT);
        }
    }
#endif
    return ret;
}

static void _mqtt_q_handler(void *arg)
{
    DA16X_UNUSED_ARG(arg);

    int ret;
    char *user_msg_buf;
    BaseType_t xStatus;

    // message queue init
    while (1) {
        if (!_mqtt_q) {
            MQTT_ERROR("[%s] Msg Q doesn't exist!", __func__);
    	    break;
        }

        xStatus = xQueueReceive(_mqtt_q, &user_msg_buf, portMAX_DELAY);
        if (xStatus != pdPASS) {
            MQTT_ERROR("[%s] Q recv error (%d)\r\n", __func__, xStatus);
            vTaskDelete(NULL);
            break;
        }

        if (user_msg_buf) {
            ret = _mqtt_pub_msg(user_msg_buf);
            if (ret != 0) {
#if !defined (USE_MQTT_SEND_WITH_QOS_API)
                MQTT_ERROR("[MQTT_SAMPLE] Sending a message failed, refer to mqtt_client_send_message()\n");
#else
                MQTT_ERROR("[MQTT_SAMPLE] Sending a message failed, refer to mqtt_client_send_message_with_qos()\n");
#endif
            }

            // allocated in _mqtt_send_to_q()
            free(user_msg_buf);
        } else {
            MQTT_ERROR("[%s] can't send a NULL user_msg_buf\r\n", __func__);
        }
    }

    // shouldn't get here -- mqtt_sample_deinit() should kill the task
    vTaskDelete(NULL);
    return;
}

void mqtt_sample_client_deinit(void)
{
    if (mqtt_client_is_running() == TRUE) {
        mqtt_client_force_stop();
        mqtt_client_stop();
    }

    if (_mqtt_q_hdler) {
        vTaskDelete(_mqtt_q_hdler);
        _mqtt_q_hdler = NULL;
    }

    if (_mqtt_q) {
        vQueueDelete(_mqtt_q);
        _mqtt_q = NULL;
    }
}

static int _mqtt_chk_connection(int timeout)
{
    int wait_cnt = 0;
    for (wait_cnt = 0; wait_cnt < timeout; wait_cnt++) {
        if (mqtt_client_check_conn()) {
             break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    if (wait_cnt == timeout) {
        MQTT_WARN("[%s] mqtt connection timeout!, check your configuration\r\n", __func__);
        return pdFALSE;
    }

    return pdTRUE;
}

/**
 ****************************************************************************************
 * @brief mqtt_client sample description
 * How to run this sample :
 *   See Example UM-WI-055_DA16200_FreeRTOS_Example_Application_Manual
 ****************************************************************************************
 */
int mqtt_sample_client_init(msg_cb_t msg_cb, pub_cb_t pub_cb, conn_cb_t conn_cb, disconn_cb_t disconn_cb, sub_cb_t sub_cb, unsub_cb_t unsub_cb)
{
    g_msg_cb = msg_cb;
    g_pub_cb = pub_cb;
    g_conn_cb = conn_cb;
    g_disconn_cb = disconn_cb;
    g_sub_cb = sub_cb;
    g_unsub_cb = unsub_cb;

    BaseType_t	xRet;

    // stop any MQTT client that might be running with "stale" info or callbacks
    if (mqtt_client_is_running() == TRUE) {
        mqtt_client_force_stop();
        mqtt_client_stop();
    }

    // message queue init
    _mqtt_q = xQueueCreate(MSG_Q_SIZE, sizeof(char *));
    if (_mqtt_q == NULL) {
        MQTT_ERROR("[%s] Msg Q Create Error!", __func__);
        goto cleanup;
    }

    // start Q message handler
    xRet = xTaskCreate(_mqtt_q_handler, NAME_MY_APP_Q_HANDLER, (2048 / 4), NULL,
                       (OS_TASK_PRIORITY_USER + 6), &_mqtt_q_hdler);
    if (xRet != pdPASS) {
        _mqtt_q_hdler = NULL;
        MQTT_ERROR("[%s] Failed to create %s (0x%02x)\n", __func__,
               NAME_MY_APP_Q_HANDLER, xRet);
        goto cleanup;
    }

    // register callbacks
    mqtt_client_set_msg_cb(_mqtt_msg_cb);
    mqtt_client_set_pub_cb(_mqtt_pub_cb);
    mqtt_client_set_conn_cb(_mqtt_conn_cb);
    mqtt_client_set_disconn_cb(_mqtt_disconn_cb);
    // Note: IoTConnect only subscribes to a single topic (on Azure)
    mqtt_client_set_subscribe_cb(_mqtt_sub_cb);
    // Note: IoTConnect doesn't unsubscribe (on Azure)
    mqtt_client_set_unsubscribe_cb(_mqtt_unsub_cb);

    // start MQTT client with new config that's just been saved
    mqtt_client_start();

    // my app terminates if mqtt connection is not made within the specified time.
    if (_mqtt_chk_connection(100) == pdFALSE) {
        MQTT_ERROR("[%s] _mqtt_chk_connection failed\n", __func__);
        return -1;
    }

    return 0;

cleanup:
    // FIXME TODO cleanup partial tasks / Q's / eventGroups, etc.
    return -1;
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

int set_nvcache_int(int key, int value) {
    int test_int;
    if(da16x_get_config_int(key, &test_int) != CC_SUCCESS || test_int != value) {
        if(da16x_set_nvcache_int(key, value) != CC_SUCCESS) {
            MQTT_ERROR("da16x_set_nvcache_int(%d, %d) failed\n", key, value);
	}
	return 1;
    }
    return 0;
}

int set_nvcache_str(int key, const char *value) {
    if(value != NULL) {
        char test_str[MQTT_PASSWORD_MAX_LEN] = {0, }; // password has the max length in str type
        if(da16x_get_config_str(key, test_str) != CC_SUCCESS || strcmp(test_str, value) != 0) {
            if(da16x_set_nvcache_str(key, (char *) value) != CC_SUCCESS) {
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

int platform_get_iotconnect_use_cmdack(int *value) {
    int status;
    char string[8];

    *value = 0;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK value is empty\n");
        return -1;
    }
    *value = atoi(string);

    return 0;
}

int platform_set_iotconnect_use_cmdack(void) {
    int status = da16x_set_config_str(DA16X_CONF_STR_IOTCONNECT_USE_CMD_ACK, "0");
    if(status != CC_SUCCESS)
    {
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
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK value is empty\n");
        return -1;
    }
    *value = atoi(string);

    return 0;
}

int platform_set_iotconnect_use_otaack(void) {
    int status = da16x_set_config_str(DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK, "0");
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to write DA16X_CONF_STR_IOTCONNECT_USE_OTA_ACK\n");
        return -1;
    }

    return 0;
}
int platform_get_cpid(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_CPID, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_CPID\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_CPID value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_env(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_ENV, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_ENV\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_ENV value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_duid(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_DUID, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_DUID\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_DUID value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_auth_type(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE value is empty\n");
        return -1;
    }
    return 0;
}

int platform_set_dtg(char *string) {
    int status;
    status = da16x_set_config_str(DA16X_CONF_STR_IOTCONNECT_DTG, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to write DA16X_CONF_STR_IOTCONNECT_DTG\n");
        return -1;
    }
    return 0;
}

int platform_get_dtg(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_DTG, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_DTG\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_DTG value is empty\n");
        return -1;
    }
    return 0;
}

int platform_get_symmetric_key(char *string) {
    int status;
    *string = '\0';
    status = da16x_get_config_str(DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY, string);
    if(status != CC_SUCCESS)
    {
        MQTT_ERROR("Failed to get DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY\n");
        return -1;
    }
    if(*string == '\0')
    {
        MQTT_ERROR("DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY value is empty\n");
        return -1;
    }
    return 0;
}

void mqtt_sample_client_nvram_config(const char *broker,
		              int port,
			      const char *username,
			      const char *password,
			      const char *clientid, 
			      const char *pub,
			      const char *sub,
                              int qos)
{
    int running_state = mqtt_client_is_running();

    // stop any MQTT client that might be running with "stale" info
    if (running_state == TRUE) {
        mqtt_client_force_stop();
        mqtt_client_stop();
    }

    set_nvcache_int(DA16X_CONF_INT_SNTP_CLIENT, 1);

    /*
     * I think DA16X_CONF_INT_MQTT_AUTO will get modified by mqtt_client_start()
     */
    set_nvcache_str(DA16X_CONF_STR_MQTT_BROKER_IP,     broker);
    set_nvcache_int(DA16X_CONF_INT_MQTT_PORT,          port);
    set_nvcache_str(DA16X_CONF_STR_MQTT_USERNAME,      username);
    /*
     * password can be NULL
     */
    set_nvcache_str(DA16X_CONF_STR_MQTT_PASSWORD,      password);
    set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, clientid);

    set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_TOPIC,     pub);
    // set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_CID,       clientid);
    set_nvcache_int(DA16X_CONF_INT_MQTT_QOS,           qos);
    set_nvcache_int(DA16X_CONF_INT_MQTT_TLS,           1);
    set_nvcache_int(DA16X_CONF_INT_MQTT_PING_PERIOD,   60);
    set_nvcache_int(DA16X_CONF_INT_MQTT_VER311,        1);

    //
    // Generally "nuke" any subscriptions.
    //
    // try to delete any "top" topic - may fail
    delete_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC);
    int ret_num;
    if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &ret_num) == 0)
    {
        // MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM exists, so delete "sub" topics
        for (int i = 0; i < ret_num; i++)
        {
            char topics[16] = {0, };
            sprintf(topics, "%s%d", MQTT_NVRAM_CONFIG_SUB_TOPIC, i);
            delete_nvram_env(topics);
        }
    }
    delete_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM);

    // add new subscription
    //
    // can't use set_nvcache_str() as DA16X_CONF_STR_MQTT_SUB_TOPIC_ADD doesn't work to read 
    //
    if(da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC_ADD, (char *) sub) != CC_SUCCESS) {
        MQTT_ERROR("da16x_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_TOPIC_ADD, %s) failed\n", sub);
    }

    // save nvcache values
    da16x_nvcache2flash();

    if (running_state == TRUE) {
        mqtt_client_start();
        
        // my app terminates if mqtt connection is not made within the specified time.
        if (_mqtt_chk_connection(100) == pdFALSE) {
            MQTT_ERROR("[%s] _mqtt_chk_connection failed\n", __func__);
        }
    }
}

// Need certificate for MQTTS to talk to the broker
//
// I guess we could update SFLASH_ROOT_CA_ADDR1 in case connection doesn't work and try a different root CA, since Azure has 3 potential certs?
//
void mqtt_broker_cert_config(const char *root_ca, unsigned int root_ca_len)
{
    cert_flash_write(SFLASH_ROOT_CA_ADDR1, (char *) root_ca, root_ca_len);
}

/*
 * device_cert / device_private_key can be NULL
 */
void mqtt_device_cert_config(const char *device_cert, unsigned int device_cert_len,
			     const char *device_private_key, unsigned int device_private_key_len)
{
    if(device_cert && device_private_key) {
        cert_flash_write(SFLASH_CERTIFICATE_ADDR1, (char *) device_cert, device_cert_len);
        cert_flash_write(SFLASH_PRIVATE_KEY_ADDR1, (char *) device_private_key, device_private_key_len);
    } else {
        cert_flash_delete(SFLASH_CERTIFICATE_ADDR1);
        cert_flash_delete(SFLASH_PRIVATE_KEY_ADDR1);
    }
}

#endif	// (__MQTT_CLIENT_SAMPLE__)

/* EOF */
