# Note that this file can be used as a top level CMakeLists.txt to build all the Demos (it includes
# them all, but they are usable in their own right, which is why no common work is done here)

cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)
# set constants

add_compile_definitions(PICO_DEFAULT_LED_PIN=21) # Default LED pin for RP2040 Pico

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

PROJECT(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(TestSemaphores)
add_subdirectory(TestSemaphoresSingleExec)
add_subdirectory(TestOperations)
add_subdirectory(TestQueue)

#add_subdirectory(TestFreeRTOSWifi)