# IoTConnect Application Setup

## Requirements

You require the following:

* The connection type

  * **1** for AWS
  * **2** for Azure

* MQTT Broker, Discovery and Sync certificates

  * ***AWS***
    AWS only requires a X509 certificate for the MQTT broker.

    At the time of writing, this is **Starfield Services Root Certificate Authority - G2**
   
    It can be acquired here: https://www.amazontrust.com/repository/SFC2CA-SFSRootCAG2.pem
  
  * ***AZURE***

    Azure requires:

    * An appropriate X509 certificate for the MQTT broker. 

      Azure at the time of writing uses *DigiCert Global Root G2**
   
      It can be acquired here: https://cacerts.digicert.com/DigiCertGlobalRootG2.crt.pem

    * An appropriate IoTConnect Discovery / Sync Root CA

      Examples (pick what suits your environment)
  
      * Microsoft ECC Root Certificate Authority 2017
      * Microsoft RSA Root Certificate Authority 2017

      You can acquire these here: https://www.microsoft.com/pkiops/docs/repository.htm

* IoTConnect Device **Certificate and Key** (or symmetric key if using symmetric key authentication)

* IoTConnect Device **DUID**

* IoTConnect Device **CPID** (Key Vault)

* IoTConnect Environment **ENV** (Key Vault)

## Certificate setup

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

### MQTT Root CA

Use `cert write ca1` to write the the AWS MQTT Root CA. Copy and paste it into the terminal, and terminate the input with `CTRL+C`.

```
[/DA16200/NET] # cert write ca1
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----

ca1 Write success.
```

### (Azure only) IoTConnect HTTP Discovery/Sync Cert

Skip this step if you are using AWS.

Use `cert write ca2` to write the the MQTT Root CA. Copy and paste it into the terminal, and terminate the input with `CTRL+C`.

```
[/DA16200/NET] # cert write ca2
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----

ca2 Write success.
``` 

### MQTT device certificate and private key  

If the device is using an X509-style authentication scheme, then you will also have to write this data to the device in the same manner using `cert write cert1` and `cert write key1` respectively.

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

## Verification of certificate setup

Use `cert status` to check that the data has been written.

```
[/DA16200/NET] # cert status

#1:
  For MQTT, CoAPs Client
  - Root CA     : Found
  - Certificate : Found
  - Private Key : Found
  - DH Parameter: Empty

(...)

[/DA16200/NET] #
```

## Setting up IoTConnect configuration parameters

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
[/DA16600/NET] # iotconnect_config use_cmd_ack [value] (see below)
[/DA16600/NET] # iotconnect_config use_ota_ack [value] (see below)
```

The MQTT Broker configuration will be set automatically after a **Discovery/Sync** cycle.

### use_cmd_ack
By default commands will fail automatically, i.e. the acknowledgement is handled implicitly.

To handle the command acknowledgement explicitly set use_cmd_ack to 1.
To revert to implicit command acknowledgement set use_cmd_ack to 0.

### use_ota_ack
By default OTA update will fail automatically, i.e. the acknowledgement is handled implicitly.

To handle the OTA update acknowledgement explicitly set use_ota_ack to 1.
To revert to implicit OTA acknowledgement set use_ota_ack to 0.

## Finalization

You can now run `iotconnect_client setup` and `iotconnect_client start` to start the IoTConnect client and send telemetry.

You can also simply reboot the device to apply the settings in one go.
