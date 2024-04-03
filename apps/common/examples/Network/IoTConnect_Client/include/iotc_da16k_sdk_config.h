#ifndef IOTC_DA16K_SDK_CONFIG_H
#define IOTC_DA16K_SDK_CONFIG_H

/* Config for enabling printing on DA16k */

#include "common_def.h"

#define IOTC_ENDLN "\r\n"

#define IOTC_ERROR(...) \
    do { \
        PRINTF(__VA_ARGS__); PRINTF(IOTC_ENDLN); \
    } while(0)

#define IOTC_WARN(...) \
    do { \
        PRINTF(__VA_ARGS__); PRINTF(IOTC_ENDLN); \
    } while(0)

#define IOTC_INFO(...) \
    do { \
        PRINTF(__VA_ARGS__); PRINTF(IOTC_ENDLN); \
    } while(0)

#endif // IOTC_DA16K_SDK_CONFIG_H
