# IoTConnect Application Setup

## Requirements

You require the following IoTConnect configuration parameters:

* Connection type

  * **1** for AWS
  * **2** for Azure

* Device **Certificate and Key** (or symmetric key if using symmetric key authentication)

* Device **DUID** (Key Vault)

* Device **CPID** (Key Vault)

* Environment **ENV** (Key Vault)

## Configuration outside of the command line

These parameters can be configured via [AT command set](./doc/AT_COMMAND_SET.md). 

The [ATCMD Library](https://github.com/avnet-iotconnect/iotc-freertos-da16k-atcmd-lib) - on supported platforms - provides structures and APIs to configure these parameters from the host devices (i.e. devices that this module is connected to) as well.

## Configuring via DA16K Serial Command Line

### Certificate setup

At the command prompt, type `net` to get access to the network based commands (need to type up to get back to “normal” command prompt).

The `cert` command is used to write the certificate  and key data:

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

(...)
```

#### MQTT device certificate and private key  

If the device is using an X509-style authentication scheme, then you will have to write this data to the device in the same manner using `cert write cert1` and `cert write key1` respectively.

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

### Verification of certificate setup

Use `cert status` to check that the data has been written.

```
[/DA16200/NET] # cert status

#1:
  For MQTT, CoAPs Client
  - Root CA     : Empty
  - Certificate : Found
  - Private Key : Found
  - DH Parameter: Empty

(...)

[/DA16200/NET] #
```

### Setting up IoTConnect configuration parameters

* Set up Connection Type
* Set up Environment
* Set up CPID
* Set up DUID
* Set up Symmetric key (if using symmetric key authentication)
* Set up Authentication type

  * `1`: X509
  * `2` Symmetric key


Use the `iotconnect_config` command to accomplish this.

```
[/DA16600/NET] # iotconnect_config connection_type 1
[/DA16600/NET] # iotconnect_config duid somedevice
[/DA16600/NET] # iotconnect_config cpid 954ACBA65B4A88799FACBCDEADBEEF12
[/DA16600/NET] # iotconnect_config env SOME_ENV
[/DA16600/NET] # iotconnect_config auth_type 1
```

The MQTT Broker configuration will be set automatically after a **Discovery/Sync** cycle.

### Finalization

You can now run `iotconnect_client setup` and `iotconnect_client start` to start the IoTConnect client and send telemetry.

You can also simply reboot the device to apply the settings in one go.
