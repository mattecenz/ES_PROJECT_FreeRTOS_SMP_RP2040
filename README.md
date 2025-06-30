# ES PROJECT FreeRTOS SMP RP2040

We provide a simple library in order to deploy functions on multiple cores in the [Arduino Nano RP2040 connect](https://docs.arduino.cc/hardware/nano-rp2040-connect/) board.

## SETUP PROJECT FROM ZERO

**NB:** tested only in *Ubuntu* for the moment.

Step 1: **install the sdk for working with the Raspberry Pico**

Instructions taken from [this](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) document in the Appendix C.

In the parent directory you want to have the pico sdk installed:

```bash
$ mkdir pico
$ cd pico/
$ git clone https://github.com/raspberrypi/pico-sdk.git --branch master
$ cd pico-sdk
$ git submodule update --init
```

For ease of developement from now on you may want to export the environment variable `PICO_SDK_PATH` (or else you need to add it manually when compiling with `-DPICO_SDK_PATH=...`):

```bash
$ export PICO_SDK_PATH=<path>/pico-sdk
```

Step 2: **installing the toolchain**

Launch:

```bash
$ sudo apt update
$ sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

If you are in Ubuntu or Debian you may also need to run:

```bash
$ sudo apt install g++ libstdc++-arm-none-eabi-newlib
```

Step 3: **installing picotool**

Picotool is a useful program which can help us with flashing the code directly on our board.

We can simply clone the [official repository](https://github.com/raspberrypi/picotool) and then compile it:

**NB:** There is a compatibility issue with the current version of picotool (2.1.1) and CMake 4.0.0. In order to solve the issue one has either to manually fix the error in the `CMakeListst.txt` or compile the `develop` branch of the repository. The issue will be solved in the next release.

**NB:** Remember to have `PICO_SDK_PATH` enabled.

```bash
$ git clone https://github.com/raspberrypi/picotool.git
$ cd picotool
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=<path> -DPICOTOOL_FLAT_INSTALL=1 ..
$ make install
```

Then we can export the environment variable:

```bash
$ export picotool_DIR=<path>/picotool
```

Step 4: **compiling the project**

The main structure of the repository is taken from the [official examples](https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos) on GitHub.

**NB:** The script tries to manually find a FreeRTOS directory installation path, if it does not find it you may need to manually export the variable `FREERTOS_KERNEL_PATH`. If no FreeRTOS has been installed just go to the [official github page](https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/7ce8266bc5c6e13534959179295d7ec25f9e438c) and clone it.

To compile the project we can do:

```bash
$ mkdir build
$ cd build
$ cmake .. -DPICO_BOARD=arduino_nano_rp2040_connect
$ make
```

**NB:** the flag `-DPICO_BOARD=arduino_nano_rp2040_connect` tells the sdk for which board the project needs to be compiled.

**NB:** I needed to slightly change the cmake found on the repository, as it seems that the version I downloaded had some warnings which were not suppressed.

And in the `build/OnEitherCore` we should see the **.uf2** files which can be deployed in the board.

**NB:** if the test with the wifi module has to be compiled, the correct pico board is `-DPICO_BOARD=pico_w`.

## HOWTO NAVIGATE THE DIRECTORIES

The [CMakeLists.txt](./CMakeLists.txt) file decides which of the subdirectories to compile. 

For the moment the choice is manual, but can be improved by using environment variables (TODO).

The main test resides in [TestSemaphores](./TestSemaphores/).

The directory is composed of a cmake file which imports all the necessary files found in [include](./include/).

These files are both cmake commands to find the FreeRTOS kernel (if not specified manually) and FreeRTOS configuration files.

The real library is implemented in [LibraryFreeRTOS_RP2040.h](./include/LibraryFreeRTOS_RP2040.h), which is extensively commented.

## EXAMPLE USAGE

The library is very simple to use. Indeed in your `main.c` file you can create your function and then call the library by using two primitives:

* `create_test_pipeline_function()`: to be called outside of your main function. It is responsible for setting up the tasks which will run the test on both cores.
* `start_test_pipeline()`: called in the main function. It is resposible for creating the master task which will orchestrate the execution of the two slaves.

```c
#include "LibraryFreeRTOS_RP2040.h"

uint32_t addition(uint32_t num1, uint32_t num2){
    return num1+num2;
}

create_test_pipeline_function(test_add, uint32_t, "%ld", addition, 10, 5);

int main(){

    // Setup the SDK and the relative IO.
    start_hw();

    start_test_pipeline(test_add);

    // Launch the scheduler.
    start_FreeRTOS();

}
```

#
