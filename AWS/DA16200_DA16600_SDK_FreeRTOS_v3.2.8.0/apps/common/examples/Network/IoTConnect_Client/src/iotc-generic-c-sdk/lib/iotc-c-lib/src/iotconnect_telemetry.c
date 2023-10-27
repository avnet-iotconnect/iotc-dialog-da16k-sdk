/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>


#include "iotconnect_common.h"
#include "iotconnect_lib.h"
#include "iotconnect_telemetry.h"


#include "_cJSON.h" // have globally replaced cJSON -> _cJSON to avoid conflicts with older/incompatible version of cJSON included by Renesas

/////////////////////////////////////////////////////////
// _cJSON implementation

struct IotclMessageHandleTag {
    _cJSON *root_value;
    _cJSON *telemetry_data_array;
    _cJSON *current_telemetry_object; // an object inside the "d" array inside the "d" array of the root object
    _cJSON *parent;
};

_cJSON *json_object_dotset_locate(_cJSON *search_object, char **leaf_name, const char *path) {
    *leaf_name = NULL;

    static const char *DELIM = ".";
    char *mutable_path = iotcl_strdup(path);
    char *token = strtok(mutable_path, DELIM);

    if (NULL == mutable_path) {
        IOTC_ERROR("iotcl_strdup() failed");
        return NULL;
    }

    if (NULL == token) {
        // PANIC. Should not happen
        IOTC_ERROR("token is NULL");
        free(mutable_path);
        return NULL;
    }

    _cJSON *target_object = search_object;
    do {
        free(*leaf_name);
        *leaf_name = iotcl_strdup(token);
        if (!*leaf_name) {
            IOTC_ERROR("iotcl_strdup() failed");
            goto cleanup;
        }
        char *last_token = *leaf_name;
        token = strtok(NULL, DELIM);
        if (NULL != token) {
            // we have more and need to create the nested object
            _cJSON *parent_object;
            if (_cJSON_HasObjectItem(target_object, last_token)) {
                parent_object = _cJSON_GetObjectItem(target_object, last_token);
            } else {
                parent_object = _cJSON_AddObjectToObject(target_object, last_token);
            }

            // NOTE: The user should clean up the search object if we return
            // That should free all added objects, so this should be safe to do
            if (!parent_object) {
                goto cleanup;
            }

            target_object = parent_object;
        }
    } while (token != NULL);

    free(mutable_path);
    return target_object;

cleanup:
    free(mutable_path);
    return NULL;
}

static _cJSON *setup_telemetry_object(IotclMessageHandle message) {
    IotclConfig *config = iotcl_get_config();
    if (!config) {
        IOTC_ERROR("config is NULL");
        return NULL;
    }
    if (!message) {
        IOTC_ERROR("message is NULL");
        return NULL;
    }

    _cJSON *telemetry_array = _cJSON_CreateObject();    //telemetry object is the data object d.
    if (!telemetry_array) {
        return NULL;
    }
    if (!_cJSON_AddStringToObject(telemetry_array, "id", config->device.duid)) {
        goto cleanup_to;
    }
    if (!_cJSON_AddStringToObject(telemetry_array, "tg", "")) {
        goto cleanup_to;
    }
    //add 3rd level d object which has cpu, version, sensor data.
    _cJSON *d3_object = _cJSON_AddObjectToObject(telemetry_array, "d");  //add "d" object for telemetry data(cpu, version)
    if (!d3_object) {
        goto cleanup_da;
    }
    //add telemetry object(id, tg, d_object) to "d" array of root.
    if (!_cJSON_AddItemToArray(message->telemetry_data_array, telemetry_array)) {
        goto cleanup_cto;
    }

    // setup the actual telemetry object to be used in subsequent calls
    message->current_telemetry_object = _cJSON_CreateObject();
    if (!message->current_telemetry_object) {
        goto cleanup_cto;
    }
    message->current_telemetry_object = d3_object;

    return telemetry_array; // return 2nd level d array

cleanup_cto:
    _cJSON_free(message->current_telemetry_object);

cleanup_da:
    _cJSON_free(d3_object);

cleanup_to:
    _cJSON_free(telemetry_array);

    return NULL;
}

IotclMessageHandle iotcl_telemetry_create(void) {

    IotclConfig *config = iotcl_get_config();
    if (!config) {
        IOTC_ERROR("config is NULL");
        return NULL;
    }
    if (!config->telemetry.cd) {
        IOTC_ERROR("config->telemetry.cd is NULL");
        return NULL;
    }

    struct IotclMessageHandleTag *msg =
            (struct IotclMessageHandleTag *) calloc(sizeof(struct IotclMessageHandleTag), 1);
    if (!msg) {
        IOTC_ERROR("calloc failed");
        return NULL;
    }

    msg->parent = _cJSON_CreateObject();    //create a root
    if (!msg->parent) {
        IOTC_ERROR("_cJSON_CreateObject() failed");
        goto cleanup;
    }

    if (!_cJSON_AddStringToObject(msg->parent, "cd", config->telemetry.cd)) {
        goto cleanup_root;
    }
    if (!_cJSON_AddNumberToObject(msg->parent, "mt", 0)) {
        goto cleanup_root; // telemetry message type (zero)
    }

//    msg->root_value = root;
//    if (!msg->root_value) goto cleanup;

    //create 1st level d object which is msg->root_vlaue
    msg->root_value = _cJSON_AddObjectToObject(msg->parent, "d");
    if (!msg->root_value) {
        goto cleanup_root;
    }

    //add 2nd level d array
    msg->telemetry_data_array = _cJSON_AddArrayToObject(msg->root_value, "d"); //create 2nd d_array in 1st d object
    if (!msg->telemetry_data_array) {
        goto cleanup_msg;
    }

    return msg;

cleanup_root:
    _cJSON_free(msg->parent);

cleanup_msg:
    _cJSON_free(msg->root_value);

cleanup:
    free(msg);
    return NULL;
}


// Not used
bool iotcl_telemetry_add_with_epoch_time(IotclMessageHandle message, time_t time) {
    if (!message) {
        return false;
    }
    _cJSON *telemetry_object = setup_telemetry_object(message);
    if (!telemetry_object)
    {
        return false;
    }
    if (!_cJSON_AddNumberToObject(telemetry_object, "ts", (double) time)) {
        return false;
    }
    if (!_cJSON_HasObjectItem(message->root_value, "ts")) {
        if (!_cJSON_AddNumberToObject(message->root_value, "ts", (double) time)) {
            return false;
        }
    }
    return true;
}

bool iotcl_telemetry_add_with_iso_time(IotclMessageHandle message, const char *time) {
    if (!message) {
        return false;
    }
    if (!_cJSON_AddStringToObject(message->root_value, "dt", time)) {
        return false;
    }

    _cJSON *const telemetry_object = setup_telemetry_object(message);
    if (!telemetry_object) {
        return false;
    }
    if (!_cJSON_AddStringToObject(telemetry_object, "dt", time)) {
        return false;
    }
    return true;
}

bool iotcl_telemetry_set_number(IotclMessageHandle message, const char *path, double value) {
    if (!message) {
        return false;
    }
    if (NULL == message->current_telemetry_object) {
        if (!iotcl_telemetry_add_with_iso_time(message, iotcl_iso_timestamp_now())) {
            return false;
        }
    }
    char *leaf_name = NULL;
    _cJSON *target = json_object_dotset_locate(message->current_telemetry_object, &leaf_name, path);
    if (!target) {
        goto cleanup; // out of memory
    }

    if (!_cJSON_AddNumberToObject(target, leaf_name, value)) {
        goto cleanup_tgt;
    }
    free(leaf_name);
    return true;

cleanup_tgt:
    _cJSON_free(target);

cleanup:
    free(leaf_name);
    return false;
}

bool iotcl_telemetry_set_bool(IotclMessageHandle message, const char *path, bool value) {
    if (!message) {
        return false;
    }
    if (NULL == message->current_telemetry_object) {
        if (!iotcl_telemetry_add_with_iso_time(message, iotcl_iso_timestamp_now())) {
            return false;
        }
    }
    char *leaf_name = NULL;
    _cJSON *target = json_object_dotset_locate(message->current_telemetry_object, &leaf_name, path);
    if (!target) {
        goto cleanup; // out of memory
    }

    if (!_cJSON_AddBoolToObject(target, leaf_name, value)) {
        goto cleanup_tgt;
    }
    free(leaf_name);
    return true;

cleanup_tgt:
    _cJSON_free(target);

    cleanup:
    free(leaf_name);
    return false;
}

bool iotcl_telemetry_set_string(IotclMessageHandle message, const char *path, const char *value) {
    if (!message) {
        return false;
    }
    if (NULL == message->current_telemetry_object) {
        iotcl_telemetry_add_with_iso_time(message, iotcl_iso_timestamp_now());
    }
    char *leaf_name = NULL;
    _cJSON *const target = json_object_dotset_locate(message->current_telemetry_object, &leaf_name, path);
    if (!target) {
        goto cleanup; // out of memory
    }

    if (!_cJSON_AddStringToObject(target, leaf_name, value)) {
        goto cleanup_tgt;
    }
    free(leaf_name);
    return true;

cleanup_tgt:
    _cJSON_free(target);

cleanup:
    free(leaf_name);
    return false;
}

bool iotcl_telemetry_set_null(IotclMessageHandle message, const char *path) {
    if (!message) {
        return false;
    }
    if (NULL == message->current_telemetry_object) {
        iotcl_telemetry_add_with_iso_time(message, iotcl_iso_timestamp_now());
    }
    char *leaf_name = NULL;
    _cJSON *const target = json_object_dotset_locate(message->current_telemetry_object, &leaf_name, path);
    if (!target) {
        goto cleanup; // out of memory
    }

    if (!_cJSON_AddNullToObject(target, leaf_name)) {
        goto cleanup_tgt;
    }
    free(leaf_name);
    return true;

cleanup_tgt:
    _cJSON_free(target);

cleanup:
    free(leaf_name);
    return false;
}

const char *iotcl_create_serialized_string(IotclMessageHandle message, bool pretty) {
    const char *serialized_string = NULL;
    if (!message) {
        IOTC_ERROR("message is NULL\n");
        return NULL;
    }
    if (!message->parent) {
        IOTC_ERROR("message->parent is NULL\n");
        return NULL;
    }
    serialized_string = (pretty) ? _cJSON_Print(message->parent) : _cJSON_PrintUnformatted(message->parent);
    if (!serialized_string) {
        IOTC_ERROR("serialized_string is NULL\n");
        return NULL;
    }
    return serialized_string;
}

void iotcl_destroy_serialized(const char *serialized_string) {
    _cJSON_free((char *) serialized_string);
}

void iotcl_telemetry_destroy(IotclMessageHandle message) {
    if (message) {
    	_cJSON_Delete(message->parent);
//        _cJSON_Delete(message->root_value);
        free(message);
    }
}

const char *iotcl_serialise_telemetry_strings(unsigned int no_strings, char *string_names[], char *string_values[],
                                        unsigned int no_numbers, char *number_names[], double number_values[],
                                        unsigned int no_bools, char *bool_names[], bool bool_values[],
                                        unsigned int no_nulls, char *null_names[]) {
    IotclMessageHandle msg = NULL;
    const char *timestamp = NULL;

    msg = iotcl_telemetry_create();
    if(msg == NULL) {
        IOTC_ERROR("iotcl_telemetry_create() failed\n");
        goto cleanup;
    }

    // Optional. The first time you create a data point, the current timestamp will be automatically added
    // TelemetryAddWith* calls are only required if sending multiple data points in one packet.
    timestamp = iotcl_iso_timestamp_now();
    if(timestamp == NULL) {
        IOTC_ERROR("iotcl_iso_timestamp_now() failed\n");
        goto cleanup;
    }

    if(iotcl_telemetry_add_with_iso_time(msg, timestamp) == false) {
        IOTC_ERROR("iotcl_telemetry_add_with_iso_time() failed\n");
        goto cleanup;
    }

    for(unsigned int i = 0;i < no_strings;i++)
    {
        if(iotcl_telemetry_set_string(msg, string_names[i], string_values[i]) == false) {
            IOTC_ERROR("iotcl_telemetry_set_string() failed\n");
            goto cleanup;
        }
    }

    for(unsigned int i = 0;i < no_numbers;i++)
    {
        if(iotcl_telemetry_set_number(msg, number_names[i], number_values[i]) == false) {
            IOTC_ERROR("iotcl_telemetry_set_number() failed\n");
            goto cleanup;
        }
    }

    for(unsigned int i = 0;i < no_bools;i++)
    {
        if(iotcl_telemetry_set_bool(msg, bool_names[i], bool_values[i]) == false) {
            IOTC_ERROR("iotcl_telemetry_set_bool() failed\n");
            goto cleanup;
        }
    }

    for(unsigned int i = 0;i < no_nulls;i++)
    {
        if(iotcl_telemetry_set_null(msg, null_names[i]) == false) {
            IOTC_ERROR("iotcl_telemetry_set_null() failed\n");
            goto cleanup;
        }
    }

    const char *str = iotcl_create_serialized_string(msg, false);
    if(str == NULL) {
        IOTC_ERROR("iotcl_create_serialized_string() failed\n");
        goto cleanup;
    }

    // partial cleanup
    iotcl_telemetry_destroy(msg);
    msg = NULL;

    return str;

cleanup:
    if(msg) {
        iotcl_telemetry_destroy(msg);
    }

    return NULL;
}
