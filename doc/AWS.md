To connect to AWS more parameters need to be set for the MQTT client.

An example setup is shown below:

- net
- cert write ca1
- cert write cert1
- cert write key1
- iotconnect_config duid DEVICE_ID
- iotconnect_config cpid DEVICE_CPID // from the key vault
- iotconnect_config cd DEVICE_CD // from connection info
- iotconnect_config env dummy
- iotconnect_config auth_type 2
- mqtt_config broker DEVICE_MQTT_BROKER_HOSTNAME // from the key vault and from connection info
- mqtt_config username DEVICE_MQTT_BROKER_USERNAME // from connection info
- mqtt_config password "" // no password
- mqtt_config pub_topic devices/DEVICE_ID/messages/events/
- mqtt_config sub_topic_del
- mqtt_config sub_topic_add iot/DEVICE_ID/cmd
