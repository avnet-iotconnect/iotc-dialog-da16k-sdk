# Renesas DA16xxx IoTConnect AT Command Set

## Accessing the AT Command Console

### DA16200MOD EVK and DA16600MOD EVK

The EVK boards provide two serial ports via the USB connector.

The debug console is on the lower port (at 230400 baud).

The **AT console is on the higher port (at 115200 baud)** -- note that it **DOES NOT** echo by default.

### DA16200PMOD and DA16600PMOD

The AT console (at 115200 baud) is accessed via the PMOD connector pins.

You may wish to (in a production) connect an embedded device with a PMOD connector to send commands to the DA16K.

For debugging purposes it is desirable to access the AT console to send commands by hand.

The following example shows how to connect an FTDI-based USB-to-Serial adapter to the PMOD connector using the Type 3A Extended UART pin layout.

***Note: Colors on the cable differ from the [Quickstart Guide](./QUICKSTART.md).***

![](assets/IMG_20230822_122024106.jpg)

## AT Commands

Type
```
HELP
```
or
```
?
```
to see the list of AT commands supported

Command names tend to start with “AT” or “AT+” – for “attention”, “AT+NW” is for network related commands – “AT+NWIC” has been used as a prefix for IotConnect commands.

Commands can be sent with/without an “=” and parameters are separated by commas, e.g.
```
AT+XXXXX option1,option2
```
or
```
AT+XXXXX=option2,option2
```
Note: “send” means type and hit return key (maybe carriage return / linefeed).

### Echo

The AT commands are not normally echo’d back to the user – send the “ATE” command to enable the commands to be echo’d so that they are visible when typed into e.g. the DA16200MOD EVK second terminal.

## Notes on certificate setup

Certificates are set using the standard mechanisms on the DA16xxx.

See the [Application Setup guide](SETUP_APP.md) for details.


## IoTConnect parameter setup
### Connection Type (Azure / AWS)
#### Set Connection Type
To set the connection type send an `AT+NWICCT` command with the connection type, e.g.
```
AT+NWICCT 1
```
#### Get Connection Type
To get the conneciton type send an empty `AT+NWICCT` command, i.e.
```
AT+NWICCT
```
Successful completion should report, e.g.
```
+NWICCT:1
OK
```

### DUID (device ID)
#### Set DUID

To set the device id (DUID) send an `AT+NWICDUID` command with the device name, e.g.
```
AT+NWICDUID test
```
Successful completion should report
```
OK
```
#### Get DUID
To get the device id send an empty `AT+NWICDUID` command, i.e.
```
AT+NWICDUID
```
Successful completion should report, e.g.
```
+NWICDUID:test
OK
```
#### NVRAM
The DUID value is stored in nvram in `IOTC_DUID`

### CPID
#### Set CPID

To set the CPID send an `AT+NWICCPID` command with the CPID, e.g.
```
AT+NWICCPID testcpid
```
Successful completion should report
```
OK
```
#### Get CPID
To get the CPID send an empty `AT+NWICPID` command, i.e.
```
AT+NWICDUID
```
Successful completion should report, e.g.
```
+NWICCPID:testcpid
OK
```
#### NVRAM
The CPID value is stored in nvram in `IOTC_CPID`

Note: the CPID value should match the "cpid" field in the discovery/sync response JSON.

### ENV
#### Set ENV

To set the ENV send an `AT+NWICENV` command with the ENV, e.g.
```
AT+NWICENV testenv
```
Successful completion should report
```
OK
```
#### Get ENV
To get the device id send an empty `AT+NWICENV` command, i.e.
```
AT+NWICENV
```
Successful completion should report, e.g.
```
+NWICENV:testenv
OK
```
#### NVRAM  
The ENV value is stored in nvram in `IOTC_ENV`

### AUTHENTICATION TYPE
#### Set AUTHENTICATION TYPE
To set the (expected) authentication type send an `AT+NWICAT` command with the (expected) authentication type, e.g.
```
AT+NWICAT 1
```
Values are numeric and defined as follows:
- TOKEN = 1,
- X509 = 2,
- Self signed X509 = 2 (may become 3 in the future)
- TPM = 4 (UNSUPPORTED)
- SYMMETRIC_KEY = 5

Note: these values should match the `IOTC_AT` enum in the code, and the "at" field in the discovery/sync response JSON.

Successful completion should report
```
OK
```
#### Get AUTHENTICATION TYPE
To get the device id send an empty `AT+NWICAT` command, i.e.
```
AT+NWICAT
```
Successful completion should report, e.g.
```
+NWICAT:1
OK
```
#### NVRAM
The authentication type value is stored in nvram in `IOTC_AUTH_TYPE`

### SYMMETRIC KEY
This is only required if the authentication type is `IOTC_AT_SYMMETRIC_KEY` -- otherwise, leave blank. 

Due to issues with parsing AT commands, any base64 encoding padding characters, i.e. “=” must be replaced by “-” when sending the symmetric key.
Note: the “-” characters from the AT command will be silently converted back to “=”.

#### Set SYMMETRIC KEY
Set the base64 encoded symmetric key using, e.g.
```
AT+NWICSK 0123456789abcdef-
```
Successful completion should report
```
OK
```
#### Get SYMMETRIC KEY
To get the symmetric key send an empty `AT+NWICSK` command, i.e.
```
AT+NWICDUID
```
Successful completion should report, e.g.
```
+NWICSK:0123456789abcdef=
OK
```
#### NVRAM
The symmetric key value is stored in nvram in `IOTC_SYMMETRIC_KEY`

### Changing IoTConnect values

Note the values of DUID, CPID, ENV, authentication type and symmetric key used will be those at the point that the `AT+NWICSETUP` is sent.

Changing values of DUID, CPID, ENV, authentication type or symmetric key once `AT+NWICSETUP` has been sent will not cause those new values to be used until the `AT+NWICSETUP` is (re)run.

## IoTConnect AT Commands
These are version 1.0.0 commands, described in this document.

### VERSION
Note: the value is compiled into the executable from the #define IOTCONNECT_AT_VERSION.

#### Get VERSION
To get the version of the AT commands, send an empty AT+NWICVER command, i.e.

AT+NWICVER

Successful completion should report, e.g.

+NWICVER:1.0.0
OK

## IoTConnect lifecycle

### SETUP
To setup IoTConnect (asynchronously) send
```
AT+NWICSETUP
```
Successful completion for requesting an asynchronous setup should report
```
OK
```
Since the process is asynchronous, the “true” status of the setup process must be monitored.
#### Notifications
When the IoTConnect setup begins, there will be a:
```
+NWICSETUPBEGIN:
```
When the IoTConnect setup ends successfully, there will be a:
```
+NWICSETUPEND:1
```
When the IoTConnect setup fails, there will be a:
```
+NWICSETUPEND:0
```
### Changing IoTConnect values
To use new values of DUID, CPID, ENV, authentication type or symmetric key - any started IoTConnect process must be stopped or reset and then setup again, to force the configuration to be (re)read

### START
To start the IoTConnect discovery/sync/MQTT process (asynchronously) send
```
AT+NWICSTART
```
Successful completion for requesting an asynchronous start should report
```
OK
```
#### Notifications
Since the process is asynchronous, the “true” status of the start must be monitored.

When IoTConnect start process begins, there will be a:
```
+NWICSTARTBEGIN:
```
When IoTConnect start process ends successfully, there will be a:
```
+NWICSTARTEND:1
```
When IoTConnect start process fails, there will be a:
```
+NWICSTARTEND:0
```
### STOP

To stop IoTConnect (asynchronously) send
```
AT+NWICSTOP
```
Successful completion for requesting an asynchronous stop should report
```
OK
```
#### Notifications
Since the process is asynchronous, the “true” status of the setup process must be monitored.

When the IoTConnect stop begins, there will be a:
```
+NWICSTOPBEGIN:
```
When the IoTConnect stop ends successfully, there will be a:
```
+NWICSTOPEND:1
```
When the IoTConnect stop fails, there will be a:
```
+NWICSTOPEND:0
```

When IoTConnect is stopped it will disconnect from the IoTConnect dashboard.

It is possible to start IoTConnect once it has been stopped – and original configuration and discovery/sync values will be reused.

### RESET

To reset IoTConnect (asynchronously) send
```
AT+NWICRESET
```
Successful completion for requesting an asynchronous reset should report
```
OK
```
#### Notifications
Since the process is asynchronous, the “true” status of the reset process must be monitored.

When the IoTConnect reset begins, there will be a:
```
+NWICRESETBEGIN:
```
When the IoTConnect reset ends successfully, there will be a:
```
+NWICRESETEND:1
```
When the IoTConnect reset fails, there will be a:
```
+NWICRESETEND:0
```
When IoTConnect is reset it will disconnect from the IoTConnect dashboard.

It is not possible to start IoTConnect once it has been reset – the configuration must be setup and discovery/sync values will be retrieved again.

### MESSAGE

To send an “IoTConnect format” MQTT message semd AT+NWICMSG followed by a series of up to 7 name/value pairs, e.g.
```
AT+NWICMSG name1,value1,name2,value2
```
Note: values are treated as strings even if appear numeric – but shouldn’t affect the JSON format of the messages – and IoTConnect dashboard will still show them as a numeric value.

Successful completion should report, e.g.
```
OK
+NWMQMSGSND:1
```

## C2D commands and OTA messages

By default C2D commands and OTA messages will be handled (as failures) automatically.

## Advanced Usage

### Turning on Support for AT Command handling

To avoid the implementer having to react to (unexpected) asynchronous messages devices that are not interested in supporting AT commands can simply ignore them (providing that the IOTC_USE_CMDACK environment variable is not set).

Devices that are interested in supporting AT commands, parsing and acting on them, and acknowledging their success/failure can use the
```
AT+NWICUSECMDACK 1
```
to indicate that support is required for IoTConnect (asynchronous) commands and command-acknowledgement, i.e. +NWICCMD.

If a device is no longer interested in supporting AT commands, parsing and acting on them, and acknowledging their success/failure can use the
```
AT+NWICUSECMDACK 0
```
that ensures that a default “failure” acknowledgement is sent back to IoTConnect – and no IoTConnect asynchronous command information will be sent, i.e. no +NWICCMD will be sent.

Note: can still see a “routine parameter” command on AT connection as:
```
+NWMQMSG:{"cmdType":"0x01","data":{"cpid":"avtds","guid":"a4bfac5f-2c07-4822-a8bc-9b7dad451d9e","uniqueId":"justatoken","command":"routine parameter","ack":true,"ackId":"aaafdaa9-005d-463f-89af-c2eddc2df368","cmdType":"0x01"}},devices/avtds-justatoken/messages/devicebound/%24.to=%2Fdevices%2Favtds-justatoken%2Fmessages%2FdeviceBound,211
```
i.e. via the normal mqtt_client mechanism(s).

### AT Command

Assuming that AT+NWICUSECMDACK 1then an “AT”-friendly message will output the routine (and parameter) (swapping spaces for commas).
Note: the type and ackId are both needed to acknowledge the command, note: type is 1, i.e. first parameter in example
```
+NWICCMD:1,aaafdaa9-005d-463f-89af-c2eddc2df368,routine,parameter
```
on success, or
```
+NWICCMDFAIL
```
on (internal) failure.

### AT Command Acknowledgement

To acknowledge an asynchronous command:

If the execution was successful, then:
```
AT+NWICCMDACK type,ackId,1,message
```
and if unsuccessful
```
AT+NWICCMDACK type,ackId,0,message
```

AT+NWICCMDACK allows the implementer to react well to +NWICCMD messages.

It’s possible that the type and ackId might get misaligned – if use wrong AT commands – but server will have to be aware.

Can always send an acknowledgement.

## Turning on Support for OTA handling

To avoid the implementer having to react to (unexpected) asynchronous OTA requests devices that are not interested in supporting AT commands can simply ignore them (providing that the IOTC_USE_OTAACK environment variable is not set).

Devices that are interested in supporting AT commands, parsing and acting on them, and acknowledging their success/failure can use the
```
AT+NWICUSEOTAACK 1
```
to indicate that support is required for IoTConnect (asynchronous) OTA requests and command-acknowledgement, i.e. +NWICOTA.

If a device is no longer interested in supporting AT commands, parsing and acting on them, and acknowledging their success/failure can use the
```
AT+NWICUSEOTAACK 0
```
that ensures that a default “OTA failure” acknowledgement is sent back to IoTConnect – and no IoTConnect asynchronous OTA will be sent, i.e. no +NWICOTA will be sent.

Note: can still see a “routine parameter” command on AT connection as:
```
+NWMQMSG:{"cmdType":"0x02","data":{"cpid":"avtds","guid":"a4bfac5f-2c07-4822-a8bc-9b7dad451d9e","uniqueId":"justatoken","command":"ota","ack":false,"ackId":"59ad4c32-7de1-4de9-bb2b-a991614614db","cmdType":"0x02","ver":{"sw":"1.0","hw":"1.0"},"urls":[{"url":"https://pociotconnectblobstorage.blob.core.windows.net/firmware/3710FE82-2C21-4CDA-8EE8-A4AF34E019F5.txt?sv=2018-03-28&sr=b&sig=lLeOanorGfQ7rFerwg%2FhBF7lDUfmfBplgOMecICrvVM%3D&se=2023-07-28T13%3A36%3A32Z&sp=r","tag":""}]}},devices/avtds-justatoken/messages/devicebound/%24.to=%2Fdevices%2Favtds-justatoken%2Fmessages%2FdeviceBound,472
```
i.e. via the normal mqtt_client mechanism(s).

### AT Command

Assuming that AT+NWICUSEOTAACK 1then an “AT”-friendly message will output the ack_id, version and url information.
Note: the ackId is needed to acknowledge the command, note: there is no type, as an OTA message only has one possible type value
```
+NWICOTA:59ad4c32-7de1-4de9-bb2b-a991614614db,1.0,https://pociotconnectblobstorage.blob.core.windows.net/firmware/3710FE82-2C21-4CDA-8EE8-A4AF34E019F5.txt?sv=2018-03-28&sr=b&sig=lLeOanorGfQ7rFerwg%2FhBF7lDUfmfBplgOMecICrvVM%3D&se=2023-07-28T13%3A36%3A32Z&sp=r
```
on success, or

+NWICOTAFAIL

on (internal) failure.

### AT OTA Acknowledgement

To acknowledge an asynchronous OTA:

If the execution was successful, then:

AT+NWICOTAACK ackId,1,message

and if unsuccessful

AT+NWICOTAACK ackId,0,message

AT+NWICOTAACK allows the implementer to react well to +NWICOTA messages.

Can always send an acknowledgement.

## Non-Standard Usage

### NVRAM

### Example set of NVRAM variables

```
# printenv

Total length (1159)
country_code (STR,03) ........ UK
tls_ver (STR,04) ............. 769
peap_ver (STR,02) ............ 1
N0_Profile (STR,02) .......... 1
N0_ssid (STR,12) ............. "NETGEAR83"
N0_psk (STR,14) .............. "*************"
N0_key_mgmt (STR,08) ......... WPA-PSK
N0_proActKeyCaching (STR,02) . 0
N0_auth_max_failures (STR,02)  3
SYSMODE (STR,02) ............. 0
SNTP_RUN_FLAG (STR,02) ....... 1
SNTP_PERIOD (STR,07) ......... 129600
SNTP_SVR (STR,13) ............ http://pool.ntp.org 
SNTP_SVR_1 (STR,15) .......... 1.pool.ntp.org
SNTP_SVR_2 (STR,15) .......... http://2.pool.ntp.org 
TZONE (STR,05) ............... 3600
IOTC_CPID (STR,06) ........... avtds
IOTC_ENV (STR,09) ............ avnetpoc
IOTC_AUTH_TYPE (STR,02) ...... 1
IOTC_DUID (STR,11) ........... justatoken
IOTC_SYMMETRIC_KEY (STR,01) ..
IOTC_CONNECTION_TYPE (STR,02)  1
MQTT_BROKER (STR,48) ......... poc-iotconnect-iothub-030-eu2.azure-devices.net
MQTT_PORT (STR,05) ........... 8883
MQTT_USERNAME (STR,89) ....... poc-iotconnect-iothub-030-eu2.azure-devices.net/avtds-justatoken/?api-version=2018-06-30
MQTT_PASSWORD (STR,171) ....... SharedAccessSignature sr=poc-iotconnect-iothub-030-eu2.azure-devices.net%2Fdevices%2Favtds-justatoken&sig=JyL0PZGZZ%2BsjClyUUUl07Puu9v%2FVRboM5z4ffDCi1iY%3D&se=1722002906
MQTT_SUB_CID (STR,17) ........ avtds-justatoken
MQTT_PUB_TOPIC (STR,42) ...... devices/avtds-justatoken/messages/events/
MQTT_TLS (STR,02) ............ 1
MQTT_PING_PERIOD (STR,03) .... 60
MQTT_VER311 (STR,02) ......... 1
MQTT_SUB_TOPIC_NUM (STR,02) .. 1
MQTT_SUB_TOPIC0 (STR,48) ..... devices/avtds-justatoken/messages/devicebound/#
```

#### IoTConnect NVRAM

This whole section needs to be redacted to remove confidential information

On command line:
```
[/DA16200/NVRAM] # printenv
```

Variables generally start with IOTC. At minimum the following values need to be set in nvram:
```
IOTC_CPID (STR,06) ........... avtds
IOTC_ENV (STR,09) ............ avnetpoc
IOTC_AUTH_TYPE (STR,02) ...... 1
IOTC_DUID (STR,11) ........... justatoken
```

### MQTT NVRAM

IoTConnect uses the Renesas mqtt mosquitto client.

On a successful IoTConnect setup and start, the MQTT values will be updated (overwritten) in NVRAM, including:

Variables generally start with MQTT

```
    MQTT_CLIENT_IOTC_MODE
    MQTT_BROKER
    MQTT_PORT
    MQTT_USERNAME
    MQTT_PASSWORD
    MQTT_SUB_CID
    MQTT_PUB_TOPIC
    MQTT_TLS
    MQTT_PING_PERIOD
    MQTT_VER311
    MQTT_SUB_TOPIC_NUM
    MQTT_SUB_TOPIC0
```

### MQTT configuration by hand

On command line:
```
[/DA16200/NET] # mqtt_config status
```

```
MQTT Client Information:

MQTT Status  : Not Running
  - Broker IP          : 
  - Port               : 8883
  - Pub. Topic         : 
  - Sub. Topic         : 
  - QoS Level          : 0
  - TLS                : Enable
  - Clean Session      : Yes
  - TLS ALPN           : (None)
  - TLS SNI            : (None)
  - TLS CIPHER SUIT    : (None)
  - Ping Period        : 60
  - TLS Incoming buf   : 6144(bytes)
  - TLS Outgoing buf   : 4096(bytes)
  - TLS Auth mode      : 1
  - User name          : 
  - Password           : 
  - Client ID          : 
  - MQTT VER           : 3.1.1
```

### Running MQTT client by hand

On command line:
```
[/DA16200/NET] # mqtt_client start
```

Note: IoTConnect interfaces to the mqtt_client - so stopping/starting the MQTT client outside of IoTConnect can affect the IoTConnect connection.
