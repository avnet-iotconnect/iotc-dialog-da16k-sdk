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

// Just to give some scope for "expansion" -- update this if AT commands get added/updated
#define IOTC_AT_VERSION "1.1.0"

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

/* Every function has a variable called 'ret' which indicates the result of the command. This macro prints the <command>:1 at the end if successful. */
#define IOTC_AT_PRINT_ON_SUCCESS_IF_STRING_NOT_EMPTY(str) { if (ret == AT_CMD_ERR_CMD_OK && strlen(str) > 0) { PRINTF_ATCMD("\r\n%S:%s", argv[0] + 2, str); } }

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

/* Internal getter / setter implementation, for code deduplication */
static int iotc_at_generic_getter_setter(int argc, char *argv[], int config_value_name) {
    int     ret             = AT_CMD_ERR_CMD_OK;
    char    result_str[160] = {0, };

    if (argc == 1 || is_correct_query_arg(argc, argv[1])) {
        /* Getter AT+NWICDUID=? */
        if (da16x_get_config_str(config_value_name, result_str) != CC_SUCCESS) {
            ret = AT_CMD_ERR_NO_RESULT;
        }
    } else if (argc == 2) {
        /* Setter AT+NWICDUID=<duid> */
        if (da16x_set_config_str(config_value_name, argv[1]) != CC_SUCCESS) {
            ret = AT_CMD_ERR_COMMON_ARG_LEN;
        }
    } else {
        ret = AT_CMD_ERR_INSUFFICENT_ARGS;
    }

    IOTC_AT_PRINT_ON_SUCCESS_IF_STRING_NOT_EMPTY(result_str);

    return ret;
}

int iotc_at_nwicduid(int argc, char *argv[]) {
    return iotc_at_generic_getter_setter(argc, argv, DA16X_CONF_STR_IOTCONNECT_DUID);
}

int iotc_at_nwiccpid(int argc, char *argv[]) {
    return iotc_at_generic_getter_setter(argc, argv, DA16X_CONF_STR_IOTCONNECT_CPID);
}

int iotc_at_nwicenv(int argc, char *argv[]) {
    return iotc_at_generic_getter_setter(argc, argv, DA16X_CONF_STR_IOTCONNECT_ENV);
}

int iotc_at_nwicat(int argc, char *argv[]) {
    return iotc_at_generic_getter_setter(argc, argv, DA16X_CONF_STR_IOTCONNECT_AUTH_TYPE);
}

int iotc_at_nwicsk(int argc, char *argv[]) {
    /* Symmetric key is special because based64 padding seems to break the AT command parsing, so we swap trailing '-' for '=' */
    if (argc == 2) {
        char *arg = argv[1];
        int len = (int) strlen(arg);

        if (len >= 1 && arg[len-1] == '-')  arg[len-1] = '=';
        if (len >= 2 && arg[len-2] == '-')  arg[len-2] = '=';
        if (len >= 3 && arg[len-3] == '-')  arg[len-3] = '=';
    }

    return iotc_at_generic_getter_setter(argc, argv, DA16X_CONF_STR_IOTCONNECT_SYMMETRIC_KEY);
}

int iotc_at_nwicct(int argc, char *argv[]) {
    return iotc_at_generic_getter_setter(argc, argv, DA16X_CONF_STR_IOTCONNECT_CONNECTION_TYPE);
}

/* Internal function call wrapper, for code deduplication */
typedef void (*iotc_at_function)(void);
static inline int iotc_at_generic_function_call(int argc, char *argv[], iotc_at_function func) {
    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);
    func();
    return AT_CMD_ERR_CMD_OK;
}

int iotc_at_nwicsetup(int argc, char *argv[]) {
    return iotc_at_generic_function_call(argc, argv, setup_iotconnect);
}
int iotc_at_nwicstart(int argc, char *argv[]) {
    return iotc_at_generic_function_call(argc, argv, start_iotconnect);
}
int iotc_at_nwicstop(int argc, char *argv[]) {
    return iotc_at_generic_function_call(argc, argv, stop_iotconnect);
}
int iotc_at_nwicreset(int argc, char *argv[]) {
    return iotc_at_generic_function_call(argc, argv, reset_iotconnect);
}

/* Classic NWICMSG, with no dedicated type parameter and regular value conversion from string */
int iotc_at_nwicmsg(int argc, char *argv[]) {
    IotclMessageHandle  telemetry   = NULL;
    int                 ret         = AT_CMD_ERR_CMD_OK;
    bool                success     = true;

    PRINTF("%s argc %d\n", __func__, argc);

    for (int i = 0; i < argc; i++) {
        PRINTF("%s: argv[%d] = '%s'\n", __func__, i, argv[i]);
    }

    // Tuple size is 2 (key, value)
    // Argument count (except first, which is the command itself) must be cleanly divisible by 2
    // And we must have at least one tuple.

    if ((argc - 1) % 2 != 0 || argc < 3) {
        return AT_CMD_ERR_INSUFFICENT_ARGS;
    }

    telemetry = iotcl_telemetry_create();

    for (size_t idx = 1; idx < (size_t) argc; idx += 2) {
        const char         *key     = argv[idx + 0];
        const char         *value   = argv[idx + 1];

        success &= (IOTCL_SUCCESS == iotcl_telemetry_set_string(telemetry, key, value));
    }

    if (!success) {
        PRINTF("%s: Failed to create telemetry object from parameters.\n", __func__);
        ret = AT_CMD_ERR_WRONG_ARGUMENTS;
    } else if (iotcl_mqtt_send_telemetry(telemetry, false) != IOTCL_SUCCESS) {
        PRINTF("%s: Failed to send telemetry.\n", __func__);
        ret = AT_CMD_ERR_UNKNOWN;
    } else {
        ret = AT_CMD_ERR_CMD_OK;
    }

    iotcl_telemetry_destroy(telemetry);

    IOTC_AT_PRINT_ON_SUCCESS_IF_STRING_NOT_EMPTY("1");
    return ret;
}

/* Extended NWICEXMSG, with dedicated type parameter and hex decoding */
int iotc_at_nwicexmsg(int argc, char *argv[]) {
    IotclMessageHandle  telemetry   = NULL;
    int                 ret         = AT_CMD_ERR_CMD_OK;
    bool                success     = true;

    PRINTF("%s argc %d\n", __func__, argc);

    for (int i = 0; i < argc; i++) {
        PRINTF("%s: argv[%d] = '%s'\n", __func__, i, argv[i]);
    }

    // Tuple size is 3 (type, key, value)
    // Argument count (except first, which is the command itself) must be cleanly divisible by 3
    // And we must have at least one tuple.

    if ((argc - 1) % 3 != 0 || argc < 4) {
        return AT_CMD_ERR_INSUFFICENT_ARGS;
    }

    telemetry = iotcl_telemetry_create();

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
    }

    iotcl_telemetry_destroy(telemetry);

    IOTC_AT_PRINT_ON_SUCCESS_IF_STRING_NOT_EMPTY("1");
    return ret;
}

int iotc_at_nwicver(int argc, char *argv[]) {
	DA16X_UNUSED_ARG(argc);
    PRINTF_ATCMD("\r\n%S:" IOTC_AT_VERSION, argv[0] + 2);
    return AT_CMD_ERR_CMD_OK;
}

int iotc_at_nwicgetcmd(int argc, char *argv[]) {
    iotc_command_queue_item item    = {0};
    int                     ret     = AT_CMD_ERR_CMD_OK;

    DA16X_UNUSED_ARG(argc);
    DA16X_UNUSED_ARG(argv);

    if (iotc_command_queue_item_get(&item) == false || item.command == NULL) {
        ret = AT_CMD_ERR_NO_RESULT;
    }

    IOTC_AT_PRINT_ON_SUCCESS_IF_STRING_NOT_EMPTY(item.command);

    iotc_command_queue_item_destroy(item);

    return ret;
}
