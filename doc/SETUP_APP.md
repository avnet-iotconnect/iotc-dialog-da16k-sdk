# /IOTCONNECT Application Setup

## Requirements

You require the following IoTConnect configuration parameters:

* Connection type

  * <kbd>1</kbd> for AWS
  * <kbd>2</kbd> for Azure

* Device **Certificate and Key** (or symmetric key if using symmetric key authentication)

* Device `DUID` (Key Vault)

* Device `CPID` (Key Vault)

* Environment `ENV` (Key Vault)

## Configuration outside of the command line

These parameters can be configured via the [AT command set](./AT_COMMAND_SET.md). 

The [ATCMD Library](https://github.com/avnet-iotconnect/iotc-freertos-da16k-atcmd-lib) - on supported platforms - provides structures and APIs to configure these parameters from the host devices (i.e. devices that this module is connected to) as well.

## Configuring via DA16K Serial Command Line

### Certificate setup

At the command prompt, type <kbd>net</kbd> to get access to the network based commands (need to type <kbd>up</kbd> to get back to main prompt menu).
```
net
```
<pre><samp>[/DA16200/NET] # <kbd>net</kbd></samp></pre>
The <kbd>cert</kbd> command is used to write the certificate and key data:
```
cert status
```
<pre><samp>[/DA16200/NET] # <kbd>cert status</kbd>
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
  - DH Parameter: Empty</samp></pre>

#### MQTT device certificate and private key  

If the device is using an X509-style authentication scheme, then you will have to write this data to the device in the same manner using `cert write cert1` and `cert write key1` respectively.

#### MQTT device certificate

```
cert write cert1
```
<pre><samp>[/DA16200/NET] # <kbd>cert write cert1</kbd>
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----

cert1 Write success.
</samp></pre>

#### MQTT device private key
```
cert write key1
```

<pre><samp>[/DA16200/NET] # <kbd>cert write key1</kbd>
Typing data: (Certificate value)
        Cancel - CTRL+D, End of Input - CTRL+C or CTRL+Z
-----BEGIN PRIVATE KEY-----
...
-----END PRIVATE KEY-----

key1 Write success.
</samp></pre>

### Verification of certificate setup

Use <kbd>cert status</kbd> to check that the data has been written.

```
cert status
```

<pre><samp>[/DA16200/NET] # <kbd>cert status</kbd>
#1:
  For MQTT, CoAPs Client
  - Root CA     : Empty
  - Certificate : Found
  - Private Key : Found
  - DH Parameter: Empty
</samp></pre>

### Setting up IoTConnect configuration parameters

* Set up Connection Type
* Set up Environment
* Set up CPID
* Set up DUID
* Set up Symmetric key (if using symmetric key authentication)
* Set up Authentication type

  * <kbd>1</kbd>: X509
  * <kbd>2</kbd> Symmetric key


Use the <kbd>iotconnect_config</kbd> command to accomplish this.
<pre><samp>[/DA16600/NET] # </samp></pre>

```
iotconnect_config connection_type 1
```
```
iotconnect_config duid REPLACE_WITH_YOUR_DUID
```
```
iotconnect_config cpid REPLACE_WITH_YOUR_CPID
```
```
iotconnect_config env REPLACE_WITH_YOUR_ENV
```
```
iotconnect_config auth_type 1
```

The MQTT Broker configuration will be set automatically after a **Discovery/Sync** cycle.

### Finalization

You can now run:
```
iotconnect_client setup
```
and then
```
iotconnect_client start
````
to start the /IOTCONNECT client and send telemetry.

You can also simply reboot the device to apply the settings.
