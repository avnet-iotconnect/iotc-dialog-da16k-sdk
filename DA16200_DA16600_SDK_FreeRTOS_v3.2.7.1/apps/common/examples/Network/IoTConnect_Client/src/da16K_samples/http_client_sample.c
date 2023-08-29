/**
 ****************************************************************************************
 *
 * @file http_client_sample.c
 *
 * @brief Sample app for HTTP client (GET, POST).
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


#if defined (__HTTP_CLIENT_SAMPLE__)
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
#include <string.h>

#include "iotc_http_request.h"
#include "common_config.h"

// or specify as functions
#define HTTP_DEBUG(...) do{ PRINTF(GREEN_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)
#define HTTP_WARN(...) do{ PRINTF(YELLOW_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)
#define HTTP_ERROR(...) do{ PRINTF(RED_COLOR );PRINTF(__VA_ARGS__);PRINTF(CLEAR_COLOR);}while(0)

extern int iotconnect_basic_sample_main(void);

#define    EVT_HTTP_COMPLETE     (1UL << 0x00)
#define    EVT_HTTP_DATA     (1UL << 0x01)
#define    EVT_HTTP_ERROR     (1UL << 0x02)

#undef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

#define HTTP_CLIENT_SAMPLE_TASK_NAME    "http_c_sample"
#define    HTTP_CLIENT_SAMPLE_STACK_SIZE    (1024 * 6) / 4 //WORD

TaskHandle_t g_sample_http_client_xHandle = NULL;

static EventGroupHandle_t my_app_event_group = NULL;
static char *total_payload = NULL;
static long unsigned int total_payload_len = 0;

static ip_addr_t g_server_addr;
static httpc_connection_t g_conn_settings = {0, };
static httpc_state_t *g_connection = NULL;
static DA16_HTTP_CLIENT_REQUEST g_request;

/*
 * g_http_url and g_post_data are used as globals to pass info to the asynchronous HTTP task
 */
static char g_post_data[256];
static char g_http_url[256];

#if 0
/* DigiCert Global Root G2 */
static const char *http_root_ca_buffer0 =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\r\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\r\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\r\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\r\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\r\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\r\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\r\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\r\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\r\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\r\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\r\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\r\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\r\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\r\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\r\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\r\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\r\n"
"MrY=\r\n"
"-----END CERTIFICATE-----\r\n";

// Need certificate for HTTPS to talk to the server
static void http_broker_cert_config(const char *root_ca, int root_ca_len)
{
    cert_flash_write(SFLASH_ROOT_CA_ADDR2, (char *) root_ca, root_ca_len);
}
#endif

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
#define    CERT_MAX_LENGTH        1024 * 4
#define    FLASH_WRITE_LENGTH    1024 * 4
static void http_client_clear_alpn(httpc_secure_connection_t *conf)
{
    int idx = 0;

    if (conf->alpn) {
        for (idx = 0 ; idx < conf->alpn_cnt ; idx++) {
            if (conf->alpn[idx]) {
                vPortFree(conf->alpn[idx]);
                conf->alpn[idx] = NULL;
            }
        }

        vPortFree(conf->alpn);
    }

    conf->alpn = NULL;
    conf->alpn_cnt = 0;

    return ;
}

static void http_client_clear_https_conf(httpc_secure_connection_t *conf)
{
    if (conf) {
        if (conf->ca) {
            vPortFree(conf->ca);
        }

        if (conf->cert) {
            vPortFree(conf->cert);
        }

        if (conf->privkey) {
            vPortFree(conf->privkey);
        }

        if (conf->sni) {
            vPortFree(conf->sni);
        }

        http_client_clear_alpn(conf);

        memset(conf, 0x00, sizeof(httpc_secure_connection_t));

        conf->auth_mode = MBEDTLS_SSL_VERIFY_NONE;
        conf->incoming_len = HTTPC_DEF_INCOMING_LEN;
        conf->outgoing_len = HTTPC_DEF_OUTGOING_LEN;
    }

    return ;
}

static int http_client_read_cert(unsigned int addr, unsigned char **out, size_t *outlen)
{
    int ret = 0;
    unsigned char *buf = NULL;
    size_t buflen = CERT_MAX_LENGTH;

    buf = pvPortMalloc(CERT_MAX_LENGTH);
    if (!buf) {
        HTTP_ERROR("Failed to allocate memory(%x)\r\n", addr);
        return -1;
    }

    memset(buf, 0x00, CERT_MAX_LENGTH);

    ret = cert_flash_read(addr, buf, CERT_MAX_LENGTH);
    if (ret == 0 && buf[0] != 0xFF) {
        *out = buf;
        *outlen = strlen(buf) + 1;
        return 0;
    }

    if (buf) {
        vPortFree(buf);
    }
    return 0;
}
static void http_client_read_certs(httpc_secure_connection_t *settings)
{
    int ret = 0;

    //to read ca certificate
    ret = http_client_read_cert(SFLASH_ROOT_CA_ADDR2, &settings->ca, &settings->ca_len);
    if (ret) {
        HTTP_ERROR("failed to read CA cert\r\n");
        goto err;
    }

    //to read certificate
    ret = http_client_read_cert(SFLASH_CERTIFICATE_ADDR2,
                                &settings->cert, &settings->cert_len);
    if (ret) {
        HTTP_ERROR("failed to read certificate\r\n");
        goto err;
    }

    //to read private key
    ret = http_client_read_cert(SFLASH_PRIVATE_KEY_ADDR2,
                                &settings->privkey, &settings->privkey_len);
    if (ret) {
        HTTP_ERROR("failed to read private key\r\n");
        goto err;
    }

    return ;

err:

    if (settings->ca) {
        vPortFree(settings->ca);
    }

    if (settings->cert) {
        vPortFree(settings->cert);
    }

    if (settings->privkey) {
        vPortFree(settings->privkey);
    }

    settings->ca = NULL;
    settings->ca_len = 0;
    settings->cert = NULL;
    settings->cert_len = 0;
    settings->privkey = NULL;
    settings->privkey_len = 0;

    return ;
}
#endif //ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

static err_t httpc_cb_headers_done_fn(httpc_state_t *connection,
                                      void *arg, struct    pbuf *hdr, u16_t hdr_len, u32_t content_len)
{

    DA16X_UNUSED_ARG(connection);
    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(hdr);

    HTTP_DEBUG("hdr_len : %d, content_len : %d\n", hdr_len, content_len);
    return ERR_OK;
}

static void httpc_cb_result_fn(void *arg, httpc_result_t httpc_result,
                               u32_t rx_content_len, u32_t srv_res, err_t err)
{

    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(srv_res);

    HTTP_DEBUG("httpc_result: %d, received: %d byte, err: %d\n", httpc_result, rx_content_len, err);

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    http_client_clear_https_conf(&g_conn_settings.tls_settings);//(&g_request.https_conf);
#endif // ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

    if(err != 0) {
        HTTP_ERROR("\n[%s:%d] Received an error!!\r\n", __func__, __LINE__);
        xEventGroupSetBits(my_app_event_group, EVT_HTTP_ERROR);
    } else if(rx_content_len != total_payload_len) {
        HTTP_WARN("\n[%s:%d] Received data doesn't match the total payload!! \r\n", __func__, __LINE__);
	HTTP_WARN("rx_content_len = %s\r\n", rx_content_len);
	HTTP_WARN("total_payload_len = %s\r\n", total_payload_len);

        xEventGroupSetBits(my_app_event_group, EVT_HTTP_ERROR);
    } else {
        HTTP_DEBUG("\n[%s:%d] Received data ok!! \r\n", __func__, __LINE__);
        xEventGroupSetBits(my_app_event_group, EVT_HTTP_COMPLETE);
    }
    return;
}

static void add_single_payload(struct pbuf *p)
{
    // +1 for a NULL terminator!
    char *resize_total_payload = (total_payload == NULL) ? malloc(p->len + 1) : realloc(total_payload, total_payload_len + p->len + 1);
    if (resize_total_payload == NULL) {
        if (total_payload) {
            free(total_payload);
        }
        total_payload = NULL;
        total_payload_len = 0;

        /* subsequent calls may malloc()/realloc() -- but the EVT_HTTP_ERROR bit should indicate that payload data is missing */
        xEventGroupSetBits(my_app_event_group, EVT_HTTP_ERROR);
        return;
    }

    total_payload = resize_total_payload;
    memcpy(total_payload + total_payload_len, p->payload, p->len);
    total_payload_len = total_payload_len + p->len; // doesn't count NULL terminator
    total_payload[total_payload_len] = '\0'; // ensure is NULL terminated in case do string operations later
    HTTP_DEBUG("total_payload_len = %d \r\n", total_payload_len);
}

static err_t httpc_cb_recv_fn(void *arg, struct tcp_pcb *tpcb,
                              struct pbuf *p, err_t err)
{
    DA16X_UNUSED_ARG(arg);
    DA16X_UNUSED_ARG(tpcb);
    DA16X_UNUSED_ARG(err);

    if (p == NULL) {
        HTTP_ERROR("\n[%s:%d] Receive data is NULL !! \r\n", __func__, __LINE__);
        return ERR_BUF;
    } else {
        while (p != NULL) {
            HTTP_DEBUG("\n[%s:%d] Received length = %d \r\n", __func__, __LINE__, p->len);
            add_single_payload(p);
            p = p->next;
        }
    }

    return ERR_OK;
}

static void http_client_sample(void *arg)
{
    err_t error = ERR_OK;
#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    unsigned int sni_len = 0;
    char *sni_str;
    int index = 0;
    int alpn_cnt = 0;
#endif// ENABLE_HTTPS_SERVER_VERIFY_REQUIRED

    if (arg == NULL) {
        HTTP_ERROR("[%s:%d] Input is NULL !! \r\n", __func__, __LINE__);
        return;
    }

    // Initialize ...
    memset(&g_server_addr, 0, sizeof(ip_addr_t));
    memset(&g_conn_settings, 0, sizeof(httpc_connection_t));
    g_connection = NULL;

    memcpy(&g_request, (DA16_HTTP_CLIENT_REQUEST *)arg, sizeof(DA16_HTTP_CLIENT_REQUEST));

    g_conn_settings.use_proxy = 0;
    g_conn_settings.altcp_allocator = NULL;

    g_conn_settings.headers_done_fn = httpc_cb_headers_done_fn;
    g_conn_settings.result_fn = httpc_cb_result_fn;
    g_conn_settings.insecure = g_request.insecure;

    if(strlen(g_post_data) != 0) {
        error = httpc_insert_send_data("POST", g_post_data, strlen(g_post_data));
        if (error != ERR_OK) {
            HTTP_ERROR("Failed to insert POST data\n");
            goto finish;
        }
    }

    if (g_conn_settings.insecure) {
        memset(&g_conn_settings.tls_settings, 0x00, sizeof(httpc_secure_connection_t));
        g_conn_settings.tls_settings.incoming_len = HTTPC_MAX_INCOMING_LEN;
        g_conn_settings.tls_settings.outgoing_len = HTTPC_DEF_OUTGOING_LEN;

#ifdef ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
        http_client_read_certs(&g_conn_settings.tls_settings);

        /* auth_mode */
        /* #define MBEDTLS_SSL_VERIFY_NONE      0  */
        /* #define MBEDTLS_SSL_VERIFY_OPTIONAL  1  */
        /* #define MBEDTLS_SSL_VERIFY_REQUIRED  2  */
        g_conn_settings.tls_settings.auth_mode = MBEDTLS_SSL_VERIFY_NONE;

        /* SNI */
        sni_str = read_nvram_string(HTTPC_NVRAM_CONFIG_TLS_SNI);
        if (sni_str != NULL) {
            sni_len = strlen(sni_str);

            if ((sni_len > 0) && (sni_len < HTTPC_MAX_SNI_LEN)) {
                if (g_conn_settings.tls_settings.sni != NULL) {
                    vPortFree(g_conn_settings.tls_settings.sni);
                }
                g_conn_settings.tls_settings.sni = pvPortMalloc(sni_len + 1);
                if (g_conn_settings.tls_settings.sni == NULL) {
                    HTTP_ERROR("[%s]Failed to allocate SNI(%ld)\n", __func__, sni_len);
                    goto finish;
                }
                strcpy(g_conn_settings.tls_settings.sni, sni_str);
                g_conn_settings.tls_settings.sni_len = sni_len + 1;
                HTTP_ERROR("[%s]ReadNVRAM SNI = %s\n", __func__, g_conn_settings.tls_settings.sni);
            }
        }

        /* ALPN */
        if (read_nvram_int(HTTPC_NVRAM_CONFIG_TLS_ALPN_NUM, &alpn_cnt) == 0) {
            if (alpn_cnt > 0) {
                http_client_clear_alpn(&g_conn_settings.tls_settings);

                g_conn_settings.tls_settings.alpn = pvPortMalloc((alpn_cnt + 1) * sizeof(char *));
                if (!g_conn_settings.tls_settings.alpn) {
                    HTTP_ERROR("[%s]Failed to allocate ALPN\n", __func__);
                    goto finish;
                }

                for (index = 0 ; index < alpn_cnt ; index++) {
                    char nvrName[15] = {0, };
                    char *alpn_str;

                    if (index >= HTTPC_MAX_ALPN_CNT) {
                        break;
                    }

                    sprintf(nvrName, "%s%d", HTTPC_NVRAM_CONFIG_TLS_ALPN, index);
                    alpn_str = read_nvram_string(nvrName);

                    g_conn_settings.tls_settings.alpn[index] = pvPortMalloc(strlen(alpn_str) + 1);
                    if (!g_conn_settings.tls_settings.alpn[index]) {
                        HTTP_ERROR("[%s]Failed to allocate ALPN#%d(len=%d)\n", __func__, index + 1, strlen(alpn_str));
                        http_client_clear_alpn(&g_conn_settings.tls_settings);
                        goto finish;
                    }

                    g_conn_settings.tls_settings.alpn_cnt = index + 1;
                    strcpy(g_conn_settings.tls_settings.alpn[index], alpn_str);
                    HTTP_ERROR("[%s]ReadNVRAM ALPN#%d = %s\n", __func__,
                           g_conn_settings.tls_settings.alpn_cnt,
                           g_conn_settings.tls_settings.alpn[index]);
                }
                g_conn_settings.tls_settings.alpn[index] = NULL;
            }
        }
#endif //ENABLE_HTTPS_SERVER_VERIFY_REQUIRED
    }

    if (is_in_valid_ip_class((char *)g_request.hostname)) {
        ip4addr_aton((char *)g_request.hostname, &g_server_addr);
        error = httpc_get_file(&g_server_addr,
                               g_request.port,
                               (char *)&g_request.path[0],
                               &g_conn_settings,
                               (altcp_recv_fn)httpc_cb_recv_fn,
                               NULL,
                               &g_connection);
    } else {
        error = httpc_get_file_dns((char *)&g_request.hostname[0],
                                   g_request.port,
                                   (char *)&g_request.path[0],
                                   &g_conn_settings,
                                   (altcp_recv_fn)httpc_cb_recv_fn,
                                   NULL,
                                   &g_connection);
    }

    if (error != ERR_OK) {
        HTTP_ERROR("Failed to send HTTP(S) request!! (error=%d)\n", error);
        goto finish;
    }

finish:

    while (1) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
    return;

}

int iotconnect_https_request(IotConnectHttpResponse *response, const char *url_buff, const char *post_data)
{
    EventBits_t events = 0;
    BaseType_t xReturned;
    DA16_HTTP_CLIENT_REQUEST request;

    memset(response, 0, sizeof(*response));

    if(total_payload) {
        HTTP_WARN("total_payload was non-NULL");
        free(total_payload);
    }
    if(total_payload_len) {
        HTTP_WARN("total_payload_len was non-zero");
    }
    total_payload = NULL;
    total_payload_len = 0;

    /*
     * FIXME TODO
     * Not sure I need to copy to static buffers here...
     */
    if(url_buff == NULL || strlen(url_buff) == 0) {
        goto finish;
    }

    strcpy(g_http_url, url_buff);
    if(post_data) {
        strcpy(g_post_data, post_data);
    } else {
        *g_post_data = '\0';
    }

#if 0
    http_broker_cert_config(http_root_ca_buffer0, strlen(http_root_ca_buffer0));
#endif

    /*
     * http_client_parse_uri() will set
     * http:// -> insecure == pdFALSE
     * https:// -> insecure == pdTRUE
     *
     * So, it seems that insecure should be read as "in secure" which is a bit counterintuitive.
     *
     * By default:
     * ===========
     * #define HTTP_SERVER_PORT              80
     * #define HTTPS_SERVER_PORT             443
     * #define HTTPC_MAX_HOSTNAME_LEN        256
     * #define HTTPC_MAX_PATH_LEN            256
     * #define HTTPC_MAX_NAME                20
     * #define HTTPC_MAX_PASSWORD            20
     */
    if (http_client_parse_uri((unsigned char *)g_http_url, strlen((char *)g_http_url), &request) != ERR_OK) {
        HTTP_ERROR("Failed to parse URI\r\n");
        goto finish;
    }

    my_app_event_group = xEventGroupCreate();
    if (my_app_event_group == NULL) {
        HTTP_ERROR("[%s] Event group Create Error!", __func__);
        goto finish;
    }

    /* clear wait bits here */
    xEventGroupClearBits( my_app_event_group, (EVT_HTTP_COMPLETE | EVT_HTTP_ERROR) );

    xReturned = xTaskCreate(http_client_sample,
                            HTTP_CLIENT_SAMPLE_TASK_NAME,
                            HTTP_CLIENT_SAMPLE_STACK_SIZE,
                            (void *)&request,
                            tskIDLE_PRIORITY + 1,
                            &g_sample_http_client_xHandle);
    if (xReturned != pdPASS) {
        HTTP_ERROR("[%s] Failed task create %s \r\n", __func__, HTTP_CLIENT_SAMPLE_TASK_NAME);
        goto finish;
    }

    events = xEventGroupWaitBits(my_app_event_group,
                                 (EVT_HTTP_COMPLETE | EVT_HTTP_ERROR),
                                 pdFALSE,
                                 pdFALSE,
                                 30000 / portTICK_PERIOD_MS);

    /* wait for discovery bit to be set here */
    /* check for timeout, etc. */
    if (events & EVT_HTTP_ERROR) {
        HTTP_ERROR("HTTP(S) error");

        /* note there may be a non-NULL total_payload, etc. */
        if(total_payload)
        {
            free(total_payload);
        }
        total_payload = NULL;
        total_payload_len = 0;
    
        goto finish;
    } 

    if ( !(events & EVT_HTTP_COMPLETE) ) {
        HTTP_ERROR("HTTP(S) did not complete");

        /* note there may be a non-NULL total_payload, etc. */
        if(total_payload)
        {
            free(total_payload);
        }
        total_payload = NULL;
        total_payload_len = 0;
    
        goto finish;
    }

    HTTP_DEBUG("HTTP(S) completed successfully - total_payload is %d bytes\n\n", total_payload_len);
    if(total_payload_len <= 2)
    {
        // minimum acceptable JSON string is "{}"
        // note malloc/realloc have +1 for a NULL terminator!
        goto finish;
    }

    long unsigned int i;
    for(i = 0;i < total_payload_len && total_payload[i] != '{';i++);
    if(i >= total_payload_len)
    {
        // can't find a '{'
        goto finish;
    }

    long unsigned int j;
    for(j = total_payload_len;j > i && total_payload[j] != '}';j--);
    if(j == i)
    {
        // can't find a last '}' before the first '{'
        goto finish;
    }
    total_payload[j + 1] = '\0'; // NULL terminate immediately after the last "}" in case there is some trailing "cruft"

    response->data = &total_payload[i]; // just in case -- as there might be some leading "cruft" before the JSON starts?

finish:
    /*
     * FIXME TODO delay left from original code -- is it still needed?
     */
    vTaskDelay( 100 / portTICK_PERIOD_MS );

    if(my_app_event_group) {
        vEventGroupDelete(my_app_event_group);
        my_app_event_group = NULL;
    }
    
    if (g_sample_http_client_xHandle) {
        vTaskDelete(g_sample_http_client_xHandle);
        g_sample_http_client_xHandle = NULL;
    }

    // did anything go wrong?
    if(response->data == NULL) {
        memset(response, 0, sizeof(*response));

        free(total_payload);
        total_payload = NULL;
        total_payload_len = 0;

        return (int) ERR_UNKNOWN;
    }

    response->original = total_payload; // might be some leading "cruft" before/after the JSON? response->original != response_data
    response->original_len = total_payload_len; // might be some leading "cruft" before/after the JSON? total_payload_len != strlen(response->data)
    return (int) ERR_OK;
}

void iotconnect_free_https_response(IotConnectHttpResponse *response)
{
    if(response->original) {
        if(response->original != total_payload) {
            HTTP_WARN("response->original != total_payload\n");
        }
    }
    memset(response, 0, sizeof(*response));

    if(total_payload) {
        free(response->original);
    }
    total_payload = NULL;
    total_payload_len = 0;
}

err_t network_ok(void) 
{
    int iface = WLAN0_IFACE;
    int wait_cnt = 0;

    while (chk_network_ready(iface) != pdTRUE) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        wait_cnt++;

        if (wait_cnt == 100) {
            HTTP_ERROR("\r\n [%s] ERR : No network connection\r\n", __func__);
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
                HTTP_ERROR("SNTP sync timed out - check Internet conenction. Reboot to start over \n");
            } else {
                if (ret == -2) { // timeout
                    HTTP_WARN("SNTP was disabled. Reboot to start over \n");
    
                    da16x_set_config_int(DA16X_CONF_INT_SNTP_CLIENT, 1);
                }
            }
            return ERR_UNKNOWN;
       }
   }
    
    return ERR_OK;
}

void http_client_sample_entry(void *param)
{
    DA16X_UNUSED_ARG(param);
    // slight delay before starting up -- just to give us a chance to type at the prompt, if required.
    vTaskDelay( 5000 / portTICK_PERIOD_MS );

    if(network_ok() != ERR_OK) {
        goto finish;
    }

    iotconnect_basic_sample_main();

finish:
    /*
     * TASK HAS FINISHED BUT CANNOT RETURN - SO JUST WAIT IN A LOOP!
     */
    HTTP_DEBUG("TASK HAS FINISHED\n");
    while (1) {
        vTaskDelay( 100 / portTICK_PERIOD_MS );
    }
}


#endif // (__HTTP_CLIENT_SAMPLE__)

/* EOF */
