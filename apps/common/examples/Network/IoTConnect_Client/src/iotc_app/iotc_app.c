//
// Copyright: Avnet 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 6/24/21.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdk_type.h"
#include "da16x_network_common.h"
#include "util_api.h"

#include "iotcl.h"
#include "iotcl_util.h"
#include "iotconnect.h"
#include "iotc_da16k_util.h"

#include "mqtt_client.h"
#include "iotc_app.h"
#include "iotc_log.h"
#include "iotc_da16k_dynamic_ca.h"

#include "atcmd_iotc.h"

#define EVT_IOTC_STOP      		(1UL << 0x00)
#define EVT_IOTC_SETUP     		(1UL << 0x01)
#define EVT_IOTC_START     		(1UL << 0x02)
#define EVT_IOTC_RESET     		(1UL << 0x03)
#define EVT_IOTC_DISCONNECT		(1UL << 0x04)

#define COMMAND_QUEUE_SIZE (16)

static EventGroupHandle_t   my_app_event_group  = NULL;
static QueueHandle_t        command_queue       = NULL;

static IotConnectClientConfig s_client_cfg = {0};

static bool is_setup_ok	   = false;		// configuration loaded from NVRAM
static bool is_initialized = false;		// SDK is initialized and it ran HTTP discovery/identity successfully.
static bool is_autoconnect = false;		// If start() is successful, autoconnect will be on by default unless stop() is called.

// AWS Qualification ---
// we use this as a flag AND keep the string forever until restart
#ifndef AWS_QALIFICATION_HOST // define this at compile time if you want to force this qualification mode
static char* aws_qualification_host = NULL;
#else
static char* aws_qualification_host = AWS_QALIFICATION_HOST;
#endif

// AWS Qualification ---
// when we connected successfully last. Some qualification tests are
// not robust enough and we need to force a reconnect after staying connected too long.
static TickType_t 			last_connect_time = 0;

static bool network_ok(void) {
    int iface = WLAN0_IFACE;
    int wait_cnt = 0;

    while (chk_network_ready(iface) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wait_cnt++;

        if (wait_cnt == 100) {
            IOTC_ERROR("[%s] ERR : No network connection", __func__);
            return false;
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
                IOTC_ERROR("SNTP sync timed out - check Internet connection. Reboot to start over.");
            } else {
                if (ret == -2) { // timeout
                    IOTC_WARN("SNTP was disabled. Reboot to start over.");
    
                    da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, 1);
                }
            }
            return false;
       }
   }
    
    return true;
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

void disconnect_iotconnect(void) {
    if(my_app_event_group == NULL) {
        return;
    }
    xEventGroupSetBits(my_app_event_group, EVT_IOTC_DISCONNECT);
}

static void on_connection_status(IotConnectMqttStatus status) {
    // Add your own status handling
    switch (status) {
        case IOTC_CS_MQTT_CONNECTED:
            IOTC_INFO("IoTConnect Client Connected");
            break;
        case IOTC_CS_MQTT_DISCONNECTED:
            IOTC_INFO("IoTConnect Client Disconnected");
            break;
        default:
            IOTC_INFO("IoTConnect Client ERROR");
            break;
    }

    //
    // reflect the state of the IoTConnect MQTT connection
    //
    atcmd_asynchony_event_for_icmqtt(status == IOTC_CS_MQTT_CONNECTED ? 1 : 0);
}


/* Deallocate command queue item's strings */
void iotc_command_queue_item_destroy(iotc_command_queue_item item) {
    free((void*)item.command);
    free((void*)item.ack_id);
}

// call this function before attempting to reconnect
static void set_up_qualilification_mode(const char * host) {
    IotclMqttConfig *mc = iotcl_mqtt_get_config();
    if (!mc) {
        IOTC_ERROR("set_up_qualilification_mode called, but not initialized?");
    	return;
    }
    // This value is permanent until reboot. No need to free it.
    // This one copy is to be kept around and second below in case it is freed
    if (!aws_qualification_host) {
        aws_qualification_host = iotcl_strdup(host);
    }
    const char* qualification_topic = "qualification";
    if (0 != strcmp("qualification", mc->pub_rpt)) {
    	// we need to override
    	iotcl_free(mc->host);
    	iotcl_free(mc->pub_rpt);
    	iotcl_free(mc->pub_ack);
    	iotcl_free(mc->sub_c2d);
    	mc->host = iotcl_strdup(host);
    	mc->pub_rpt = iotcl_strdup(qualification_topic);
    	mc->pub_ack = iotcl_strdup(qualification_topic);
    	mc->sub_c2d = iotcl_strdup(qualification_topic);
    }
}


/* Callback for received IoTC device commands */
static void on_command_receive(IotclC2dEventData data) {
    const char                  *command        = iotcl_c2d_get_command(data);
    const char                  *ack_id         = iotcl_c2d_get_ack_id(data);
    iotc_command_queue_item   queue_entry     = {0}; 

    if (command) {

// This behavior is disabled in the project by default, as it can be a security risk
// Enable this #define to enable the trigger and ensure that it is disabled in production
#ifdef AWS_QUALFICIATION_CMD_TRIGGER
        IOTC_INFO("Command %s received with %s ACK ID", command, ack_id ? ack_id : "no");

        // check if we got the qualification command
        const char * const QUALIFICATION_START_PREFIX_CMD = "aws-qualification-start "; // with a space
        if (command && (0 == strncmp(QUALIFICATION_START_PREFIX_CMD, command, strlen(QUALIFICATION_START_PREFIX_CMD)))) {
        	const char* qual_host = &command[strlen(QUALIFICATION_START_PREFIX_CMD)];
        	set_up_qualilification_mode(qual_host);
        	// ensure that we always set up the host FIRST
        	disconnect_iotconnect(); // trigger a disconnect on the main application thread. It will then attempt to reconnect
            IOTC_INFO("AWS Device Qualification will start after a wait period...");
        	return;
        }

#endif

        queue_entry.command = iotcl_strdup(command);

        if (queue_entry.command == NULL) {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
            goto cleanup;
        }

        if (ack_id) {
            queue_entry.ack_id = iotcl_strdup(ack_id);

            if (queue_entry.ack_id == NULL) {
                iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
                goto cleanup;
            }
        }

        if (pdTRUE == xQueueSendToBack(command_queue, &queue_entry, 0)) {
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_SUCCESS_WITH_ACK, "Command added to local queue");
        } else {
            IOTC_ERROR("Command queue full!");
            iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Command queue full");
            goto cleanup;
        }        
    } else {
        IOTC_ERROR("Failed to parse command");
        iotcl_mqtt_send_cmd_ack(ack_id, IOTCL_C2D_EVT_CMD_FAILED, "Internal error");
        goto cleanup;
    }

    return;

cleanup:
    iotc_command_queue_item_destroy(queue_entry);
}

bool iotc_command_queue_item_get(iotc_command_queue_item *dst_item) {

    if (command_queue == NULL) {
        IOTC_ERROR("Queue does not exist!");
        return false;
    }

    if (dst_item == NULL) {
        IOTC_ERROR("Target item is NULL");
        return false;
    }

    if (pdTRUE == xQueueReceive(command_queue, dst_item, 0)) {
        return true;
    } else {
        return false;    // Empty queue
    }
}

static void send_qualification_telemetery(void) {
    IotclMessageHandle msg = iotcl_telemetry_create();

    bool success = iotcl_telemetry_set_string(msg, "qualification", "true") == IOTCL_SUCCESS;
    if (!success) {
        IOTC_ERROR("ERROR in iotcl_telemetry_set_string!");
    } else {
        if (iotcl_mqtt_send_telemetry(msg, false) != IOTCL_SUCCESS) {
        	IOTC_ERROR("ERROR in iotcl_mqtt_send_telemetry!");
        }
    }
    iotcl_telemetry_destroy(msg);
}

static int periodic_event_wrapper(void) {
	int ret = -1;
	if (!is_initialized || !is_autoconnect) {
		return ret; // Client is stopped. Ignore silently
	}
	// keep trying to re-connect if in autoconnect mode
	if (!iotconnect_sdk_is_connected()) {
		ret = iotconnect_sdk_connect();
		if (ret != 0) {
			return ret;
		} else {
        	IOTC_INFO("IoTConnect Service task connected.");
    		last_connect_time = xTaskGetTickCount(); // only needed to stop stuck sessions for AWS qual
		}
	}
	if (aws_qualification_host) {
		// if in qualification mode...
        send_qualification_telemetery();
        if (((xTaskGetTickCount() - last_connect_time) * portTICK_PERIOD_MS) > 60000) {
        	IOTC_WARN("----------\nWARNING: Connection lingered for too long. Restarting the connection\n----------");
            iotconnect_sdk_disconnect();
        }
	}
	return ret;
}

//
// a wrapper to send +NWICSETUPBEGIN / +NWICSETUPEND messages
//
int setup_wrapper(void) {
    atcmd_asynchony_event_for_icsetup_begin();

    iotconnect_sdk_init_config(&s_client_cfg);

    if (iotc_da16k_read_config(&s_client_cfg) != 0) {
        IOTC_ERROR("Failed to get configuration values from nvram");
        goto cleanup;
    }

    // Set MQTT & HTTP Certs for this connection type
    iotc_da16k_dynamic_ca_set(s_client_cfg.connection_type);

    IOTC_INFO("IOTC_CONNECTION_TYPE = %s", s_client_cfg.connection_type == IOTC_CT_AWS ? "AWS" : "AZURE");
    IOTC_INFO("IOTC_ENV = %s", s_client_cfg.env);
    IOTC_INFO("IOTC_CPID = %s", s_client_cfg.cpid);
    IOTC_INFO("IOTC_DUID = %s", s_client_cfg.duid);
    IOTC_INFO("IOTC_AUTH_TYPE = %d", s_client_cfg.auth_info.type);
    IOTC_INFO("IOTC_AUTH_SYMMETRIC_KEY = %s", s_client_cfg.auth_info.data.symmetric_key ? s_client_cfg.auth_info.data.symmetric_key : "(null)");

    s_client_cfg.status_cb = on_connection_status;
    s_client_cfg.ota_cb = NULL;
    s_client_cfg.cmd_cb = on_command_receive;
    s_client_cfg.qos = 1; // force QOS to 1, regardless of what is stored in NVRAM
    s_client_cfg.verbose = true; // change this if you don't want to see inbound and outbound MQTT messages

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
int start_wrapper(void)
{
    int ret = -1;

    IOTC_INFO("IOTC: Client Starting...");
    atcmd_asynchony_event_for_icstart_begin();

    //
    // Try a small number of times - in case of an intermitent failure
    //
    for(int i = 0; i < 5; i++) {
        //
        // Run discovery/sync
    	// NOTE: For retries, calling iotconnect_sdk_init will denint first, so no need to call it
        ret = iotconnect_sdk_init(&s_client_cfg);
        if (ret == 0) {
        	is_initialized = true;
        	if (aws_qualification_host) {
        		set_up_qualilification_mode(aws_qualification_host);
        	}
        	break;
        } else {
        	is_initialized = false;
            IOTC_ERROR("iotconnect_sdk_init failed: %d", ret);
        }
    }

    // assuming that we initialized, we can keep trying to connect even if failed as long as is_initialized
    is_autoconnect = is_initialized;

	if (ret != 0) {
        IOTC_ERROR("start_wrapper() failed: %d", ret);
    }

#ifndef IOTC_DISABLE_AWS_QUALFICIATION_CMD
	// This command could be a security rish. Add this #define to your compiler settings in order to disable it.
	IOTC_INFO("AWS Device Qualification Command Trigger is enabled for this build. This should be disabled in production builds.");
#endif

    atcmd_asynchony_event_for_icstart_end(ret == 0);
    return ret;
}

//
// a wrapper to send +NWICSTOPBEGIN / +NWICSTOPEND messages
//
static void stop_wrapper(void)
{
	// only disable autoconnect when not in qualification mode
	is_autoconnect = false;

    atcmd_asynchony_event_for_icstop_begin();
    if(iotconnect_sdk_is_connected() == true) {
        iotconnect_sdk_disconnect();
    }

    // Reset so we don't start next session with leftover commands, and don't serve pending commands to connected clients inquiring.
    xQueueReset(command_queue);

    atcmd_asynchony_event_for_icstop_end(true);
    IOTC_INFO("IOTC: Client Stopped.");
}

//
// a wrapper to send +NWICRESETBEGIN / +NWICRESETEND messages
//
static void reset_wrapper(void) {

    is_initialized = false;
    is_autoconnect = false;

    atcmd_asynchony_event_for_icreset_begin();
        
    if(iotconnect_sdk_is_connected() == true) {
        iotconnect_sdk_disconnect();
    }
    iotconnect_sdk_deinit();

    iotc_da16k_dynamic_ca_clear();

    // This deinits all the allocated data in the config struct
    iotc_da16k_reset_config(&s_client_cfg);

    // Reset so we don't start next session with leftover commands, and don't serve pending commands to connected clients inquiring.
    xQueueReset(command_queue);

    atcmd_asynchony_event_for_icreset_end(true);
    IOTC_INFO("IOTC: Client configuration is Reset.");
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
    IOTC_INFO("\n\n\nInitializing the IoTConnect Client\n\n\n");

    /* Create queue to store commands in */
    command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(iotc_command_queue_item));
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
                                     10000 / portTICK_PERIOD_MS);

        bool reset = events & EVT_IOTC_RESET;
        bool start = events & EVT_IOTC_START;
        bool setup = events & EVT_IOTC_SETUP;
        bool stop = events & EVT_IOTC_STOP;
        bool disconnect = events & EVT_IOTC_DISCONNECT;
        // ignore EVT_IOTC_PERIODIC_EVENT, as we will always run the handler

        xEventGroupClearBits( my_app_event_group, (EVT_IOTC_STOP | EVT_IOTC_SETUP | EVT_IOTC_START | EVT_IOTC_RESET | EVT_IOTC_DISCONNECT) );


        if(!start && !setup && !stop && !reset && !disconnect) {
        	// will execute once every 10 seconds (per xEventGroupWaitBits delay)
            periodic_event_wrapper();
            continue;
        }

        if (disconnect) {
        	if (is_initialized) {
            	iotconnect_sdk_disconnect();
        	}
        	if (!aws_qualification_host) {
            	is_autoconnect = false; // only if not in qualification mode
        	}
        }

        if(network_ok() == false) {
            IOTC_ERROR("Command ignored - Network interface is down!");
            continue;
        }

        //
        // disconnect from iotconnect, 
        // teardown the current config and deallocate any memory
        //
        if(reset) {
            reset_wrapper();
            is_setup_ok = false;
            is_initialized = false;
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
            is_setup_ok = (0 == setup_wrapper());
        }

        if(start) {
            if (is_setup_ok == false) {
                IOTC_ERROR("Cannot start wrapper - config setup failed.");
                IOTC_INFO("Did you run: iotconnect_client setup?");
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
    
    IOTC_INFO("Exiting IotConnect service.");
    return 0;
}
