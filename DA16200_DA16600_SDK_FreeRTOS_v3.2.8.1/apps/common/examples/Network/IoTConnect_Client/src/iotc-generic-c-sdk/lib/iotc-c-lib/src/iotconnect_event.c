/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */
#include <stdlib.h>
#include <string.h>

#include "_cJSON.h" // have globally replaced _cJSON -> _cJSON to avoid conflicts with older/incompatible version of _cJSON included by Renesas
#include "iotconnect_common.h"
#include "iotconnect_lib.h"

#define CJSON_ADD_ITEM_HAS_RETURN \
    (CJSON_VERSION_MAJOR * 10000 + CJSON_VERSION_MINOR * 100 + CJSON_VERSION_PATCH >= 10713)

#if !CJSON_ADD_ITEM_HAS_RETURN
#error "_cJSON version must be 1.7.13 or newer"
#endif

struct IotclEventDataTag {
    _cJSON *data;
    _cJSON *root;
    IotConnectEventType type;
};

/*
Conversion of boolean to IOT 2.0 specification messages:

For commands:
Table 15 [Possible values for st]
4 Command Failed with some reason
6 Executed successfully

For OTA:
Table 16 [Possible values for st]
4 Firmware command Failed with some reason
7 Firmware command executed successfully
*/
static int to_ack_status(bool success, IotConnectEventType type) {
    int status = 4; // default is "failure"
    if (success == true) {
        switch (type) {
            case DEVICE_COMMAND:
                status = 6;
                break;
            case DEVICE_OTA:
                status = 7;
                break;
            default:
                // Can't do more than assume failure if unknown type is used.
                IOTC_ERROR("%s type %d\n", __func__, type);
                break;
        }
    }
    return status;
}


static bool iotc_process_callback(struct IotclEventDataTag *eventData) {
    IOTC_DEBUG("%s\n", __func__);

    if (eventData == NULL)
    {
        IOTC_ERROR("%s eventData == NULL\n", __func__);
        return false;
    }

    IotclConfig *config = iotcl_get_config();
    if (config == NULL)
    {
        IOTC_ERROR("%s config == NULL\n", __func__);
        return false;
    }

    if (config->event_functions.msg_cb) {
        config->event_functions.msg_cb(eventData, eventData->type);
    }
    switch (eventData->type) {
        case DEVICE_COMMAND:
            if (config->event_functions.cmd_cb) {
                IOTC_DEBUG("%s calling cmd_cb\n", __func__);

                config->event_functions.cmd_cb(eventData);
            } else {
                IOTC_DEBUG("%s cmd_cb is NULL\n", __func__);
            }
            break;
        case DEVICE_OTA:
            if (config->event_functions.ota_cb) {
                IOTC_DEBUG("%s calling ota_cb\n", __func__);

                config->event_functions.ota_cb(eventData);
            } else {
                IOTC_DEBUG("%s ota_cb is NULL\n", __func__);
            }
            break;
        default:
            IOTC_ERROR("%s unknown eventData->type %d\n", __func__, eventData->type);
            break;
    }

    return true;
}


/************************ CJSON IMPLEMENTATION **************************/
static inline bool is_valid_string(const _cJSON *json) {
    return (NULL != json && _cJSON_IsString(json) && json->valuestring != NULL);
}

#if 1
bool iotcl_process_event(const char *event) {
    IOTC_DEBUG("%s\n", __func__);

    bool status = false;
    _cJSON *root = _cJSON_Parse(event);
    if (root == NULL)
    {
        IOTC_ERROR("%s root == NULL\n", __func__);
        return false;
    }


    { // scope out the on-the fly varialble declarations for cleanup jump
    	_cJSON *j_type = _cJSON_GetObjectItemCaseSensitive(root, "ct");
        if (!_cJSON_IsNumber(j_type)) {
                IOTC_ERROR("%s failed to parse \"ct\"\n", __func__);
        	goto cleanup;
        }

        IotConnectEventType type = j_type->valueint + 1;

        _cJSON *j_ack_id;
		j_ack_id = _cJSON_GetObjectItemCaseSensitive(root, "ack");

        if (type < DEVICE_COMMAND) {
            goto cleanup;
        }

        if (type == DEVICE_OTA) {
        	if (!is_valid_string(j_ack_id)) {
                IOTC_ERROR("%s failed to parse \"ack\"\n", __func__);
            	goto cleanup;
        	}
        }

        struct IotclEventDataTag *eventData = (struct IotclEventDataTag *) calloc(
                sizeof(struct IotclEventDataTag), 1);

        if (NULL == eventData) {
                IOTC_ERROR("%s NULL == eventData\n", __func__);
        	goto cleanup;
        }

        eventData->root = root;
        eventData->data = NULL;
        eventData->type = type;

        // eventData and root (via eventData->root) will be freed when the user calls
        // iotcl_destroy_event(). The user is responsible to free this data inside the callback,
        // once they are done with it. This is done so that the user can choose to keep the event data
        // for purposes of replying with an ack once another process (perhaps in another thread) completes.

        return iotc_process_callback(eventData);
    }

    cleanup:
    _cJSON_Delete(root);
    return status;
}


#else
bool iotcl_process_event(const char *event) {
    bool status = false;
    _cJSON *root = _cJSON_Parse(event);

    if (!root) {
       return false;
    }

    { // scope out the on-the fly varialble declarations for cleanup jump
        // root object should only have cmdType
        _cJSON *j_type = _cJSON_GetObjectItemCaseSensitive(root, "cmdType");
        if (!is_valid_string(j_type)) goto cleanup;

        _cJSON *j_ack_id = NULL;
        _cJSON *data = NULL; // data should have ackId
        if (!is_valid_string(j_ack_id)) {
            data = _cJSON_GetObjectItemCaseSensitive(root, "data");
            if (!data) goto cleanup;
            j_ack_id = _cJSON_GetObjectItemCaseSensitive(data, "ackId");
            if (!is_valid_string(j_ack_id)) goto cleanup;
        }

        if (4 != strlen(j_type->valuestring)) {
            // Don't know how to parse it then...
            goto cleanup;
        }

        IotConnectEventType type = (IotConnectEventType) strtol(&j_type->valuestring[2], NULL, 16);

        if (type < DEVICE_COMMAND) {
            goto cleanup;
        }

        // In case we have a supported command. Do some checks before allowing further processing of acks
        // NOTE: "i" in cpId is lower case, but per spec it's supposed to be in upper case
        if (type == DEVICE_COMMAND || type == DEVICE_OTA) {
            if (
                    !is_valid_string(_cJSON_GetObjectItem(data, "cpid"))
                    || !is_valid_string(_cJSON_GetObjectItemCaseSensitive(data, "uniqueId"))
                    ) {
                goto cleanup;
            }
        }

        struct IotclEventDataTag *eventData = (struct IotclEventDataTag *) calloc(
                sizeof(struct IotclEventDataTag), 1);
        if (NULL == eventData) goto cleanup;

        eventData->root = root;
        eventData->data = data;
        eventData->type = type;

        // eventData and root (via eventData->root) will be freed when the user calls
        // iotcl_destroy_event(). The user is responsible to free this data inside the callback,
        // once they are done with it. This is done so that the user can choose to keep the event data
        // for purposes of replying with an ack once another process (perhaps in another thread) completes.
        return iotc_process_callback(eventData);
    }

    cleanup:

    _cJSON_Delete(root);
    return status;
}
#endif

char *iotcl_clone_command(IotclEventData data) {
    _cJSON *command = _cJSON_GetObjectItemCaseSensitive(data->data, "command");
    if (NULL == command || !is_valid_string(command)) {
        return NULL;
    }

    return iotcl_strdup(command->valuestring);
}

char *iotcl_clone_download_url(IotclEventData data, size_t index) {
    _cJSON *urls = _cJSON_GetObjectItemCaseSensitive(data->data, "urls");
    if (NULL == urls || !_cJSON_IsArray(urls)) {
        return NULL;
    }
    if ((size_t) _cJSON_GetArraySize(urls) > index) {
        _cJSON *url = _cJSON_GetArrayItem(urls, (int) index);
        if (is_valid_string(url)) {
            return iotcl_strdup(url->valuestring);
        } else if (_cJSON_IsObject(url)) {
            _cJSON *url_str = _cJSON_GetObjectItem(url, "url");
            if (is_valid_string(url_str)) {
                return iotcl_strdup(url_str->valuestring);
            }
        }
    }
    return NULL;
}


char *iotcl_clone_sw_version(IotclEventData data) {
    _cJSON *ver = _cJSON_GetObjectItemCaseSensitive(data->data, "ver");
    if (_cJSON_IsObject(ver)) {
        _cJSON *sw = _cJSON_GetObjectItem(ver, "sw");
        if (is_valid_string(sw)) {
            return iotcl_strdup(sw->valuestring);
        }
    }
    return NULL;
}

char *iotcl_clone_hw_version(IotclEventData data) {
    _cJSON *ver = _cJSON_GetObjectItemCaseSensitive(data->data, "ver");
    if (_cJSON_IsObject(ver)) {
        _cJSON *sw = _cJSON_GetObjectItem(ver, "hw");
        if (is_valid_string(sw)) {
            return iotcl_strdup(sw->valuestring);
        }
    }
    return NULL;
}

#if 1
char *iotcl_clone_ack_id(IotclEventData data) {
	_cJSON *j_ack_id;

    if (!data) return NULL;
    // already checked that ack ID is valid in the messages

    if(!data->root) return NULL;

	j_ack_id = _cJSON_GetObjectItemCaseSensitive(data->root, "ack");

	if (!j_ack_id) return NULL;

    char *ack_id = j_ack_id->valuestring;
    return iotcl_strdup(ack_id);
}
#else
char *iotcl_clone_ack_id(IotclEventData data) {
    _cJSON *ackid = _cJSON_GetObjectItemCaseSensitive(data->data, "ackId");
    if (is_valid_string(ackid)) {
        return iotcl_strdup(ackid->valuestring);
    }
    return NULL;
}
#endif

static const char *create_ack(
        IotConnectEventType message_type,
        const char *ack_id,
        bool success,
        const char *message) {

    const char *result = NULL;

    IotclConfig *config = iotcl_get_config();

    if (!config) {
        return NULL;
    }

    _cJSON *ack_json = _cJSON_CreateObject();

    if (ack_json == NULL) {
        return NULL;
    }

    // message type 5 in response is the command response. Type 11 is OTA response.
    if (!_cJSON_AddNumberToObject(ack_json, "mt", message_type == DEVICE_COMMAND ? 5 : 11)) goto cleanup;

    // FIXME: Is it "t" or "dt" ?
    if (!_cJSON_AddStringToObject(ack_json, "t", iotcl_iso_timestamp_now())) goto cleanup;

    if (!_cJSON_AddStringToObject(ack_json, "uniqueId", config->device.duid)) goto cleanup;
    if (!_cJSON_AddStringToObject(ack_json, "cpId", config->device.cpid)) goto cleanup;

    {
        _cJSON *sdk_info = _cJSON_CreateObject();
        if (NULL == sdk_info) {
            return NULL;
        }
        if (!_cJSON_AddItemToObject(ack_json, "sdk", sdk_info)) {
            _cJSON_Delete(sdk_info);
            goto cleanup;
        }
        if (!_cJSON_AddStringToObject(sdk_info, "l", CONFIG_IOTCONNECT_SDK_NAME)) goto cleanup;
        if (!_cJSON_AddStringToObject(sdk_info, "v", CONFIG_IOTCONNECT_SDK_VERSION)) goto cleanup;
        if (!_cJSON_AddStringToObject(sdk_info, "e", config->device.env)) goto cleanup;
    }

    {
        _cJSON *ack_data = _cJSON_CreateObject();
        if (NULL == ack_data) goto cleanup;
        if (!_cJSON_AddItemToObject(ack_json, "d", ack_data)) {
            _cJSON_Delete(ack_data);
            goto cleanup;
        }
        if (!_cJSON_AddStringToObject(ack_data, "ackId", ack_id)) goto cleanup;
        if (!_cJSON_AddStringToObject(ack_data, "msg", message ? message : "")) goto cleanup;
        if (!_cJSON_AddNumberToObject(ack_data, "st", to_ack_status(success, message_type))) goto cleanup;
    }

    result = (const char *) _cJSON_PrintUnformatted(ack_json);

    // fall through
    cleanup:
    _cJSON_Delete(ack_json);
    return result;
}

IotConnectEventType iotcl_get_event_type(IotclEventData data) {
    return (data) ? data->type : UNKNOWN_EVENT;
}

const char *iotcl_create_ack_string(
        IotConnectEventType type,
        const char *ack_id,
        bool success,
        const char *message
) {
    if (!ack_id) return NULL;
    const char *ret = create_ack(type, ack_id, success, message);
    return ret;
}

const char *iotcl_create_ota_ack_response(
        const char *ota_ack_id,
        bool success,
        const char *message
) {
    const char *ret = create_ack(DEVICE_OTA, ota_ack_id, success, message);
    return ret;
}

void iotcl_destroy_event(IotclEventData data) {
    _cJSON_Delete(data->root);
    free(data);
}

