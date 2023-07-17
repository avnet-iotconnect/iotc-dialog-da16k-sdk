

 

# DA16200 Developer's Guide

## Initial Setup

See [QUICKSTART](./QUICKSTART.md) for details on how to flash the images and setup IoTConnect.

## Setup For FreeRTOS development

Setup your development environment as per the Renesas
```
User Manual
DA16200 DA16600 FreeRTOS Getting Started Guide
UM-WI-056
```

Currently the document can be found linked at:
[Renesas DA16200 page](https://www.renesas.com/us/en/products/wireless-connectivity/wi-fi/low-power-wi-fi/da16200mod-devkt-da16200-ultra-low-power-wi-fi-modules-development-kit?gclid=EAIaIQobChMIxKyz4qHcgAMV1oFQBh3eWQsQEAAYASAAEgLqnvD_BwE#document)
or
[Renesas DA16600 page](https://www.renesas.com/eu/en/products/wireless-connectivity/wi-fi/low-power-wi-fi/da16600mod-devkt-da16600-ultra-low-power-wi-fi-bluetooth-low-energy-modules-development-kit#document).

Recommended to setup so can build and flash the DA16200 – at least using YModem protocol, i.e. using `loady` commands. Need to run easy WiFi `setup` to associate a WiFi access point (SSID, password, etc.). Should also set SNTP to start automatically on boot. Check that the board is configured correctly and that can build at least one example application, e.g. apps/common/examples/Network/Http_Client using projects/da16200 (or projects/da16600).

Note: DA16600 has two different flash configurations, whereas DA16200 only has a single flash configuration.

Issue: It should be possible to easily use binaries between different boards using DA16200 - may have to generate two sets of binaries to cover all DA16600 boards.

## Setup for IoTConnect development

Warning: the FreeRTOS software the IoTConnect application has been based on is DA16200_DA16600_SDK_FreeRTOS_v3.2.7.1.zip – other versions may work, but cannot be guaranteed.

The IoTConnect example application and modifications to the FreeRTOS package can be accessed at [IoTConnect dialog github](https://github.com/avnet-iotconnect/iotc-dialog-da16k-sdk).

Clone the git repo and then use the downloaded files to overwrite their counterparts / add files to the original Renesas FreeRTOS v3.2.7.1 package – may need to take account of different directory levels to do so.

## Compiling the IoTConnect application

The IoTConnect application is at apps/common/examples/Network/IoTConnect_Client.

The application was developed for a DA16200, so `projects/da16200` should work to compile the application.

Hopefully (warning) DA16600 version, i.e. `projects/da16600` should also work to compile the application.

Once the binaries are compiled they should be in: `projects/dialog/img`.

See [QUICKSTART](./QUICKSTART.md) for details on how to flash the images and setup IoTConnect.

 

