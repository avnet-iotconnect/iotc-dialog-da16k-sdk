/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdbool.h>
#include <stdlib.h>

#include <string.h>

#include "cJSON.h"
#include "iotconnect_common.h"
#include "iotconnect_discovery.h"

static int get_numeric_value_or_default(cJSON *cjson, const char *value_name, int default_value) {
    cJSON* tmp_value = cJSON_GetObjectItem(cjson, value_name);
    if (!tmp_value || !cJSON_IsNumber(tmp_value)) {
        return default_value;
    }
    return tmp_value->valueint;
}

static char *safe_get_string_and_strdup(cJSON *cjson, const char *value_name) {
    cJSON *value = cJSON_GetObjectItem(cjson, value_name);
    if (!value) {
        IOTC_ERROR("cJSON_GetObjectItem failed\n");
        return NULL;
    }
    const char *str_value = cJSON_GetStringValue(value);
    if (!str_value) {
        IOTC_ERROR("cJSON_GetStringValue failed\n");
        return NULL;
    }
    return iotcl_strdup(str_value);
}

static bool split_url(IotclDiscoveryResponse *response) {
    size_t base_url_len = strlen(response->url);


    // mutable version that will allow us to modify the url string
    char *base_url_copy = iotcl_strdup(response->url);
    if (!base_url_copy) {
        IOTC_ERROR("iotcl_strdup failed\n");
        return false;
    }
    int num_found = 0;
    char *host = NULL;
    for (size_t i = 0; i < base_url_len; i++) {
        if (base_url_copy[i] == '/') {
            num_found++;
            if (num_found == 2) {
                host = &base_url_copy[i + 1];
                // host will be terminated below
            } else if (num_found == 3) {
                response->path = iotcl_strdup(&base_url_copy[i]); // first make a copy
                base_url_copy[i] = 0; // then terminate host so that it can be duped below
                break;
            }
        }
    }
    response->host = iotcl_strdup(host);
    free(base_url_copy);

    return (response->host && response->path);
}

IotclDiscoveryResponse *iotcl_discovery_parse_discovery_response(const char *response_data) {
    cJSON *json_root = cJSON_Parse(response_data);
    if (!json_root) {
        IOTC_ERROR("cJSON_Parse failed\n");
        return NULL;
    }

    cJSON *dcJSON = cJSON_GetObjectItem(json_root, "d");
    if (!dcJSON) {
        cJSON_Delete(json_root);
        return NULL;
    }

    cJSON *base_urlcJSON = cJSON_GetObjectItem(dcJSON, "bu");
    if (!base_urlcJSON) {
        cJSON_Delete(json_root);
        IOTC_ERROR("missing baseurl\n");
        return NULL;
    }

    IotclDiscoveryResponse *response = (IotclDiscoveryResponse *) calloc(1, sizeof(IotclDiscoveryResponse));
    if (!response) {
        goto cleanup;
    }

    { // separate the declaration into a block to allow jump without warnings
        char *jsonBaseUrl = base_urlcJSON->valuestring;
        if (!jsonBaseUrl) {
            IOTC_ERROR("jsonBaseUrl is NULL\n");
            goto cleanup;
        }

        response->url = iotcl_strdup(jsonBaseUrl);
        if (!response->url) {
            IOTC_ERROR("iotcl_strdup failed\n");
            goto cleanup;
        }

        if (split_url(response)) {
            cJSON_Delete(json_root);
            return response;
        } // else cleanup and return null

    }

    cleanup:
    cJSON_Delete(json_root);
    iotcl_discovery_free_discovery_response(response);

    IOTC_ERROR("iotcl_discovery_parse_discovery_response failed\n");
    return NULL;
}

void iotcl_discovery_free_discovery_response(IotclDiscoveryResponse *response) {
    if (response) {
        free(response->url);
        free(response->host);
        free(response->path);
        free(response);
    }
}

IotclSyncResponse *iotcl_discovery_parse_sync_response(const char *response_data) {

    IotclSyncResponse *response = (IotclSyncResponse *) calloc(1, sizeof(IotclSyncResponse));
    if (NULL == response) {
        return NULL;
    }
    cJSON *sync_json_root = cJSON_Parse(response_data);
    if (!sync_json_root) {
        response->ds = IOTCL_SR_PARSING_ERROR;
        return response;
    }
    cJSON *sync_res_json = cJSON_GetObjectItemCaseSensitive(sync_json_root, "d");
    if (!sync_res_json) {
        cJSON_Delete(sync_json_root);
        response->ds = IOTCL_SR_PARSING_ERROR;
        return response;
    }

    if (response->ds == IOTCL_SR_PARSING_ERROR) {
        response->ds = IOTCL_SR_PARSING_ERROR;
    } else {
        response->ds = IOTCL_SR_OK;
    }

    if (response->ds == IOTCL_SR_OK) {
        cJSON *meta_json = cJSON_GetObjectItemCaseSensitive(sync_res_json, "meta");
		if (!meta_json) {
		    cJSON_Delete(sync_json_root);
		    response->ds = IOTCL_SR_PARSING_ERROR;
		    return response;
		}

		response->cd = safe_get_string_and_strdup(meta_json, "cd");

		if (!response->cd) {
            cJSON_Delete(sync_json_root);
            response->ds = IOTCL_SR_PARSING_ERROR;
            return response;
		}

        cJSON *p = cJSON_GetObjectItemCaseSensitive(sync_res_json, "p");

        if (p) {

            response->broker.name = safe_get_string_and_strdup(p, "n");
            response->broker.client_id = safe_get_string_and_strdup(p, "id");
            response->broker.host = safe_get_string_and_strdup(p, "h");
            response->broker.user_name = safe_get_string_and_strdup(p, "un");
            response->broker.port = get_numeric_value_or_default(p, "p", 8883);

#if 0
            response->broker.pass = safe_get_string_and_strdup(p, "pwd");

            cJSON *topics = cJSON_GetObjectItemCaseSensitive(p, "topics");

            response->broker.sub_topic = safe_get_string_and_strdup(topics, "c2d");
            response->broker.pub_topic = safe_get_string_and_strdup(topics, "rpt");
#endif

            if (
                    !response->cpid ||
                    !response->broker.host ||
                    !response->broker.client_id ||
#if 0
					!response->broker.pass ||   // FIXME: password field in discovery response, not sync response.  password may actually be null or empty
					!response->broker.sub_topic ||
                    !response->broker.pub_topic ||
#endif
					!response->broker.user_name
            ) {
                // Assume parsing error, but it could also be (unlikely) allocation error
                response->ds = IOTCL_SR_PARSING_ERROR;
            }
        } else {
            response->ds = IOTCL_SR_PARSING_ERROR;
        }
    } else {
        switch (response->ds) {
            case IOTCL_SR_DEVICE_NOT_REGISTERED:
            case IOTCL_SR_UNKNOWN_DEVICE_STATUS:
            case IOTCL_SR_AUTO_REGISTER:
            case IOTCL_SR_DEVICE_NOT_FOUND:
            case IOTCL_SR_DEVICE_INACTIVE:
            case IOTCL_SR_DEVICE_MOVED:
            case IOTCL_SR_CPID_NOT_FOUND:
                // all fall through
                break;
            default:
                response->ds = IOTCL_SR_UNKNOWN_DEVICE_STATUS;
                break;
        }
    }
    // we have duplicated strings, so we can now free the result
    cJSON_Delete(sync_json_root);
    return response;
}

void iotcl_discovery_free_sync_response(IotclSyncResponse *response) {
    if (!response) {
        return;
    }
    free(response->cpid);
    free(response->cd);
    free(response->dtg);
    free(response->broker.host);
    free(response->broker.client_id);
    free(response->broker.user_name);
    free(response->broker.pass);
    free(response->broker.name);
    free(response->broker.sub_topic);
    free(response->broker.pub_topic);
    free(response);
}
