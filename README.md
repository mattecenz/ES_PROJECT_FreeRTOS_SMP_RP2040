# ES PROJECT FreeRTOS SMP RP2040

**TODO:** add description.

## SETUP PROJECT FROM ZERO

**NB:** tested only in *Ubuntu* for the moment.

Step 1: **install the sdk for working with the Raspberry Pico**

Instructions taken from [this](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) document in the Appendix C.

In the parent directory you want to have the pico sdk installed:

```
$ mkdir pico
$ cd pico/
$ git clone https://github.com/raspberrypi/pico-sdk.git --branch master
$ cd pico-sdk
$ git submodule update --init
```

For ease of developement from now on you may want to export the environment variable `PICO_SDK_PATH` (or else you need to add it manually when compiling with `-DPICO_SDK_PATH=...`):

```
$ export PICO_SDK_PATH=../../pico-sdk
```

Step 2: **installing the toolchain**

Launch:

```
$ sudo apt update
$ sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

If you are in Ubuntu or Debian you may also need to run:

```
$ sudo apt install g++ libstdc++-arm-none-eabi-newlib
```

Step 3: **installing picotool**

Picotool is a useful program which can help us with flashing the code directly on our board.

We can simply clone the [official repository](https://github.com/raspberrypi/picotool) and then compile it:

**NB:** When using the latest version of CMake 4.0.0 (and onwards), the support of versions <3.5 has been removed. So we need to update `pico-sdk/lib/mbedtls/CMakeLists.txt:23` and put the minimum version to 3.5. This is a problem of the open source project and it cannot be avoided. (At the moment I have opened an [Issue on Github](https://github.com/Mbed-TLS/mbedtls/issues/10123),let's see what they answer with).

```
$ git clone https://github.com/raspberrypi/picotool.git
$ cd picotool
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=<path> -DPICOTOOL_FLAT_INSTALL=1 ..
$ make install
```

**NB:** Remember to have `PICO_SDK_PATH` enabled.

Then we can export the environment variable:

```
$ export picotool_DIR=<path>/picotool
```

Step 4: **compiling the project**

The main structure of the repository is taken from the [official examples](https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos) on GitHub.

**NB:** The script tries to manually find a FreeRTOS directory installation path, if it does not find it you may need to manually export the variable `FREERTOS_KERNEL_PATH`. If no FreeRTOS has been installed just go to the [official github page](https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/7ce8266bc5c6e13534959179295d7ec25f9e438c) and clone it.

To compile the project we can do:

```
$ cd RP2040
$ mkdir build
$ cd build
$ cmake .. -DPICO_BOARD=arduino_nano_rp2040_connect
$ make
```

**NB:** the flag `-DPICO_BOARD=arduino_nano_rp2040_connect` tells the sdk for which board the project needs to be compiled.

**NB:** I needed to slightly change the cmake found on the repository, as it seems that the version I downloaded had some warnings which were not suppressed.

And in the `build/OnEitherCore` we should see the **.uf2** files which can be deployed in the board.