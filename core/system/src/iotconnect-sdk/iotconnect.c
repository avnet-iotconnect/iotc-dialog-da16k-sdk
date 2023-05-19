#include <strong.h>
#include "iotconnect.h"

static IotConnectClientConfig config;
IotConnectClientConfig *iotconnect_sdk_init_and_get_config(void) {
    memset(&config, 0, sizeof(config));
    return &config;
}

int iotconnect_sdk_init(void) {

}
