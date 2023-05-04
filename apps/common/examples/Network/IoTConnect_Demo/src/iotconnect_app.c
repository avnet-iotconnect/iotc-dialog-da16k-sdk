//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 4/19/21.
//

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
#include "user_http_client.h"
#include "lwip/apps/http_client.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"
#include "lwip/err.h"
#include "mbedtls/ssl.h"
#include "command_net.h"

#define	IOTCONNECT_APP_STACK_SIZE	(1024 * 6) / 4 //WORD
#define IOTCONNECT_APP_TASK_NAME 	"IoTConnect Demo"

static TaskHandle_t iotconnect_app_task_handle = NULL;

static void iotconnect_app_task(void *param) {
	DA16X_UNUSED_ARG(param);
	PRINTF("Foo\n");
}
void iotconnect_demo_entry(void *param) {
	DA16X_UNUSED_ARG(param);

	BaseType_t xReturned;
	DA16_HTTP_CLIENT_REQUEST request;

	int wait_cnt = 0;
	int iface = WLAN0_IFACE;

	PRINTF(">>> Start IoTConnect Demo\n");

	while (chk_network_ready(iface) != pdTRUE) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
		wait_cnt++;

		if (wait_cnt == 100) {
			PRINTF("\r\n [%s] ERR : No network connection\r\n", __func__);
			wait_cnt = 0;
			goto finish;
		}
	}

	if (iotconnect_app_task_handle) {
		vTaskDelete(iotconnect_app_task_handle);
		iotconnect_app_task_handle = NULL;
	}

	xReturned = xTaskCreate(iotconnect_app_task, // <eclipse formatting hint
			IOTCONNECT_APP_TASK_NAME, //
			IOTCONNECT_APP_STACK_SIZE, //
			(void*) &request, //
			tskIDLE_PRIORITY + 1, //
			&iotconnect_app_task_handle //
			);
	if (xReturned != pdPASS) {
		PRINTF(RED_COLOR " [%s] Failed task create %s \r\n" CLEAR_COLOR,
				__func__, IOTCONNECT_APP_TASK_NAME);
	}

	finish: while (1) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	return;
}

