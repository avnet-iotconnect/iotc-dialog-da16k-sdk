//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 4/19/21.
//

#include <stdbool.h>
#include "sdk_type.h"
#include "sample_defs.h"
#include <ctype.h>
#include "da16x_system.h"
#include "application.h"
#include "iface_defs.h"
#include "common_def.h"
#include "da16x_network_common.h"
#include "da16x_dns_client.h"
#include "util_api.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"
#include "lwip/err.h"
#include "mbedtls/ssl.h"
#include "command_net.h"

// #include "da16_time.h"
#include "cJSON.h"
#include "iotconnect_lib.h"

#if defined (__SUPPORT_ATCMD__)
/// Data transfer with AT-CMD interface
typedef struct _atcmd_httpc_context {
    // flag
    unsigned int insert;
    //recv buffer
    char *buffer;
    unsigned int  buffer_len;
} atcmd_httpc_context;
#endif // (__SUPPORT_ATCMD__)

typedef enum {
    IOTC_CLIENT_OPCODE_INIT,  // Init value. Do not start, stop or anything else
    IOTC_CLIENT_OPCODE_START,
    IOTC_CLIENT_OPCODE_STOP,
} DA16_IOTC_OPCODE;

/// IOTC Request structure
typedef struct IOTC_CLIENT_REQUEST {
    DA16_IOTC_OPCODE op_code;
    CHAR d;
    bool set_cpid;
    bool set_env;
    bool set_duid;
} IOTC_CLIENT_REQUEST;


#define	IOTCONNECT_APP_STACK_SIZE	(1024 * 6) / 4 //WORD
#define IOTCONNECT_APP_TASK_NAME 	"IoTConnect Demo"

static TaskHandle_t iotconnect_app_task_handle = NULL;
static cJSON_Hooks hooks;
static IOTC_CLIENT_REQUEST iotc_client_request;

void iotc_client_display_usage(void)
{
    PRINTF("\nUsage: IOTC Client\n");
    PRINTF("\x1b[93mName\x1b[0m\n");
    PRINTF("\tiotc-client - IOTC Client\n");
    PRINTF("\x1b[93mSYNOPSIS\x1b[0m\n");
    PRINTF("\tiotc-client [OPTION]\n");
    PRINTF("\x1b[93mDESCRIPTION\x1b[0m\n");
    PRINTF("\tConnect to IoTConnect\n");

    PRINTF("\t\x1b[93m-cpid <CPID>\x1b[0m\n");
    PRINTF("\t\tSet CPID\n");
    PRINTF("\t\x1b[93m-env <ENV>\x1b[0m\n");
    PRINTF("\t\tSet Environment\n");
    PRINTF("\t\x1b[93m-duid <DUID>\x1b[0m\n");
    PRINTF("\t\tSet Device Uinque ID\n");
    PRINTF("\t\x1b[93m-help\x1b[0m\n");
    PRINTF("\t\tDisplay help\n");
    return ;
}

static void iotconnect_lib_test(void) {
    IotclConfig config;

    hooks.free_fn = vPortFree;
    hooks.malloc_fn = pvPortMalloc;
    cJSON_InitHooks(&hooks);
    memset(&config, 0, sizeof(config));
    config.device.cpid = "MyCpid";
    config.device.duid = "my-device-id";
    config.device.env = "prod";
    config.telemetry.dtg = "5a913fef-428b-4a41-9927-6e0f4a1602ba";

    iotcl_init(&config);

    IotclMessageHandle msg = iotcl_telemetry_create();
    // Initial entry will be created with system timestamp
    // You can call AddWith*Time before making Set* calls in order to add a custom timestamp

    // NOTE: Do not mix epoch and ISO timestamps
    iotcl_telemetry_set_number(msg, "789", 123);
    iotcl_telemetry_set_string(msg, "boo.abc.tuv", "prs");

    __time64_t now;
    da16x_time64(NULL, &now);
    // Create a new entry with different time and values
    iotcl_telemetry_add_with_iso_time(msg, iotcl_to_iso_timestamp(now));
    iotcl_telemetry_set_number(msg, "boo.bar", 111);
    iotcl_telemetry_set_string(msg, "123", "456");
    //iotcl_telemetry_set_number(msg, "789", 123.55);

    iotcl_telemetry_add_with_iso_time(msg, iotcl_iso_timestamp_now());
    iotcl_telemetry_set_null(msg, "nulltest");
    iotcl_telemetry_set_bool(msg, "booltest", true);

    const char *str = iotcl_create_serialized_string(msg, true);
    iotcl_telemetry_destroy(msg);
    PRINTF("Telemetry: %s (cnt=%u)\n", str, strlen(str));
    iotcl_destroy_serialized(str);
}

static void iotconnect_app_task(void *param) {
    DA16X_UNUSED_ARG(param);
    #define MEMORY_TEST
    #ifdef MEMORY_TEST
    #define TEST_BLOCK_SIZE  256
    #define TEST_BLOCK_COUNT 100
    static void *blocks[TEST_BLOCK_COUNT];
    void memory_test() {
        int i = 0;
        for (; i < TEST_BLOCK_COUNT; i++) {
            void *ptr = malloc(TEST_BLOCK_SIZE);
            if (!ptr) {
                break;
            }
            blocks[i] = ptr;
        }
        PRINTF("====Allocated %d blocks of size %d (of max %d)===\n", i, TEST_BLOCK_SIZE, TEST_BLOCK_COUNT);
        for (int j = i-1; j >= 0; j--) {
            free(blocks[j]);
        }
    }
    memory_test();
    memory_test();
    iotconnect_lib_test();
    #endif /* MEMORY_TEST */
    while (1) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static err_t iotc_client_parse_request(int argc, char *argv[], IOTC_CLIENT_REQUEST *request) {
    int index = 0;
    err_t err = ERR_OK;

    char **cur_argv = ++argv;

    for (index = 1; index < argc; index++, cur_argv++) {
        if (**cur_argv == '-') {
            if (strcasecmp("-cpid", *cur_argv) == 0) {
                request->set_cpid = true;
                cur_argv++;
            } else if (strcasecmp("-env", *cur_argv) == 0) {
                request->set_env = true;
                cur_argv++;
            } else if (strcasecmp("-duid", *cur_argv) == 0) {
                request->set_duid = true;
                cur_argv++;
            } else {
                PRINTF("Invalid parameters(%s)\n", *cur_argv);
                return ERR_VAL;
            }
        } else if (strcasecmp("start", *cur_argv) == 0) {
            request->op_code = IOTC_CLIENT_OPCODE_START;
        } else if (strcasecmp("stop", *cur_argv) == 0) {
            request->op_code = IOTC_CLIENT_OPCODE_STOP;
        } else if (strcasecmp("help", *cur_argv) == 0) {
            iotc_client_display_usage();
        } else {
            PRINTF("Invalid operation(%s)\n", *cur_argv);
            return ERR_VAL;
        }
    }
    return err;
}



void iotc_client_process_request(void *param) {
    DA16X_UNUSED_ARG(param);

    BaseType_t xReturned;
    IOTC_CLIENT_REQUEST* request = (IOTC_CLIENT_REQUEST*) param;

    PRINTF(">>> Starting the IoTConnect Client\n");

    if (iotconnect_app_task_handle) {
        vTaskDelete(iotconnect_app_task_handle);
        iotconnect_app_task_handle = NULL;
    }

    // When ready, laumch our own task so we have stack size under control
    xReturned = xTaskCreate(iotconnect_app_task, // <eclipse formatting hint
            IOTCONNECT_APP_TASK_NAME, //
            IOTCONNECT_APP_STACK_SIZE, //
            (void*) &request, //
            tskIDLE_PRIORITY + 1, //
            &iotconnect_app_task_handle //
            );
    if (xReturned != pdPASS) {
        PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR, __func__, IOTCONNECT_APP_TASK_NAME);
    }
    return;
}

err_t run_iotc_client(int argc, char *argv[]) {
    err_t err = ERR_OK;
    BaseType_t xReturned;

    if (argc <= 1) {
        iotc_client_display_usage();
        return ERR_ARG;
    }

    memset(&iotc_client_request, 0, sizeof(iotc_client_request));

    /*
    if (da16_http_client_conf.status == IOTC_CLIENT_OPCODE_INIT) {
        http_client_init_conf(&da16_http_client_conf);
    }
    err = http_client_execute_request(&da16_http_client_conf, &da16_httpc_request);
    */

    err = iotc_client_parse_request(argc, argv, &iotc_client_request);
    if (err != ERR_OK) {
        iotc_client_display_usage();
        goto err;
    }

    iotc_client_process_request(&iotc_client_request);

    if (err != ERR_OK) {
        goto err;
    }

    return ERR_OK;

    err:

    return err;
}

#ifdef __ITCONNECT_CLIENT_SAMPLE__
void iotc_client_sample_entry(void *param)
{
    DA16X_UNUSED_ARG(param);

    int wait_cnt = 0;
    int iface = WLAN0_IFACE;

    PRINTF("\n\n>>> Starting the IoTConnect Client sample\n\n\n");

    while (chk_network_ready(iface) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wait_cnt++;

        if (wait_cnt == 100) {
            PRINTF("\r\n [%s] ERR : No network connection\r\n", __func__);
            wait_cnt = 0;
            goto finish;
        }
    }
    char* args[2];
    args[0] = "iotc-client";
    args[1] = "start";

    run_iotc_client(2, args);

finish:
    while (1) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }

    return;
}
#endif // (__ITCONNECT_CLIENT_SAMPLE__)

