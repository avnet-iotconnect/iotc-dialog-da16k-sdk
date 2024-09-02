//
// Copyright: Avnet 2024
// Created by E. Voirin on 8/21/24.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "atcmd.h"
#include "iotconnect.h"
#include "iotcl.h"
#include "iotcl_telemetry.h"
#include "iotc_app.h"

typedef enum e_iotc_data_type {
    IOTC_AT_STRING = 0,
    IOTC_AT_BOOL,
    IOTC_AT_FLOAT32,
    IOTC_AT_FLOAT64,
    IOTC_AT_DATA_TYPE_COUNT
} iotc_at_data_type;

typedef union {
    bool        d_bool;
    float       d_float32;
    double      d_float64;
} iotc_at_data;

static inline bool iotc_at_asciihex_byte_decode (uint8_t *out, const char *hex) {
    /*  this is used from within here only and we can expect the parameter to be valid 
        and contain at least 2 characters */
    char to_decode[] = { hex[0], hex[1], '\0' };
    char *endptr = NULL;

    *out = (uint8_t) strtol(to_decode, &endptr, 16);

    /* Conversion was successful if the end is after two characters */
    return endptr == &to_decode[2];
}

static bool iotc_at_decode_hex_string(void *out, const char *hex, size_t length) {
    uint8_t *dst = (uint8_t *) out + length - 1;

    if (out == NULL || hex == NULL || strlen(hex) != length * 2) {
        return false;
    }

    while (dst >= (uint8_t *) out) {
        if (!iotc_at_asciihex_byte_decode(dst, hex)) {
            return false;
        }

        dst -= 1;
        hex += 2;
    };

    return true;
}

static void iotc_print_at_data(iotc_at_data_type type, iotc_at_data data) {
    switch (type) {
        case IOTC_AT_BOOL:
            PRINTF("%s: Type: %d, data: %u\n", __func__, (int) type, (unsigned) data.d_bool);
            break;
        case IOTC_AT_FLOAT32:
            PRINTF("%s: Type: %d, data: %f\n", __func__, (int) type, data.d_float32);
            break;
        case IOTC_AT_FLOAT64:
            PRINTF("%s: Type: %d, data: %f\n", __func__, (int) type, data.d_float64);
            break;
        default:
            return;
    }
}

int iotc_at_nwicexmsg(int argc, char *argv[]) {
    int ret = AT_CMD_ERR_CMD_OK;

    // Tuple size is 3 (type, key, value)
    // Argument count (except first, which is the command itself) must be cleanly divisible by 3
    // And we must have at least one tuple.

    PRINTF("%s argc %d\n", __func__, argc);

    for (int i = 0; i < argc; i++) {
        PRINTF("%s: argv[%d] = '%s'\n", __func__, i, argv[i]);
    }

    if ((argc - 1) % 3 != 0 || argc < 4) {
        return AT_CMD_ERR_INSUFFICENT_ARGS;
    }

    IotclMessageHandle telemetry = iotcl_telemetry_create();
    bool success = true;

    for (size_t idx = 1; idx < (size_t) argc; idx += 3) {
        iotc_at_data_type   type    = atoi(argv[idx + 0]);
        const char         *key     = argv[idx + 1];
        const char         *value   = argv[idx + 2];
        iotc_at_data        data;

        switch (type) {
            case IOTC_AT_STRING:
                success &= (IOTCL_SUCCESS == iotcl_telemetry_set_string(telemetry, key, value));
                break;
            case IOTC_AT_BOOL:
                success &= iotc_at_decode_hex_string(&data, value, sizeof(uint8_t));
                success &= (IOTCL_SUCCESS == iotcl_telemetry_set_bool(telemetry, key, data.d_bool));
                break;
            case IOTC_AT_FLOAT32:
                success &= iotc_at_decode_hex_string(&data, value, sizeof(float));
                success &= (IOTCL_SUCCESS == iotcl_telemetry_set_number(telemetry, key, data.d_float32));
                break;
            case IOTC_AT_FLOAT64:
                success &= iotc_at_decode_hex_string(&data, value, sizeof(double));
                success &= (IOTCL_SUCCESS == iotcl_telemetry_set_number(telemetry, key, data.d_float64));
                break;
            default:
                success = false;
                PRINTF("%s: Unknown data type received in message.\n", __func__);
                break;
        }

        if (success) {
            iotc_print_at_data(type, data);
        }
    }

    if (!success) {
        PRINTF("%s: Failed to create telemetry object from parameters.\n", __func__);
        ret = AT_CMD_ERR_WRONG_ARGUMENTS;
    } else if (iotcl_mqtt_send_telemetry(telemetry, false) != IOTCL_SUCCESS) {
        PRINTF("%s: Failed to send telemetry.\n", __func__);
        ret = AT_CMD_ERR_UNKNOWN;
    } else {
        ret = AT_CMD_ERR_CMD_OK;
        PRINTF_ATCMD("\r\n+NWICEXCMD:1");
    }

    iotcl_telemetry_destroy(telemetry);

    return ret;
}
