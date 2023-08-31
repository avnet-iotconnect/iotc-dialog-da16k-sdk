#include "iotc_device_client.h"
#include "iotconnect_common.h"
#include "mqtt_client_sample.h"

static bool is_initialized = false;
static bool is_connected = false;
static IotConnectStatusCallback status_cb = NULL; // callback for connection status
static IotConnectC2dCallback c2d_msg_cb = NULL; // callback for inbound messages

static void iotc_device_deinit(void) {
    is_initialized = false;
    is_connected = false;
    status_cb = NULL;
    c2d_msg_cb = NULL;

    mqtt_sample_client_deinit();
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

static void iotc_mqtt_msg_cb(const char *buf, int len, const char *topic)
{
    IOTC_DEBUG("[MQTT_SAMPLE] Msg Recv: topic=%s, msg=%s, len = %d\n\n" CLEAR_COLOR, topic, buf, len);

    if (c2d_msg_cb) {
        c2d_msg_cb( (const unsigned char*) buf, (size_t) len);
    }
}

static void iotc_mqtt_pub_cb(int mid)
{
    IOTC_DEBUG("[%s] message id %d has been published!\n\n" CLEAR_COLOR, __func__, mid);
}

static void iotc_mqtt_conn_cb(void)
{
    IOTC_DEBUG("[%s] MQTT connection callback!\n\n" CLEAR_COLOR, __func__);

    is_connected = true;
}

static void iotc_mqtt_disconn_cb(void)
{
    IOTC_DEBUG("[%s] MQTT disconnection callback!\n\n" CLEAR_COLOR, __func__);

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_DISCONNECTED);
    }

    iotc_device_deinit();
}

static void iotc_mqtt_sub_cb(void)
{
    IOTC_DEBUG("[%s] topic subscribed!\n\n" CLEAR_COLOR, __func__);

    // signal IOTC_CS_MQTT_CONNECTED when broker is connected *and* the subscription succeeded
    if (is_connected == false) {
        IOTC_ERROR("[%s] topic subscribed but not connected?\n\n" CLEAR_COLOR, __func__);
    }

    if (status_cb) {
        status_cb(IOTC_CS_MQTT_CONNECTED);
    }
}

static void iotc_mqtt_unsub_cb(void)
{
    IOTC_ERROR("\n\n[%s] topic unsubscribed unexpectedly!\n\n" CLEAR_COLOR, __func__);
}

int iotc_device_client_send_message_qos(const char *message, int qos) {
    IOTC_WARN("%s: NOT FULLY IMPLEMENTED\n\n", __func__);
    // maybe we can implement by setting in NVRAM?
    // but really should be kept with the message in the message Q
    (void) qos;
    return mqtt_sample_client_pub_send(message, true);
}

int iotc_device_client_send_message(const char *message) {
    return mqtt_sample_client_pub_send(message, true);
}

int iotc_platform_acquire_config_values(IotConnectClientConfig *c) {
    char temp_str[256];
    int temp_int;

    c->env = NULL;
    c->cpid = NULL;
    c->duid = NULL;
    c->auth_info.data.symmetric_key = NULL;

    if(platform_get_env(temp_str) != 0)
    {
        IOTC_ERROR("platform_get_env() failed\n");
        goto cleanup;
    }
    c->env = iotcl_strdup(temp_str);
    if(c->env == NULL)
    {
        IOTC_ERROR("iotcl_strdup() failed\n");
        goto cleanup;
    }

    if(platform_get_cpid(temp_str) != 0)
    {
        IOTC_ERROR("platform_get_cpid() failed\n");
        goto cleanup;
    }
    c->cpid = iotcl_strdup(temp_str);
    if(c->cpid == NULL)
    {
        IOTC_ERROR("iotcl_strdup() failed\n");
        goto cleanup;
    }

    if(platform_get_duid(temp_str) != 0)
    {
        IOTC_ERROR("platform_get_duid() failed\n");
        goto cleanup;
    }
    c->duid = iotcl_strdup(temp_str);
    if(c->duid == NULL)
    {
        IOTC_ERROR("iotcl_strdup() failed\n");
        goto cleanup;
    }

    if(platform_get_auth_type(temp_str) != 0)
    {
        IOTC_ERROR("platform_get_auth_type() failed\n");
        goto cleanup;
    }
    if(sscanf(temp_str, "%d", &temp_int) != 1)
    {
        IOTC_ERROR("Failed to sscanf IOTC_AUTH_TYPE \"%s\"\n", temp_str);
        goto cleanup;
    }
    c->auth_info.type = temp_int;

    if(c->auth_info.type == IOTC_AT_SYMMETRIC_KEY)
    {
        if(platform_get_symmetric_key(temp_str) != 0)
        {
            IOTC_ERROR("platform_get_symmetric_key() failed\n");
            goto cleanup;
        }
        c->auth_info.data.symmetric_key = (const char *) iotcl_strdup(temp_str);
        if(c->auth_info.data.symmetric_key == NULL)
        {
            IOTC_ERROR("iotcl_strdup() failed\n");
            goto cleanup;
        }
    }

    return 0;

cleanup:
    free(c->env);
    free(c->cpid);
    free(c->duid);
    free((void *) c->auth_info.data.symmetric_key);
    return -1;
}

//
// setup the MQTT client configuration
//
int iotc_device_client_setup(IotConnectDeviceClientConfig *c) {
    /*
     * Remember the dtg value recovered from the sync response
     * The dtg value is needed if ever need to create a message "from scratch" and use AT+NWMSMSG command instead of AT+NWICMSG
     */
    platform_set_dtg(c->sr->dtg);

    mqtt_sample_client_nvram_config(c->sr->broker.host,
		           8883,
			   c->sr->broker.user_name,
			   c->sr->broker.pass,
		    	   c->sr->broker.client_id,
                           c->sr->broker.pub_topic,
			   c->sr->broker.sub_topic,
                           c->qos);

    // Don't handle certificates -- assume that there are AT commands to set them in a standard way.

    return 0;
}

//
// Run (start) the MQTT client using previously set configuration
//
int iotc_device_client_run(IotConnectDeviceClientConfig *c) {
    int ret;

    ret = mqtt_sample_client_init(iotc_mqtt_msg_cb, iotc_mqtt_pub_cb, iotc_mqtt_conn_cb, iotc_mqtt_disconn_cb, iotc_mqtt_sub_cb, iotc_mqtt_unsub_cb);
    if (ret != 0) {
        IOTC_ERROR("[%s] mqtt_sample_client_init() failed\n", __func__);
	goto cleanup;
    }

    is_initialized = true;

    c2d_msg_cb = c->c2d_msg_cb;
    status_cb = c->status_cb;

    return 0;

cleanup:
    iotc_device_deinit();
    return -1;
}

int iotc_device_client_preinit_certs(IotConnectDeviceClientConfig *c) {
    // Need X509 certificate the server -- this isn't optional
    mqtt_broker_cert_config( c->auth->trust_store, strlen(c->auth->trust_store));

    // Need X509 certificate and private key for the device -- they may be NULL for some authentication types
    mqtt_device_cert_config( c->auth->data.cert_info.device_cert, (c->auth->data.cert_info.device_cert) ? strlen(c->auth->data.cert_info.device_cert) : 0,
  			     c->auth->data.cert_info.device_key,  (c->auth->data.cert_info.device_cert) ? strlen(c->auth->data.cert_info.device_key) : 0);
    
    return 0;
}

int iotc_device_client_init(IotConnectDeviceClientConfig *c) {
    int ret;
    ret = iotc_device_client_setup(c);
    if(ret == 0)
    {
        ret = iotc_device_client_run(c);
    }

    return ret;
}


// not supported 
char* iotc_device_client_get_tpm_registration_id(void) {
    return NULL;
}

