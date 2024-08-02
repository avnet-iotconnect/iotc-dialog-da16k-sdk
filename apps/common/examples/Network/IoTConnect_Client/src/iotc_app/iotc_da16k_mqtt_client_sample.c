/**
 ****************************************************************************************
 *
 * @file iotc_da16k_mqtt_client
 *
 * @brief MQTT Client, based on MQTT Client Sample Application
 *
 * Based on mqtt_client_sample.c from 
 * DA16200_DA16600_SDK_FreeRTOS_v3.2.7.1 in apps/common/examples/Network/Http_Client/src
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
#include "task.h"
#include "net_bsd_sockets.h"
#include "da16x_network_common.h"
#include "user_dpm.h"
#include "user_dpm_api.h"
#include "common_def.h"
#include "command_net.h"
#include "mqtt_client.h"
#include "util_api.h"
#include "da16x_cert.h"

#include "iotc_da16k_mqtt_client_sample.h"
#include "iotc_da16k_util.h"

#define	SAMPLE_MQTT_CLIENT		"MQTT_CLIENT"
#define	NAME_MY_APP_EVENT_HANDLER	"MY_APP_EVENT_HANDLER"
#define	NAME_MY_APP_Q_HANDLER		"MY_APP_Q_HANDLER"

/// Wait count for mqtt send: in multiple of 100 ms
#define MQTT_PUB_MAX_WAIT_CNT    100

/// The Message Queue size of my application queue
#define MSG_Q_SIZE               5

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

/* For the message queues. Items enqueued contain a topic string and a message string. */

typedef struct {
    const char *topic;
    const char *message;
} MqttQueueItem;

static BaseType_t _mqtt_send_to_q(const char *topic, const char *message, bool copy) {
    BaseType_t status;
    MqttQueueItem item = { NULL, NULL };

    if (!_mqtt_q) {
        MQTT_ERROR("[%s] Msg Q doesn't exist!", __func__);
	return -1;
    }

    if(copy) {
        /* Take a copy of the message, as the original may be freed before the Q entry is dealt with? */
        item.topic = strdup(topic);
        item.message = strdup(message);
    } else {
        // assume buf can use free() on buf once it has been sent - original argument should not be deallocated
        item.topic = topic;
        item.message = message;        
    }

    if (!item.topic || !item.message) {
        MQTT_ERROR("[%s] failed to allocate topic or message string!", __func__);
    	return -1;
    }

    MQTT_ERROR("[%s] topic %s, message %s!", __func__, item.topic ? item.topic : "NULL", item.message ? item.message : "NULL");
    tx_publish++;

    status = xQueueSendToBack(_mqtt_q, &item, 10);

    if(status != pdTRUE) {
        MQTT_ERROR("[%s] failed to enqueue message!", __func__);
        if (copy) {
            free((void*) item.topic);
            free((void*) item.message);
        }
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief mqtt_client sample callback function for processing PUBLISH messages \n
 * Users register a callback function to process a PUBLISH message. \n
 * @param[in] buf the message paylod
 * @param[in] len the message paylod length
 * @param[in] topic the topic this mqtt_client subscribed to
 ****************************************************************************************
 */
static void _mqtt_msg_cb(const char *buf, int len, const char *topic) {
    MQTT_DEBUG("\n\n[MQTT_SAMPLE] Msg Recv: topic=%s, msg=%s, len = %d\n\n", topic, buf, len);

    if(g_msg_cb) {
        g_msg_cb(buf, len, topic);
    }
}

static void _mqtt_pub_cb(int mid) {
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

static void _mqtt_conn_cb(void) {
    MQTT_DEBUG("\n\n[%s] MQTT connection callback!\n\n", __func__);

    if(g_conn_cb) {
        g_conn_cb();
    }
}

static void _mqtt_disconn_cb(void) {
    MQTT_DEBUG("\n\n[%s] MQTT disconnection callback!\n\n", __func__);

    if(g_disconn_cb) {
        g_disconn_cb();
    }
}

static void _mqtt_sub_cb(void) {
    MQTT_DEBUG("\n\n[%s] topic subscribed!\n\n", __func__);

    if(g_sub_cb) {
        g_sub_cb();
    }
}

static void _mqtt_unsub_cb(void) {
    MQTT_DEBUG("\n\n[%s] topic unsubscribed!\n\n", __func__);

    if(g_unsub_cb) {
        g_unsub_cb();
    }
}

int mqtt_sample_client_send(const char *topic, const char *message, bool copy) {
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

    if ((ret = _mqtt_send_to_q(topic, message, copy)) != pdPASS ) {
        MQTT_ERROR("[%s] Failed to add a message to Q (%d)\r\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int _mqtt_pub_msg(const char *topic, const char *buffer) {
    int ret;
#if !defined (USE_MQTT_SEND_WITH_QOS_API)
    int wait_cnt = 0;

LBL_SEND_RETRY:
    ret = mqtt_client_send_message((char *) topic, (char *) buffer);
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
    ret = mqtt_client_send_message_with_qos((char *) topic, (char *) buffer, MQTT_PUB_MAX_WAIT_CNT);
    if (ret != 0) {
        if (ret == -2) {
            MQTT_ERROR("[MQTT_SAMPLE] Mqtt send not successfully delivered, timtout=%d\n", MQTT_PUB_MAX_WAIT_CNT);
        }
    }
#endif
    return ret;
}

static void _mqtt_q_handler(void *arg) {
    DA16X_UNUSED_ARG(arg);

    int ret;
    MqttQueueItem item = { NULL, NULL };
    BaseType_t xStatus;

    // message queue init
    while (1) {
        if (!_mqtt_q) {
            MQTT_ERROR("[%s] Msg Q doesn't exist!", __func__);
    	    break;
        }

        xStatus = xQueueReceive(_mqtt_q, &item, portMAX_DELAY);
        if (xStatus != pdPASS) {
            MQTT_ERROR("[%s] Q recv error (%d)\r\n", __func__, xStatus);
            vTaskDelete(NULL);
            break;
        }

        if (item.topic && item.message) {
            ret = _mqtt_pub_msg(item.topic, item.message);
            if (ret != 0) {
#if !defined (USE_MQTT_SEND_WITH_QOS_API)
                MQTT_ERROR("[MQTT_SAMPLE] Sending a message failed, refer to mqtt_client_send_message()\n");
#else
                MQTT_ERROR("[MQTT_SAMPLE] Sending a message failed, refer to mqtt_client_send_message_with_qos()\n");
#endif
            }
        } else {
            MQTT_ERROR("[%s] Queue message or topic is NULL!\r\n", __func__);
        }

        // allocated in _mqtt_send_to_q()
        free((void*) item.topic);
        free((void*) item.message);
    }

    // shouldn't get here -- mqtt_sample_deinit() should kill the task
    vTaskDelete(NULL);
    return;
}

void mqtt_sample_client_deinit(void) {
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

static int _mqtt_chk_connection(int timeout) {
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
int mqtt_sample_client_init(msg_cb_t msg_cb, pub_cb_t pub_cb, conn_cb_t conn_cb, disconn_cb_t disconn_cb, sub_cb_t sub_cb, unsub_cb_t unsub_cb) {
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
    _mqtt_q = xQueueCreate(MSG_Q_SIZE, sizeof(MqttQueueItem ));
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

    iotc_da16k_set_nvcache_int(DA16X_CONF_INT_SNTP_CLIENT, 1);

    /*
     * I think DA16X_CONF_INT_MQTT_AUTO will get modified by mqtt_client_start()
     */
    iotc_da16k_set_nvcache_str(DA16X_CONF_STR_MQTT_BROKER_IP,     broker);
    iotc_da16k_set_nvcache_int(DA16X_CONF_INT_MQTT_PORT,          port);
    iotc_da16k_set_nvcache_str(DA16X_CONF_STR_MQTT_USERNAME,      username);
    /*
     * password can be NULL
     */
    iotc_da16k_set_nvcache_str(DA16X_CONF_STR_MQTT_PASSWORD,      password);
    iotc_da16k_set_nvcache_str(DA16X_CONF_STR_MQTT_SUB_CLIENT_ID, clientid);

    iotc_da16k_set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_TOPIC,     pub);
    // set_nvcache_str(DA16X_CONF_STR_MQTT_PUB_CID,       clientid);
    iotc_da16k_set_nvcache_int(DA16X_CONF_INT_MQTT_QOS,           qos);
    iotc_da16k_set_nvcache_int(DA16X_CONF_INT_MQTT_TLS,           1);
    iotc_da16k_set_nvcache_int(DA16X_CONF_INT_MQTT_PING_PERIOD,   60);
    iotc_da16k_set_nvcache_int(DA16X_CONF_INT_MQTT_VER311,        1);

    //
    // Generally "nuke" any subscriptions.
    //
    // try to delete any "top" topic - may fail
    delete_nvram_env(MQTT_NVRAM_CONFIG_SUB_TOPIC);
    int ret_num;
    if (read_nvram_int(MQTT_NVRAM_CONFIG_SUB_TOPIC_NUM, &ret_num) == 0) {
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

#endif    // (__MQTT_CLIENT_SAMPLE__)

/* EOF */
