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

Currently, DPM is not supported, so select **No** during setup.

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
- The first transfer (when the file selector is not in the correct directory) sometimes seems to upset the transfer process -- so the transfer process may have to be cancelled and restarted.
- Subsequent transfers (when the file selection is already in the correct directory) seem much better behaved.
- Moving TeraTerm Ymodem windows seems to adversely affect any ongoing transfer.

The smaller image, e.g. DA16200_FBOOT-GEN01-01-922f1e27d_W25Q32JW.img is flashed using loady 0

The larger image, e.g. DA16200_FRTOS-GEN01-01-98e58a5d3-006374.img is flashed using loady 23000

Run boot_idx 0 (refer to Section 4.5.4 in User Manual mentioned above)
```
reboot
```
### Prebuilt images

As there are many different device, flash and type (AWS / Azure) combinations, there are no pre-built images (yet).

# Setting up IoTConnect

Please follow the guide for the instance type (AWS / Azure) you wish to connect to.

**Note:** on Linux a more specialised terminal program may be required such as minicom, rather than uart_program_da16200 -- since entering a certicate is finised by pressing control-C or control-Z which kills/stops uart_program_da16200.

***NOTE***: It is impossible for the  DA16xxx to process multiple possible certificates for a Root CA – all testing has used a single certificate. Obviously, if a certificate doesn’t allow a connection, then it may be required to manually swap to an alternative certificate.


## Configuring Certificates & IoTConnect Application Config

Refer to the [Application Setup Guide](SETUP_APP.md).

## Running `iotconnect_client`

### Setup - read IOTC values from NVRAM

Ensure all certificates are in place, and that iotconnect_config has been used to save the configuration -- before initiating `iotconnect_client setup`.

If the configuration stage is missed, then there will be no valid values to setup.

To setup IoTConnect values, run:
`
```
iotconnect_client setup
```

### Start (Discovery/Sync & MQTT Setup)

Ensure that all certificates are in place, that iotconnect_config has been used to save the configuration, and that "iotconnect_client setup" has been run -- before initiating "iotconnect_client start".

To run IoTConnect discovery/sync and update MQTT values and start mqtt_client, run
```
iotconnect_client start
```
Check that the device is shown as connected on the IoTConnect dashboard.

Note: must have been setup before starting.

### Stop

To disconnect from IoTConnect but leave the runtime configuration intact
```
iotconnect_client stop
```
Check that the device is shown as disconnected on the IoTConnect dashboard.

**Note:** After stopping, there is no necessitgy to perform another setup before the next start. The previously determined values will be re-used.

### Reset

To disconnect from IoTConnect and **reset the entire runtime configuration**, run:

```
iotconnect_client reset
```
Check that the device is shown as disconnected on the IoTConnect dashboard.

**Note:** After resetting, another setup *must* be performed before the next start.

### Message

To send an IotConnect message with up to **7** key/value pairs, run

```
iotconnect_client msg [name1] [value1] [name2] [value2] (...)
```

Verify in the dashboard that the device is shown as connected and that the message data can be seen.

### ~~Command~~

***This chapter is not applicable at this point in time, as OTA and commands are not supported.***

> To acknowledge a C2D command failure, run:
>
> ```
>   iotconnect_client cmd_ack type ack_id 0 [message]
> ```
>
>To acknowledge a C2D command success, run:
>
>```
>iotconnect_client cmd_ack type ack_id 1 [message]
>```
>
>**Note**: `type` and `ack_id` are printed on the terminal when the command request is received.
>

### ~~OTA~~

***This chapter is not applicable at this point in time, as OTA and commands are not supported.***

To acknowledge a C2D OTA failure, run:

```
iotconnect_client ota_ack ack_id 0 [message]
```

To acknowledge a C2D OTA success, run:

```
iotconnect_client ota_ack ack_id 1 [message]
```

**Note:** `ack_id` is printed on the terminal when the OTA request is received.

## Using AT Commands

The use of AT commands is beyond this Quickstart guide.

Instead, see the [AT Command Set](AT_COMMAND_SET.md) documentation.
