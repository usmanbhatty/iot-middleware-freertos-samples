# Connect an NXP MIMXRT1060-EVK Evaluation kit using Azure IoT middleware for FreeRTOS

## What you need

- NXP Evaluation Kit: [MIMXRT1060-EVK](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/mimxrt1060-evk-i-mx-rt1060-evaluation-kit:MIMXRT1060-EVK)
- USB 2.0 A male to Micro USB male cable
- Wired Ethernet access with DHCP
- Ethernet cable

## Prerequisites

- [CMake](https://cmake.org/download/) (Version 3.13 or higher)
- [Ninja build system](https://github.com/ninja-build/ninja/releases) (Version 1.10 or higher)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) (This sample was tested against Version 10.3)
- Terminal tool like [Termite](https://www.compuphase.com/software_termite.htm), Putty, Tera Term, etc.
- To run this sample you can use a device previously created in your IoT Hub or have the Azure IoT Middleware for FreeRTOS provision your device automatically using DPS. **Note** that even when using DPS, you still need an IoT Hub created and connected to DPS. If you haven't deployed the necessary Azure resources yet, [you may use the guide here](https://github.com/Azure-Samples/iot-middleware-freertos-samples/blob/main/docs/azure-bicep-deployment.md).

IoT Hub | DPS
---------|----------
Have an [Azure IoT Hub](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal) created | Have an instance of [IoT Hub Device Provisioning Service](https://docs.microsoft.com/azure/iot-dps/quick-setup-auto-provision#create-a-new-iot-hub-device-provisioning-service)
Have a [logical device](https://docs.microsoft.com/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) created in your Azure IoT Hub using your preferred authentication method | Have an [individual enrollment](https://docs.microsoft.com/azure/iot-dps/how-to-manage-enrollments#create-a-device-enrollment) created in your instance of DPS using your preferred authentication method

*While this sample supports SAS keys and Certificates, this guide will refer only to SAS keys.

### Install prerequisites on Windows

Ensure that cmake, ninja and the ARM toolset binaries are available in the `PATH` environment variable.

You may also need to enable long path support for both Windows and git:

- Windows: <https://docs.microsoft.com/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd#enable-long-paths-in-windows-10-version-1607-and-later>
- Git: as **administrator** run

  ```powershell
  git config --system core.longpaths true
  ```

## Get the middleware

Clone the following repo to download all sample device code, setup scripts, and offline versions of the documentation. 

**If you previously cloned this repo in another sample, you don't need to do it again.**

```bash
git clone https://github.com/Azure-Samples/iot-middleware-freertos-samples.git
```

To initialize the repo, run the following command:

```bash
git submodule update --init --recursive
```

## Prepare the device

To connect the MIMXRT1060-EVK to Azure, you'll modify a configuration file for Azure IoT settings, rebuild the image, and flash the image to the device.

Update the file `iot-middleware-freertos-samples/demos/projects/NXP/mimxrt1060/config/demo_config.h` with your configuration values.

If you're using a device previously created in your **IoT Hub** with SAS authentication, disable DPS by commenting out `#define democonfigENABLE_DPS_SAMPLE` and set the following parameters:

Parameter | Value
---------|----------
 `democonfigDEVICE_ID` | _{Your Device ID value}_
 `democonfigHOSTNAME` | _{Your Host name value}_ 
 `democonfigDEVICE_SYMMETRIC_KEY` | _{Your Primary Key value}_ 

If you're using **DPS** with an individual enrollment with SAS authentication, set the following parameters:

Parameter | Value
---------|----------
 `democonfigID_SCOPE` | _{Your ID scope value}_
 `democonfigREGISTRATION_ID` | _{Your Device Registration ID value}_
 `democonfigDEVICE_SYMMETRIC_KEY` | _{Your Primary Key value}_

## Build the image

To build the device image, navigate to the `iot-middleware-freertos-samples` directory and run the following commands:

  ```bash
  cmake -G Ninja -DBOARD=mimxrt1060 -DVENDOR=NXP -Bmimxrt1060 .
  cmake --build mimxrt1060
  ```

After the build completes, confirm that a folder named `mimxrt1060/` was created and it contains a file named `demo/projects/NXP/mimxrt1060/iot-middleware-sample.bin`.

## Flash the image

1. Connect the Micro USB cable to the Micro USB port on the NXP EVK, and then connect it to your computer. After the device powers up, a solid green LED shows the power status.
1. Use the Ethernet cable to connect the NXP EVK to an Ethernet port.
1. In File Explorer, find the binary file that you created in the previous section and copy it.
1. In File Explorer, find the NXP EVK device connected to your computer. The device appears as a drive on your system with the drive label RT1060-EVK.
1. Paste the binary file into the root folder of the NXP EVK. Flashing starts automatically and completes in a few seconds.
1. Reset the device once flashing completes.

## Confirm device connection details

You can use one of the terminal applications to monitor communication and confirm that your device is set up correctly. Go to Device Manager in Windows to determine which COM port your device was assigned.

The following settings can be used to monitor serial data:

- Baud Rate: `115200`
- Data Bits: `8`
- Stop Bits: `1`
- Parity: none
- Flow Control: none

## VS Code debug experience for the MIMXRT1060-EVK 

After running the sample, you can use VS Code to debug your application directly in the Dev Kit following the steps described in [this guide](VSCodeDebug.md).

![VSCode Debug](media/VSCode-Debug.png)

## Size Chart

The following chart shows the RAM and ROM usage for the MIMXRT1060-EVK Evaluation kit from NXP. 
Build options: CMAKE_BUILD_TYPE=MinSizeRel (-Os) and no logging (-DLIBRARY_LOG_LEVEL=LOG_NONE):
This sample can include IoT Hub only, or IoT Hub plus DPS services and/or ADU services. The table below shows RAM/ROM sizes considering:

- Middleware libraries only – represents the libraries for Azure IoT connection.
- Total size – which includes the Azure IoT middleware for FreeRTOS, Mbed TLS, FreeRTOS, CoreMQTT and the HAL for the dev kit.

|  | Middleware library size | | Total Size | |
|---------|----------|---------|---------|---------
|**Sample** | **Flash (text,rodata,data)** | **RAM1,RAM2(dss,data)** | **Flash (text,rodata,data)** | **RAM1,RAM2(dss,data)** |
| IoT Hub only | 16.4 KB | 12 bytes | 247 KB | 194.6 KB
| IoT Hub + DPS | 31.7 KB | 12 bytes | 262.6 KB | 195.7 KB
| IoT Hub + ADU | 31.8 KB | 16 bytes | 282.8 KB | 208.4 KB
| IoT Hub + ADU + DPS | 41.8 KB | 16 bytes | 294.1 KB | 209.5 KB

<details>
  <summary>How these numbers were calculated:</summary>

  - Flash includes (based on [MIMXRT1062xxxxx_flexspi_nor.ld](./MIMXRT1062xxxxx_flexspi_nor.ld) for ADU samples and [MIMXRT1062xxxxx_sdram.ld](./MIMXRT1062xxxxx_sdram.ld) for non-ADU samples):
    - .flash_config (for non-ADU samples)
    - .ivt (for non-ADU samples)
    - .boot_hdr (for ADU samples)
    - .interrupts
    - .text
    - .ARM
    - .preinit_array
    - .init_array
    - .fini_array
  - RAM1, RAM2 includes (based on [MIMXRT1062xxxxx_flexspi_nor.ld](./MIMXRT1062xxxxx_flexspi_nor.ld) for ADU samples and [MIMXRT1062xxxxx_sdram.ld](./MIMXRT1062xxxxx_sdram.ld) for non-ADU samples):
    - .data
    - .bss
  - Middleware values were calculated by filtering the map file by any entries with `libaz` in the file path.
  - Total values were calculated from the map file found at the root of the build directory by summing all values within the above specified sections.
</details>
