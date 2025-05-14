cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_library(test_semaphore_common INTERFACE)
target_sources(test_semaphore_common INTERFACE
        main.c)
target_include_directories(test_semaphore_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(test_semaphore_common INTERFACE
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap1
        pico_stdlib
        pico_multicore)
target_compile_options( test_semaphore_common INTERFACE
        ### Gnu/Clang C Options
        $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
        #$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
        )

add_executable(test_semaphore_nonts)
target_link_libraries(test_semaphore_nonts on_either_core_common)
pico_add_extra_outputs(test_semaphore_nonts)
pico_enable_stdio_usb(test_semaphore_nonts 1)

add_executable(test_semaphore_ts)
target_link_libraries(test_semaphore_ts on_either_core_common)
target_compile_definitions(test_semaphore_ts PRIVATE
        THREAD_SAFE=1
)
pico_add_extra_outputs(test_semaphore_ts)
pico_enable_stdio_usb(test_semaphore_ts 1)
