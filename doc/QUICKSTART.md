# DA16200 Quickstart Guide

This document and the information it contains is CONFIDENTIAL and remains the property of our company. It may not be copied or communicated to a third party or used for any purpose other than that for which it is supplied without the prior written consent of our company. This document must be destroyed with a paper shredder.

## Boards

Please refer to the Renesas guide “User Manual, DA16200 DA16600 FreeRTOS Getting Started Guide, UM-WI-056” to check that the configuration shown above is valid - in case there are changes in the future.
Setup DA16200

### Using DA16200MOD EVK

Connect USB lead to board and PC.
![](assets/IMG_20230724_180805286.jpg)

### Using DA16200MOD
![](assets/IMG_20230719_180344535.jpg)

#### FTDI connection

Connect to FTDI as follows
![](assets/IMG_20230817_144820004.jpg)

PMOD connections
![](assets/IMG_20230720_100517916.jpg)

FTDI connections
![](assets/IMG_20230720_100436713.jpg)

Note RX/TX are crossed, TX on FTDI goes to RX on PMOD and RX on FTDI goes to TX on PMOD.
FTDI is set to 3.3v

### Using DA16600MOD EVK

Connect USB lead to board and PC.
![](assets/IMG_20230724_180805286.jpg)

### Using DA16600MOD
![](assets/IMG_20230822_110208433.jpg)

#### FTDI connection

Connect to FTDI as per DA16200
![](assets/IMG_20230822_093301382.jpg)

## Connect DA16200 (or DA16600)
![](assets/Screenshot_2023-08-14_151921.png)

minicom: need to turn off software and hardware flow control and add '-c on' as console output includes color codes -- as well as setting baud rate, etc.

### DA16200MOD EVK and DA16600MOD EVK

Note: on the DA16200/DA16600 dev kit, there are two consoles.

#### Command console
The “command console” is on the lower port (at 230400 baud). 

#### AT console
The “AT console” is on the higher port (at 115200 baud) -- note this doesn't echo by default.

### DA16200PMOD and DA16600MOD
Note: on the PMOD DA16200/DA16600 board there are also two consoles.

#### Command console
The “command console” at 230400 baud accessed via the breakout pins, as shown above.

#### AT console
The “command console” at 230400 baud accessed via the breakout pins, see the [AT Command Set](AT_COMMAND_SET.md).

## Setup your DA16200 (or DA16600) as per the Renesas document
```
User Manual
DA16200 DA16600 FreeRTOS Getting Started Guide
UM-WI-056
```
Currently the document can be found linked at:
[Renesas DA16200 page](https://www.renesas.com/us/en/products/wireless-connectivity/wi-fi/low-power-wi-fi/da16200mod-devkt-da16200-ultra-low-power-wi-fi-modules-development-kit?gclid=EAIaIQobChMIxKyz4qHcgAMV1oFQBh3eWQsQEAAYASAAEgLqnvD_BwE#document)
or
[Renesas DA16600 page](https://www.renesas.com/eu/en/products/wireless-connectivity/wi-fi/low-power-wi-fi/da16600mod-devkt-da16600-ultra-low-power-wi-fi-bluetooth-low-energy-modules-development-kit#document).

At the command prompt `[/DA16200] #` type `setup` to:
- associate a WiFi access point (SSID, password, etc.)
- set SNTP to start automatically on boot. 

This setup process will write values to NVRAM that will be used when the DA16200 reboots.

The following images show an example of how to configure the DA16200.
![](assets/Screenshot_2023-08-15_123954-1.png)

![](assets/Screenshot_2023-08-15_124048.png)


Text example is shown below:
```
Wakeup source is 0x0
[dpm_init_retmemory] DPM INIT CONFIGURATION(1)


        ******************************************************
        *             DA16200 SDK Information
        * ---------------------------------------------------
        *
        * - CPU Type        : Cortex-M4 (120MHz)
        * - OS Type         : FreeRTOS 10.4.3
        * - Serial Flash    : 4 MB
        * - SDK Version     : V3.2.7.1 GEN
        * - F/W Version     : FRTOS-GEN01-01-98e58a5d3-006374
        * - F/W Build Time  : Aug 16 2023 11:50:07
        * - Boot Index      : 0
        *
        ******************************************************



System Mode : Station Only (0)
>>> Start DA16X Supplicant ...
>>> DA16x Supp Ver2.7 - 2022_03
>>> MAC address (sta0) : d4:3d:39:39:75:00
>>> sta0 interface add OK
>>> Start STA mode...

>>> UART1 : Clock=80000000, BaudRate=115200
>>> UART1 : DMA Enabled ...

[/DA16200] # setup

Stop all services for the setting.
 Are you sure ? [Yes/No] : y



[ DA16200 EASY SETUP ]

Country Code List:
AD  AE  AF  AI  AL  AM  AR  AS  AT  AU  AW  AZ  BA  BB  BD  BE  BF  BG  BH  BL
BM  BN  BO  BR  BS  BT  BY  BZ  CA  CF  CH  CI  CL  CN  CO  CR  CU  CX  CY  CZ
DE  DK  DM  DO  DZ  EC  EE  EG  ES  ET  EU  FI  FM  FR  GA  GB  GD  GE  GF  GH
GL  GP  GR  GT  GU  GY  HK  HN  HR  HT  HU  ID  IE  IL  IN  IR  IS  IT  JM  JO
JP  KE  KH  KN  KP  KR  KW  KY  KZ  LB  LC  LI  LK  LS  LT  LU  LV  MA  MC  MD
ME  MF  MH  MK  MN  MO  MP  MQ  MR  MT  MU  MV  MW  MX  MY  NG  NI  NL  NO  NP
NZ  OM  PA  PE  PF  PG  PH  PK  PL  PM  PR  PT  PW  PY  QA  RE  RO  RS  RU  RW
SA  SE  SG  SI  SK  SN  SR  SV  SY  TC  TD  TG  TH  TN  TR  TT  TW  TZ  UA  UG
UK  US  UY  UZ  VA  VC  VE  VI  VN  VU  WF  WS  YE  YT  ZA  ZW  ALL

 COUNTRY CODE ? [Quit] (Default KR) : uk


SYSMODE(WLAN MODE) ?
        1. Station
        2. Soft-AP
        3. Station & SOFT-AP
 MODE ?  [1/2/3/Quit] (Default Station) : 1

[ STATION CONFIGURATION ]
============================================================================
[NO] [SSID]                                         [SIGNAL] [CH] [SECURITY]
----------------------------------------------------------------------------
[ 1] NETGEAR83                                          -51   1         WPA2
[ 2] VM0423187                                          -85   6         WPA2
[ 3] Virgin Media                                       -86   6     WPA2-ENT
[ 4] BT-XHASKN                                          -88   1         WPA2
[ 5] SKY40F4C                                           -88  11         WPA2
[ 6] BTHub6-R2MK                                        -89   6         WPA2
[ 7] SKY40F4C                                           -90   1         WPA2
[ 8] TALKTALKD6CAFD                                     -91   6         WPA2
[ 9] BTHub6-J563                                        -91  11         WPA2
[10] BTHub6-6SWG                                        -93  11         WPA2
[11] BTWi-fi                                            -88   1
[12] BTWi-fi                                            -90   6
[13] BTWi-fi                                            -92  11
[14] BTWi-fi                                            -93  11
----------------------------------------------------------------------------
[M] Manual Input
[Enter] Rescan
============================================================================

 Select SSID ? (1~14/Manual/Quit) : 1

 PSK-KEY(ASCII characters 8~63 or Hexadecimal characters 64) ? [Quit]
[123456789|123456789|123456789|123456789|123456789|123456789|1234]
:***********

 Do you want to set advanced WiFi configuration ? [No/Yes/Quit] (Default No) : n

============================================
SSID        : NETGEAR83
AUTH        : WPA/WAP2-PSK
ENCRYPTION  : TKIP/AES(CCMP)
PSK KEY     : ***********
KEY TYPE    : ASCII
PMF MODE    : Disable
Hidden AP   : Not connect
============================================
 WIFI CONFIGURATION CONFIRM ? [Yes/No/Quit] : y

 IP Connection Type ? [Automatic IP/Static IP/Quit] : a


IP Connection Type: Automatic IP

 IP CONFIGURATION CONFIRM ? [Yes/No/Quit] : y

 SNTP Client enable ? [Yes/No/Quit] : y


 SNTP Period time (1 ~ 36 hours) ? (default : 36 hours) [Quit]

 GMT Timezone +xx:xx|-xx:xx (-12:00 ~ +12:00) ? (default : 00:00) [Quit]

 SNTP Server 0 addr ? (default : pool.ntp.org) [Quit]
Input :

 SNTP Server 1 addr ? (default : 1.pool.ntp.org) [Quit]
Input :

 SNTP Server 2 addr ? (default : 2.pool.ntp.org) [Quit]
Input :
============================================
SNTP Client      : Enable
SNTP Period time : 36 hours
SNTP GMT Timezone: 00:00
SNTP Server addr : pool.ntp.org
SNTP Server addr1: 1.pool.ntp.org
SNTP Server addr2: 2.pool.ntp.org
============================================
 SNTP Client CONFIRM ? [Yes/No/Quit] : y

 Fast Connection Sleep 2 Mode ? [Yes/No/Quit] : y

Configuration OK

Reboot...


Wakeup source is 0x0
[dpm_init_retmemory] DPM INIT CONFIGURATION(1)


        ******************************************************
        *             DA16200 SDK Information
        * ---------------------------------------------------
        *
        * - CPU Type        : Cortex-M4 (120MHz)
        * - OS Type         : FreeRTOS 10.4.3
        * - Serial Flash    : 4 MB
        * - SDK Version     : V3.2.7.1 GEN
        * - F/W Version     : FRTOS-GEN01-01-98e58a5d3-006374
        * - F/W Build Time  : Aug 16 2023 11:50:07
        * - Boot Index      : 0
        *
        ******************************************************


>>> Wi-Fi Fast_Connection mode ...


System Mode : Station Only (0)
>>> Start DA16X Supplicant ...
>>> DA16x Supp Ver2.7 - 2022_03
>>> MAC address (sta0) : d4:3d:39:39:75:00
>>> sta0 interface add OK
>>> Start STA mode...

>>> UART1 : Clock=80000000, BaudRate=115200
>>> UART1 : DMA Enabled ...
Fast scan(Manual=0), freq=2412, num_ssids=1

>>> Network Interface (wlan0) : UP
>>> Associated with cc:40:d0:ef:b4:57

Connection COMPLETE to cc:40:d0:ef:b4:57

-- DHCP Client WLAN0: SEL(6)
-- DHCP Client WLAN0: REQ(1)
-- DHCP Client WLAN0: CHK(8)
-- DHCP Client WLAN0: BOUND(10)
         Assigned addr   : 192.168.0.12
               netmask   : 255.255.255.0
               gateway   : 192.168.0.1
               DNS addr  : 192.168.0.1

         DHCP Server IP  : 192.168.0.1
         Lease Time      : 24h 00m 00s
         Renewal Time    : 12h 00m 00s
```

### DPM

Note at present need to have DPM support selected as No during setup.

## Flashing images using uart_program_da16200

Please refer to the Renesas guide “User Manual, DA16200 DA16600 FreeRTOS Getting Started Guide, UM-WI-056” in case there are changes in the future - in version 8, see section 4.5 "Programming Firmware Images".

For example, after building IoTConnect_client, copy appropriate Linux or Windows version of uart_program_da16200 to apps/common/examples/Network/IoTConnect_Client/projects/da16200/img and then (in Linux)
- cd apps/common/examples/Network/IoTConnect_Client/projects/da16200/img
- ./uart_program_da16200 -i 0 DA16200_FBOOT-GEN01-01-c7f4c6cc22_W25Q32JW.img
- ./uart_program_da16200 -i 23000 DA16200_FRTOS-GEN01-01-f017bfdf51-006558.img
- ./uart_program_da16200

Note: the uart_program_da16200 is part of the DA16200_DA16600_SDK_FreeRTOS_v3.2.8.0.zip in:
- utility/j-link/scripts/qspi/linux/uart_program_da16200
- utility/j-link/scripts/qspi/win/uart_program_da16200.exe

## Flashing images using Ymodem

Please refer to the Renesas guide “User Manual, DA16200 DA16600 FreeRTOS Getting Started Guide, UM-WI-056” in case there are changes in the future - in version 7, see section 4.5.1 "Firmware Update Using Commands".

To flash DA16200 images use TeraTerm YModem transfer protocol (or similar) to send the images to the DA16200 using the “command console”, e.g.
![](assets/Screenshot_2023-08-14_150336.png)

Note: TeraTerm Ymodem transfer seems a little sensitive.
- The first transfer (when the file selector is not in the correct directory) sometimes seems upset the transfer process -- so the transfer process may have to be cancelled and restarted.
- Subsequent transfers (when the file selection is already in the correct directory) seem much better behaved.
- Moving TeraTerm Ymodem windows seems to adversely affect any ongoing transfer.

The smaller image, e.g. DA16200_FBOOT-GEN01-01-922f1e27d_W25Q32JW.img is flashed using loady 0

The larger image, e.g. DA16200_FRTOS-GEN01-01-98e58a5d3-006374.img is flashed using loady 23000

Run boot_idx 0 (refer to Section 4.5.4 in User Manual mentioned above)
```
reboot
```
### Images for DA16200

See: [DA16200 images](./image/release-da16200-images-332b6a40bac5213ee79c4c991b9af3e2bbd1a41c.tgz).

### Images for DA16600 with Adesto flash

See: [DA16600 Adesto images](./image/release-da16600-adesto-images-5b816ee62b6747c543c8287e0148712557f7c691.tgz).

## Setting up IoTConnect

### Setup X509 keys (for IoTConnect and Azure)

At the command prompt, type net to get access to the network based commands (need to type up to get back to “normal” command prompt).

Need to set the X509 certificates for IoTConnect discovery/sync and Azure MQTT, e.g.
```
[/DA16200/NET] # cert status

#1:
  For MQTT, CoAPs Client
  - Root CA     : Empty
  - Certificate : Empty
  - Private Key : Empty
  - DH Parameter: Empty

#2:
  For HTTPs, OTA
  - Root CA     : Empty
  - Certificate : Empty
  - Private Key : Empty
  - DH Parameter: Empty

#3:
  For Enterprise (802.1x)
  - Root CA     : Empty
  - Certificate : Empty
  - Private Key : Empty
  - DH Parameter: Empty

TLS_CERT for ATCMD
  - TLS_CERT_01 : Empty
  - TLS_CERT_02 : Empty
  - TLS_CERT_03 : Empty
  - TLS_CERT_04 : Empty
  - TLS_CERT_05 : Empty
  - TLS_CERT_06 : Empty
  - TLS_CERT_07 : Empty
  - TLS_CERT_08 : Empty
  - TLS_CERT_09 : Empty
  - TLS_CERT_10 : Empty
```

Pick an appropriate X509 certificate for the Azure MQTT broker, e.g. Need to copy/paste (with CR) and then type “control-C”.

```
[/DA16200/NET] # cert write ca1
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----

ca1 Write success.
```
Note Azure X509 certificates are in process of changing, so may need to use a different certificate for the Root of Trust in the future.

It is unclear whether DA16200 is able to process multiple possible certificates for a Root CA – all testing has used a single certificate. Obviously, if a certificate doesn’t allow a connection, then may need to “manually” swap to an alternative certificate.

Pick an appropriate X509 certificate for the IoTConnect HTTP discovery/sync, e.g. Need to copy/paste (with CR) and then type “control-C”.
```
[/DA16200/NET] # cert write ca2
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----

ca2 Write success.
```

Now we can see that the certificates are written, i.e. MQTT and HTTP Root CA certificates are now found.

```
[/DA16200/NET] # cert status

#1:
  For MQTT, CoAPs Client
  - Root CA     : Found
  - Certificate : Empty
  - Private Key : Empty
  - DH Parameter: Empty

#2:
  For HTTPs, OTA
  - Root CA     : Found
  - Certificate : Empty
  - Private Key : Empty
  - DH Parameter: Empty

#3:
  For Enterprise (802.1x)
  - Root CA     : Empty
  - Certificate : Empty
  - Private Key : Empty
  - DH Parameter: Empty

TLS_CERT for ATCMD
  - TLS_CERT_01 : Empty
  - TLS_CERT_02 : Empty
  - TLS_CERT_03 : Empty
  - TLS_CERT_04 : Empty
  - TLS_CERT_05 : Empty
  - TLS_CERT_06 : Empty
  - TLS_CERT_07 : Empty
  - TLS_CERT_08 : Empty
  - TLS_CERT_09 : Empty
  - TLS_CERT_10 : Empty

[/DA16200/NET] #
```

### MQTT device certificate and private key  
If the device is using an X509-style authentication scheme, then you will also have to write:

#### MQTT device certificate
```
[/DA16200/NET] # cert write cert1
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----

cert1 Write success.
```

#### MQTT device private key
```
[/DA16200/NET] # cert write key1
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN PRIVATE KEY-----
...
-----END PRIVATE KEY-----

key1 Write success.
```

## Command line

To “drive” the PMOD when there is only one terminal available, the “net” command-line is updated to include “iotconnect_config” and “iotconnect_client”.

At the command prompt, type net to get access to the network based commands (need to type up to get back to “normal” command prompt).

### iotconnect_config

The
```
iotconnect_config 
```
command will print the values of the IOTC nvram values.

Also
```
iotconnect_config option value
```
will set iotconnect 
- env
- cpid
- duid
- symmetric_key
- auth_type
- use_cmd_ack
- use_ota_ack 
options to the specified value (note unlike the AT command a symmetric key value with “=” can be set directly).

```
[/DA16200/NET] # iotconnect_config

- iotconnect_config

Usage : iotconnect_config [reset|env|cpid|duid|symmetric_key|auth_type|use_cmd_ack|use_ota_ack] [value]
    env:
    cpid:
    duid:
    symmetric_key:
    auth_type:
    use cmd ack:
    use OTA ack:
    DTG (get) :
```

#### env
Use value from IoTConnect Dashboard - Key Vault.

#### cpid
Use value from IoTConnect Dashboard - Key Vault.

#### duid
This is the device name as specified on IoTConnect dashboard.

#### auth_type

auth_type is an numeric value representing the authentication type.

##### Token = 1
	
##### X509 = 2
	
Use “cert write cert1” and “cert write key1” to save the device MQTT cert and private key in NVRAM.

##### Self-signed (X509) = 2
	
May change to 3 in the near future, for consistency.
	

Use “cert write cert1” and “cert write key1” to save the device MQTT cert and private key in NVRAM.

##### TPM = 4

Not supported.
	
##### Symmetric key = 5

#### symmetric_key
Only required if auth_type is 5.

Use “iotconnect_config symmetric_key string” to set the base64 encoded device symmetric key to “string”.

#### use_cmd_ack
By default commands will fail automatically, i.e. the acknowledgement is handled implicitly.

To handle the command acknowledgement explicitly set use_cmd_ack to 1.
To revert to implicit command acknowledgement set use_cmd_ack to 0.

#### use_ota_ack
By default OTA update will fail automatically, i.e. the acknowledgement is handled implicitly.

To handle the OTA update acknowledgement explicitly set use_ota_ack to 1.
To revert to implicit OTA acknowledgement set use_ota_ack to 0.

### iotconnect_client

#### Setup - read IOTC values from NVRAM

To setup IoTConnect values. run
```
iotconnect_client setup
```

#### Start (Discovery/Sync & MQTT Setup)

To run IoTConnect discovery/sync and update MQTT values and start mqtt_client, run
```
iotconnect_client start
```
Check that the device is shown as connected on the IoTConnect dashboard.

Note: must have been setup before starting.

#### Stop

To disconnect from IoTConnect but leave the runtime configuration intact
```
iotconnect_client stop
```
Check that the device is shown as disconnected on the IoTConnect dashboard.

Note: after a stop don’t have have to perform a setup, before a start - the previously determined values will then be re-used.

#### Reset

To disconnect from IoTConnect and reset the runtime configuration
```
iotconnect_client reset
```
Check that the device is shown as disconnected on the IoTConnect dashboard.

Note: after a reset must perform a setup, before a start - all values will then be determined afresh.

#### Message

To send an IotConnect message, run
```
iotconnect_client msg name value name2 value2
```
for up to 7 name/value pairs.

Check that the device is shown as connected and that the message data can be seen.

#### Command

A C2D command failure can be acknowledged using
```
iotconnect_client cmd_ack type ack_id 0 message
```
A C2D command success can be acknowledged using
```
iotconnect_client cmd_ack type ack_id 1 message
```
Note type and ack_id are printed on the terminal when the command request is received.

#### OTA

A C2D OTA failure can be acknowledged using
```
iotconnect_client ota_ack ack_id 0 message
```
A C2D OTA success can be acknowledged using
```
iotconnect_client ota_ack ack_id 1 message
```
Note ack_id is printed on the terminal when the OTA request is received.

## Using AT Commands

The use of AT commands is beyond this Quickstart guide, see the [AT Command Set](AT_COMMAND_SET.md).
