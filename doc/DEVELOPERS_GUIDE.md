# DA16200 Developer's Guide

## Development Environment Setup

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


This includes the following steps:
* Downloading and installing the e² studio software + tool-chains
    * The software can be downloaded at the Renesas [e² studio](https://www.renesas.com/us/en/software-tool/e-studio#downloads) page.
* Setting up the e² Studio environment to build and flash DA16K projects.

The chapter **`5.4 Importing DA16200 FreeRTOS SDK Project into e² studio`** is easy to overlook, make sure you follow it to be able to flash the built firmware images.

## Setup for IoTConnect development

Warning: the IoTConnect application is based on the FreeRTOS SDK version 3.2.8.1 at the time of writing (using e² studio).

Other versions may work, but cannot be guaranteed.

* If you do not have a Dialog DA16K FreeRTOS SDK directory already, download and upnzip the SDK file (e.g. [DA16200_DA16600_SDK_FreeRTOS_v3.2.8.1.zip, available here](https://www.renesas.com/us/en/document/sws/da16200-da16600-freertos-sdk-v3281?language=en&r=1600096)) to an arbitrary directory (e.g. `sdk_dir`)
* Clone the git repo.
* ***Run the setup script:*** `./scripts/setup-project.sh sdk_dir`

    Note: This must be run from the repository's root directory.

## Compiling the IoTConnect application

The IoTConnect application is at `apps/common/example/Network/IoTConnect_Client`.

Import the project as follows:

* Select `File`, `Import` in the main screen of your workspace.

    ![](assets/ide1.png)

* Select `Dialog SDK Project`

    ![](assets/ide2.png)

* Select the repository directory as the `SDK root directory`.

    Seroll down to find the IoTConnect application project for your target processor (DA16200 or DA16600) and select it.

    ![](assets/ide3.png)

* Select the target device.

    ![](assets/ide4.png)

* The project is now added to the workspace and you can start developing and building.

    ![](assets/ide5.png)

### Selecting the flash configuration

On your first build, you will be asked to configure the project for the 

The flash chip option is dependent on the specific device you are targeting, so you must consult the datasheet / technical manual / schematics of the device.

The displayed example works for the DA16600PMOD dongle.

![](assets/ide6.png)

## Flashing images using `uart_program_da16200`

**Note: Make sure the serial port is not being used by other terminal software.**

### Using e² studio

As the `5.4 Importing DA16200 FreeRTOS SDK Project into e² studio` chapter adds the necessary flash tools to the IDE, you can use it to flash the built firmware from it:

![](assets/ideflash1.png)

The tool will ask you for the appropriate serial port device and then start the flashing process.

![](assets/ideflash2.png)

### Manual

Please refer to the Renesas guide *User Manual DA16200 DA16600 FreeRTOS Getting Started Guide UM-WI-056* in case there are changes in the future - in version 8, see section 4.5 (*Programming Firmware Images*).

For example, after building IoTConnect_client, copy appropriate Linux or Windows version of uart_program_da16200 to apps/common/examples/Network/IoTConnect_Client/projects/da16200/img and then (in Linux)

```
cd apps/common/examples/Network/IoTConnect_Client/projects/da16200/img
./uart_program_da16200 -i 0 DA16200_FBOOT-GEN01-01-c7f4c6cc22_W25Q32JW.img
./uart_program_da16200 -i 23000 DA16200_FRTOS-GEN01-01-f017bfdf51-006558.img
./uart_program_da16200
```

Note: the uart_program_da16200 is part of the DA16200_DA16600_SDK_FreeRTOS_v3.2.8.0.zip in:
- utility/j-link/scripts/qspi/linux/uart_program_da16200
- utility/j-link/scripts/qspi/win/uart_program_da16200.exe

## Setting up the IoTConnect application

See [QUICKSTART](./QUICKSTART.md) for details on how to flash the images and setup IoTConnect.

