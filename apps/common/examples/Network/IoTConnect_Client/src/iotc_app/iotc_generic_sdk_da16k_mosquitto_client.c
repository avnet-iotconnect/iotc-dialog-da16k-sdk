/* SPDX-License-Identifier: MIT
 * Copyright (C) 2020-2024 Avnet
 * Authors: Neil Matthews <nmatthews@witekio.com> et al.
 */

/*
 * This implements the functionality required by iotc_device_client.h in the Generic C SDK.
 *
 * In the actual SDK this is implemented with Paho. We use the DA16K Mosquitto implementation.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common_def.h"

#include "iotc_da16k_mqtt_client_sample.h"
#include "iotc_device_client.h"
#include "iotc_algorithms.h"
#include "iotc_log.h"

#ifndef MQTT_PUBLISH_TIMEOUT_MS
#define MQTT_PUBLISH_TIMEOUT_MS     10000L
#endif

static bool is_initialized = false;
static bool is_connected = false;
static IotConnectC2dCallback c2d_msg_cb = NULL; // callback for inbound messages
static IotConnectMqttStatusCallback status_cb = NULL; // callback for connection status


static void iotc_device_deinit(void) {
    mqtt_sample_client_deinit();

    status_cb = NULL;
    c2d_msg_cb = NULL;
    is_connected = false;
    is_initialized = false;
}

static void __mqtt_on_message(const char *buf, int len, const char *topic)
{
    IOTC_INFO("[%s] Msg Recv: topic=%s, msg=%s, len = %d\n\n" CLEAR_COLOR, __func__, topic, buf, len);

    if (c2d_msg_cb) {
        c2d_msg_cb( (const unsigned char*) buf, (size_t) len);
    } else {
        IOTC_INFO("%s: c2d_msg_cb == NULL\n" CLEAR_COLOR, __func__);
    }
}

static void __mqtt_on_disconnected(void)
{
    IOTC_INFO("[%s] MQTT disconnection callback!\n\n" CLEAR_COLOR, __func__);

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_DISCONNECTED);
    }
    iotc_device_deinit();
}

int iotc_device_client_disconnect(void) {
    mqtt_sample_client_deinit();
    return 0;
}

void iotc_device_client_receive(void) {
    ; // do nothing
}

bool iotc_device_client_is_connected(void) {
    if (!is_initialized) {
        return false;
    }

    return is_connected;
}

int iotc_device_client_send_message_qos(const char* topic, const char *message, int qos) {
    IOTC_WARN("%s: NOT IMPLEMENTED, defaulting to non-QOS\n\n", __func__);
    // maybe we can implement by setting in NVRAM?
    // but really should be kept with the message in the message Q
    (void) qos;
    return mqtt_sample_client_send(topic, message, true);
}

int iotc_device_client_send_message(const char *topic, const char *message) {
    return mqtt_sample_client_send(topic, message, true);
}

static void __mqtt_on_publish(int mid)
{
    IOTC_INFO("[%s] message id %d has been published!\n\n" CLEAR_COLOR, __func__, mid);
}

static void __mqtt_on_connected(void)
{
    IOTC_INFO("[%s] MQTT connection callback!\n\n" CLEAR_COLOR, __func__);

    is_connected = true;
}

static void __mqtt_on_subscribe(void)
{
    IOTC_INFO("[%s] topic subscribed!\n\n" CLEAR_COLOR, __func__);

    // signal IOTC_CS_MQTT_CONNECTED when broker is connected *and* the subscription succeeded
    if (is_connected == false) {
        IOTC_ERROR("[%s] topic subscribed but not connected?\n\n" CLEAR_COLOR, __func__);
    }

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_CONNECTED);
    } else {
        IOTC_INFO("%s: status_cb == NULL\n" CLEAR_COLOR, __func__);
    }
}

static void __mqtt_on_unsubscribe(void)
{
    IOTC_ERROR("\n\n[%s] topic unsubscribed unexpectedly!\n\n" CLEAR_COLOR, __func__);
}

// setup the MQTT client configuration

int iotc_device_client_connect(IotConnectDeviceClientConfig *c) {
    
    IotclMqttConfig *mc = iotcl_mqtt_get_config();
    if (!mc) {
        return IOTCL_ERR_CONFIG_MISSING; // caled function will print the error
    }

    mqtt_sample_client_nvram_config(mc->host,
            8883,
            mc->username,
            "dummy",
            mc->client_id,
            mc->pub_rpt,
            mc->sub_c2d,
            c->qos);

    // Don't handle certificates -- assume that there are AT commands to set them in a standard way.

    int ret;

    // Set the callbacks early
    c2d_msg_cb = c->c2d_msg_cb;
    if(c2d_msg_cb == NULL) {
        IOTC_INFO("%s: c2d_msg_cb == NULL\n" CLEAR_COLOR, __func__);
    }

    status_cb = c->status_cb;
    if(status_cb == NULL) {
        IOTC_INFO("%s: status_cb == NULL\n" CLEAR_COLOR, __func__);
    }

    ret = mqtt_sample_client_init(__mqtt_on_message, __mqtt_on_publish, __mqtt_on_connected, __mqtt_on_disconnected, __mqtt_on_subscribe, __mqtt_on_unsubscribe);
    if (ret != 0) {
        IOTC_ERROR("[%s] mqtt_sample_client_init() failed\n", __func__);
        goto cleanup;
    }

    is_initialized = true;

    return 0;

cleanup:
    iotc_device_deinit();
    return -1;
}
