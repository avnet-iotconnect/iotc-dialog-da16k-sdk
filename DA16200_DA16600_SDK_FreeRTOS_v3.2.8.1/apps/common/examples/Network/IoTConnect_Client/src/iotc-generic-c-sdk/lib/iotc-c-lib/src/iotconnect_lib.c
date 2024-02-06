/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "iotconnect_common.h"
#include "iotconnect_lib.h"

static IotclConfig config;
static bool config_is_valid = false;

// for iotconnect azure
bool iotcl_init(IotclConfig *c) {
    IOTC_DEBUG("%s\n", __func__);
    iotcl_deinit();
    if (
            !c || !c->device.env || !c->device.cpid || !c->device.duid ||
            0 == strlen(c->device.env) || 0 == strlen(c->device.cpid) || 0 == strlen(c->device.duid)
            ) {
        IOTC_ERROR ("IotConnectLib_Configure: configuration parameters missing");
        return false;
    }
    if (strlen(c->device.cpid) + 1 /* dash, separator */  + strlen(c->device.duid) > MAX_DEVICE_COMBINED_NAME) {
        IOTC_ERROR ("IotConnectLib_Configure: combined name (cpid + uuid) exceeded maximum value");
        return false;
    }
    memcpy(&config, c, sizeof(config));

    if (!config.device.duid || !config.device.cpid || !config.device.env) {
        // allocation failure
        IOTC_ERROR ("IotConnectLib_Configure: malloc failure");
        iotcl_deinit();
        return false;
    }
    config_is_valid = true;
    return true;
}

// for iotconnect aws
bool iotcl_init_v2(IotclConfig *c) {
    iotcl_deinit();
    if (!c || !c->device.duid || 0 == strlen(c->device.duid) || !c->telemetry.cd || 0 == strlen(c->telemetry.cd)) {
        IOTC_ERROR ("IotConnectLib_Configure: configuration parameters missing");
        return false;
    }
    if (1 /* dash, separator */  + strlen(c->device.duid) > MAX_DEVICE_COMBINED_NAME) {
        IOTC_ERROR ("IotConnectLib_Configure: combined name exceeded maximum value");
        return false;
    }
    memcpy(&config, c, sizeof(config));

    if (!config.device.duid || !config.telemetry.cd) {
        // allocation failure
        IOTC_ERROR ("IotConnectLib_Configure: malloc failure");
        iotcl_deinit();
        return false;
    }
    config_is_valid = true;
    return true;
}

IotclConfig *iotcl_get_config(void) {
    IOTC_DEBUG("%s\n", __func__);
    if (!config_is_valid) {
        IOTC_ERROR ("!config_is_valid");
        return NULL;
    }
    return &config;
}

void iotcl_deinit(void) {
    IOTC_DEBUG("%s\n", __func__);
    config_is_valid = false;

    memset(&config, 0, sizeof(config));
}
