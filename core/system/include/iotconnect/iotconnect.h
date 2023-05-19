//
// Copyright: Avnet, Softweb Inc. 2020
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//

#ifndef IOTCONNECT_H
#define IOTCONNECT_H

#include <stddef.h>
#include <time.h>
#include "iotconnect_event.h"
#include "iotconnect_telemetry.h"
#include "iotconnect_lib.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    IOTC_CS_UNDEFINED,
    IOTC_CS_MQTT_CONNECTED,
    IOTC_CS_MQTT_DISCONNECTED
} IotConnectConnectionStatus;

typedef void (*IotConnectStatusCallback)(IotConnectConnectionStatus data);

typedef struct {
    char *env;    // Settings -> Key Vault -> CPID.
    char *cpid;   // Settings -> Key Vault -> Evnironment.
    char *duid;   // Name of the device.
    int qos; // QOS for outbound messages. Default 1.
    IotclOtaCallback ota_cb; // callback for OTA events.
    IotclCommandCallback cmd_cb; // callback for command events.
    IotclMessageCallback msg_cb; // callback for ALL messages, including the specific ones like cmd or ota callback.
    IotConnectStatusCallback status_cb; // callback for connection status
} IotConnectClientConfig;


IotConnectClientConfig *iotconnect_sdk_init_and_get_config(void);

// call iotconnect_sdk_init_and_get_config first and configure the SDK before calling iotconnect_sdk_init()
int iotconnect_sdk_init(void);

bool iotconnect_sdk_is_connected(void);

// Will check if there are inbound messages and call adequate callbacks if there are any
// This is technically not required for the Paho implementation.
void iotconnect_sdk_receive(void);

// blocks until sent and returns 0 if successful.
// data is a null-terminated string
int iotconnect_sdk_send_packet(const char *data);

void iotconnect_sdk_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif
