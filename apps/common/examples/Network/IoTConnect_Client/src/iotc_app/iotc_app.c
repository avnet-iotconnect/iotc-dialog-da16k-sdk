//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotcl.h"
#include "iotconnect.h"
#include "iotc_da16k_util.h"

#include "mqtt_client.h"
#include "iotc_app.h"
#include "iotc_log.h"

#include "atcmd.h"

#define EVT_IOTC_STOP      (1UL << 0x00)
#define EVT_IOTC_SETUP     (1UL << 0x01)
#define EVT_IOTC_START     (1UL << 0x02)
#define EVT_IOTC_RESET     (1UL << 0x03)

#define COMMAND_QUEUE_SIZE (16)

static EventGroupHandle_t   my_app_event_group  = NULL;
static QueueHandle_t        command_queue       = NULL;

static IotConnectClientConfig s_client_cfg = {0};

static bool setupOK = false;

static err_t network_ok(void) {
    int iface = WLAN0_IFACE;
    int wait_cnt = 0;

    while (chk_network_ready(iface) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wait_cnt++;

        if (wait_cnt == 100) {
            IOTC_ERROR("\r\n [%s] ERR : No network connection\r\n", __func__);
            return ERR_UNKNOWN;
        }
    }

    /*
     * (Mostly) borrowed from MQTT client sample code
     */
    if (!dpm_mode_is_wakeup()) {
        int ret;

        // Non-DPM or DPM POR ...

        // check that SNTP has sync'd successfully.
        ret = sntp_wait_sync(10);
        if (ret != 0) {
            if (ret == -1) { // timeout
                IOTC_ERROR("SNTP sync timed out - check Internet conenction. Reboot to start over \n");
            } else {
                if (ret == -2) { // timeout
                	IOTC_WARN("SNTP was disabled. Reboot to start over \n");
    
                    da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, 1);
                }
            }
            return ERR_UNKNOWN;
       }
   }
    
    return ERR_OK;
}

void stop_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_STOP);
}

void setup_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_SETUP);
}

void start_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_START);
}

void reset_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_RESET);
}

static void on_connection_status(IotConnectMqttStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            IOTC_INFO("IoTConnect Client Connected\n");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            IOTC_INFO("IoTConnect Client Disconnected\n");
            break;
        default:
            IOTC_INFO("IoTConnect Client ERROR\n");
            break;
    }

    //
    // reflect the state of the IoTConnect MQTT connection
    //
    atcmd_asynchony_event_for_icmqtt(status == IOTC_CS_MQTT_CONNECTED ? 1 : 0);
}


/* Deallocate command queue item including strings *and* structure itself */
void iotc_command_queue_item_destroy(iotc_command_queue_item_t *item) {
    if (item) {
        free((void*)item->command);
        free((void*)item->ack_id);
    }
    free(item);
}

/* Allocate command queue item. Duplicates strings. Needs to be free'd with command_queue_item_destroy later. */
static iotc_command_queue_item_t *iotc_command_queue_item_create(const char *command, const char *ack_id) {
    iotc_command_queue_item_t * item = calloc(1, sizeof(iotc_command_queue_item_t));

    if (item == NULL) {
        IOTC_ERROR("Could not allocate command queue item!");
        return NULL;
    }

    item->command = strdup(command);
    
    if (ack_id) {
        item->ack_id = strdup(ack_id);
    }
    
    if (item->command == NULL || (ack_id && (item->ack_id == NULL))) {
        IOTC_ERROR("String allocation error (out of memory?)\n");
        iotc_command_queue_item_destroy(item);
        return NULL;
    }

    return item;
}

static void on_command_receive(IotclC2dEventData data) {
    const char                  *command        = iotcl_c2d_get_command(data);
    const char                  *ack_id         = iotcl_c2d_get_ack_id(data);
    iotc_command_queue_item_t   *queue_entry    = NULL;

    if (command) {
        IOTC_INFO("Command %s received with %s ACK ID\n", command, ack_id ? ack_id : "no");

        queue_entry = iotc_command_queue_item_create(command, ack_id);

        if (queue_entry == NULL) {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
            return;
        }

        if (pdTRUE == xQueueSendToBack(command_queue, &queue_entry, 0)) {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_SUCCESS, "Command added to local queue");
        } else {
            IOTC_ERROR("Command queue full!\n");    
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Command queue full");
            iotc_command_queue_item_destroy(queue_entry);
        }        
    } else {
        IOTC_ERROR("Failed to parse command\n");
        iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
    }
}

iotc_command_queue_item_t *iotc_command_queue_item_get() {
    iotc_command_queue_item_t * ret = NULL;

    if (command_queue == NULL) {
        IOTC_ERROR("Queue does not exist!\n");
        return NULL;
    }

    if (pdTRUE == xQueueReceive(command_queue, &ret, 0)) {
        return ret;
    } else {
        return NULL;    // Empty queue
    }
}

//
// a wrapper to send +NWICSETUPBEGIN / +NWICSETUPEND messages
//
int setup_wrapper(void) {
    atcmd_asynchony_event_for_icsetup_begin();

    iotconnect_sdk_init_config(&s_client_cfg);

    if (iotc_da16k_read_config(&s_client_cfg) != 0) {
        IOTC_ERROR("Failed to get configuration values from nvram\n");
        goto cleanup;
    }

    IOTC_INFO("IOTC_ENV = %s\n", s_client_cfg.env);
    IOTC_INFO("IOTC_CPID = %s\n", s_client_cfg.cpid);
    IOTC_INFO("IOTC_DUID = %s\n", s_client_cfg.duid);
    IOTC_INFO("IOTC_AUTH_TYPE = %d\n", s_client_cfg.auth_info.type);
    IOTC_INFO("IOTC_AUTH_SYMMETRIC_KEY = %s\n", s_client_cfg.auth_info.data.symmetric_key ? s_client_cfg.auth_info.data.symmetric_key : "(null)");

    s_client_cfg.status_cb = on_connection_status;
    s_client_cfg.ota_cb = NULL;
    s_client_cfg.cmd_cb = on_command_receive;

    atcmd_asynchony_event_for_icsetup_end(true);
    return 0;

cleanup:
    iotc_da16k_reset_config(&s_client_cfg);
    atcmd_asynchony_event_for_icsetup_end(false);
    return -1;
}

//
// a wrapper to send +NWICSTARTBEGIN / +NWICSTARTEND messages
//
int start_wrapper()
{
    int ret = -1;

    atcmd_asynchony_event_for_icstart_begin();

    //
    // Try a small number of times - in case of an intermitent failure
    //
    for(int i = 0; i < 5; i++) {
        //
        // Run discovery/sync
        // Startup mqtt_client with new MQTT_XXXX values
        //
        ret = iotconnect_sdk_init(&s_client_cfg);
        if (ret == 0) {
            ret = iotconnect_sdk_connect();
            if (ret == 0) {
                break;
            } else {
                IOTC_ERROR("iotconnect_sdk_connect failed: %d", ret);
            }
        } else {            
            IOTC_ERROR("iotconnect_sdk_init failed: %d", ret);
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    if (ret != 0) {
        IOTC_ERROR("start_wrapper() failed: %d", ret);
    }

    atcmd_asynchony_event_for_icstart_end(ret == 0);
    return ret;
}

//
// a wrapper to send +NWICSTOPBEGIN / +NWICSTOPEND messages
//
static void stop_wrapper(void)
{
    atcmd_asynchony_event_for_icstop_begin();
    if(iotconnect_sdk_is_connected() == true) {
        iotconnect_sdk_disconnect();
    }
    atcmd_asynchony_event_for_icstop_end(true);
}

//
// a wrapper to send +NWICRESETBEGIN / +NWICRESETEND messages
//
static void reset_wrapper(void) {
    atcmd_asynchony_event_for_icreset_begin();
        
    if(iotconnect_sdk_is_connected() == true) {
        iotconnect_sdk_disconnect();
    }

    // This deinits all the allocated data in the config struct
    iotc_da16k_reset_config(&s_client_cfg);
    
    atcmd_asynchony_event_for_icreset_end(true);
}

//
// This is a cutdown version that monitors the DA16X_CONF_STR_IOTCONNECT_SYNC environment variable
// which is set by AT+NWICSYNC command
// 
// When the value changes to 1 it will perform IoTConnect discovery and write NVRAM values, the MQTT client
// will be restarted and the new values from IoTConnect discovery should be used for MQTT.
//
// So sequence of events is:
// - set IoTConnect NVRAM values
// - set DA16X_CONF_STR_IOTCONNECT_SYNC to 1
// - ensure MQTT client is running
//
//
// This relies on the standard certificate mechanisms built into the DA16200
// 
// AT commands to set the HTTP / MQQT certificates [and, if required, to set the device certificate / private key] need to be sent.
//
int iotconnect_basic_sample_main(void) {
    IOTC_WARN("\n\n\nRunning in AT command mode\n\n\n");

    /* Create queue to store commands in */
    command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(iotc_command_queue_item_t*));
    if (command_queue == NULL) {
        IOTC_ERROR("[%s] Command Queue Create Error!", __func__);
        goto cleanup;
    }

    /*
     * Explicity *NOT* setting any certificates for "AT command" version
     *
     * Certificates can be set see: Table 16, Page 50, "UM-WI-003 DA16200 DA16600 Host Interface and AT Command User Manual" 
     */

    my_app_event_group = xEventGroupCreate();
    if (my_app_event_group == NULL) {
        IOTC_ERROR("[%s] Event group Create Error!", __func__);
        goto cleanup;
    }

    /* clear wait bits here */
    xEventGroupClearBits( my_app_event_group, (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET) );

    //
    // Initially run the setup and start iotconnect
    //
    xEventGroupSetBits(my_app_event_group, (EVT_IOTC_SETUP | EVT_IOTC_START));

    while (1) {
        EventBits_t events = xEventGroupWaitBits(my_app_event_group,
                                     (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET),
                                     pdFALSE,
                                     pdFALSE,
                                     30000 / portTICK_PERIOD_MS);

        bool reset = events & EVT_IOTC_RESET;
        bool start = events & EVT_IOTC_START;
        bool setup = events & EVT_IOTC_SETUP;
        bool stop = events & EVT_IOTC_STOP;

        xEventGroupClearBits( my_app_event_group, (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET) );

        if(start == false && setup == false && stop == false && reset == false) {
            // just timed out
            continue;
        }

        if(network_ok() != ERR_OK) {
        	IOTC_ERROR("Command ignored - Network interface is down!");
            continue;
        }

        //
        // disconnect from iotconnect, 
        // teardown the current config and deallocate any memory
        //
        if(reset) {
            reset_wrapper();
            setupOK = false;
            continue;
        }

        //
        // disconnect from iotconnect, but leave the config intact
        //
        if(stop) {
            stop_wrapper();
            continue;
        }

        if(setup) {
            setupOK = (0 == setup_wrapper());
        }

        if(start) {
            if (setupOK == false) {
                IOTC_ERROR("Cannot start wrapper - config setup failed.\n");
                IOTC_INFO("Did you run: iotconnect_client setup?\n");
            } else {
                start_wrapper();
            }
        }
    }

cleanup:

    if (command_queue) {
        vQueueDelete(command_queue);
        command_queue = NULL;
    }

    if(my_app_event_group) {
        vEventGroupDelete(my_app_event_group);
        my_app_event_group = NULL;
    }
    
    iotc_da16k_reset_config(&s_client_cfg);
    
    IOTC_INFO("exiting basic_sample()\n" );
    return 0;
}



/*

TODO FIXME Reimplement for new iotc-c-lib

This is old code to handle CMD/OTA ack. Since they're currently unsupported, they must be re-implemented for the new c-library at some point.

static void on_command(IotclEventData data) {
    int iotc_cmdack;
    platform_get_iotconnect_use_cmdack(&iotc_cmdack);

    IOTC_INFO("%s\n", __func__);

    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        IOTC_ERROR("%s: ack_id == NULL\n", __func__);
        goto cleanup;
    }

    if(iotc_cmdack != 1) {
        IOTC_INFO("iotc_cmdack %d -- ignoring command\n", iotc_cmdack);

        //
        // If DA16X_CONF_STR_MQTT_IOTCONNECT_USE_CMD_ACK is not 1 a validly treated command will immediately respond false
        //
        iotconnect_command_status(type, ack_id, false, "Not implemented");
        goto cleanup_ack_id;
    }

    //
    // When DA16X_CONF_STR_MQTT_IOTCONNECT_USE_CMD_ACK is 1, the response to a validly treated command is at the discretion of the AT user.
    // Simply provide information.
    //
    char *command = iotcl_clone_command(data);
    if (NULL == command) {
        IOTC_ERROR("%s: command == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");

        atcmd_asynchony_event_for_iccmd(UNKNOWN_EVENT, NULL, NULL);
        goto cleanup_ack_id;
    }
    
    // send command asynchronously here?
    // replace any spaces with a comma
    for(unsigned int i = 0;i < strlen(command);i++) {
        command[i] = (command[i] == ' ') ? ',' : command[i];
    }

    IOTC_INFO("\n\n%s type: %d\n", __func__, type);
    IOTC_INFO("%s ack_id: %s\n", __func__, ack_id);
    IOTC_INFO("%s command: %s\n\n", __func__, command);
    IOTC_INFO("iotconnect_client cmd_ack %d %s 1 \"message\" - if ok\n", type, ack_id);
    IOTC_INFO("iotconnect_client cmd_ack %d %s 0 \"message\" - if failed\n\n", type, ack_id);

    atcmd_asynchony_event_for_iccmd(type, ack_id, command);
    free((void *) command);

cleanup_ack_id:
    free((void *) ack_id);
cleanup:
    iotcl_destroy_event(data);
}*/

/*
static void on_ota(IotclEventData data) {
    int iotc_otaack;
    platform_get_iotconnect_use_otaack(&iotc_otaack);

    IotConnectEventType type = iotcl_get_event_type(data);
    char *ack_id = iotcl_clone_ack_id(data);
    if(NULL == ack_id) {
        IOTC_ERROR("%s: ack_id == NULL\n", __func__);
        goto cleanup;
    }

    if(iotc_otaack != 1) {
        IOTC_INFO("iotc_otaack %d -- ignoring command\n", iotc_otaack);

        //
        // If DA16X_CONF_STR_MQTT_IOTCONNECT_USE_OTA_ACK is not 1 a validly treated OTA message will immediately respond false
        //
        iotconnect_command_status(type, ack_id, false, "Not implemented");
        goto cleanup_ack_id;
    }

    //
    // When DA16X_CONF_STR_MQTT_IOTCONNECT_USE_OTA_ACK is 1, the response to a validly treated OTA message is at the discretion of the AT user.
    // Simply provide information.
    //
    const char *version = iotcl_clone_sw_version(data);
    if (NULL == version) {
        IOTC_ERROR("%s: version == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");

        atcmd_asynchony_event_for_icota(NULL, NULL, NULL);
        goto cleanup_ack_id;
    }

    const char *url = iotcl_clone_download_url(data, 0);
    if (NULL == url) {
        // FIXME TODO original code had comments that "url" couuld be null, and somehoe inside a "data" item instead does that need more implementation here?


        IOTC_ERROR("%s: url == NULL\n", __func__);
        iotconnect_command_status(type, ack_id, false, "Internal error");

        atcmd_asynchony_event_for_icota(NULL, NULL, NULL);
        goto cleanup_version;
    }

    IOTC_INFO("\n\n%s ack_id: %s\n", __func__, ack_id);
    IOTC_INFO("%s version: %s\n", __func__, version);
    IOTC_INFO("%s url: %s\n\n", __func__, url);
    IOTC_INFO("iotconnect_client ota_ack %s 1 \"message\" - if ok\n", ack_id);
    IOTC_INFO("iotconnect_client ota_ack %s 0 \"message\" - if failed\n\n", ack_id);

    atcmd_asynchony_event_for_icota(ack_id, version, url);

    free((void *) url);
cleanup_version:
    free((void *) version);
cleanup_ack_id:
    free((void *) ack_id);
cleanup:
    iotcl_destroy_event(data);
}
*/
