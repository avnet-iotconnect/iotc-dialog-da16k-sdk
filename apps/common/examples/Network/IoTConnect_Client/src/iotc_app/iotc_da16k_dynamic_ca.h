/* 
    Dynamic certificate store for IoTC on DA16K.

    This was created since Renesas reference code doesn't support sourcing certs from anywhere other than flash.
   
    Places that load root certs need to use the getters, IoTConnect uses the setter when the connection is initialized.

    Device certs/keys still get loaded from flash.

    Author: evoirin
    (C) Avnet 2024
*/

#ifndef  _IOTC_DA16K_DYNAMIC_CA_H_
#define  _IOTC_DA16K_DYNAMIC_CA_H_

#include "iotconnect.h"

/* Sets the appropriate MQTT/HTTP CAs according to connection type. */
void        iotc_da16k_dynamic_ca_set       (IotConnectConnectionType type);
/* Clears the internal CA assignments. */
void        iotc_da16k_dynamic_ca_clear     (void);

/* Get MQTT Root CA. Returns NULL if unconfigured. In this case, fall back to flash. */
const char *iotc_da16k_dynamic_ca_mqtt_get  (void);
/* Get HTTP Root CA. Returns NULL if unconfigured. In this case, fall back to flash. */
const char *iotc_da16k_dynamic_ca_http_get  (void);

#endif /* _IOTC_DA16K_DYNAMIC_CA_H_ */