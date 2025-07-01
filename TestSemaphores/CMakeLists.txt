cmake_minimum_required(VERSION 3.13)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../include)

pico_sdk_init()

add_library(test_semaphore_common INTERFACE)
target_sources(test_semaphore_common INTERFACE
        test_semaphores.c)
target_include_directories(test_semaphore_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PROJECT_INCLUDE_DIR}
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
target_link_libraries(test_semaphore_nonts test_semaphore_common)
pico_add_extra_outputs(test_semaphore_nonts)
pico_enable_stdio_usb(test_semaphore_nonts 1)

add_executable(test_semaphore_freertos_lock)
target_link_libraries(test_semaphore_freertos_lock test_semaphore_common)
target_compile_definitions(test_semaphore_freertos_lock PRIVATE
        USE_FREERTOS_LOCK=1
)
pico_add_extra_outputs(test_semaphore_freertos_lock)
pico_enable_stdio_usb(test_semaphore_freertos_lock 1)

add_executable(test_semaphore_sdk_lock)
target_link_libraries(test_semaphore_sdk_lock test_semaphore_common)
target_compile_definitions(test_semaphore_sdk_lock PRIVATE
        USE_SDK_LOCK=1
)
pico_add_extra_outputs(test_semaphore_sdk_lock)
pico_enable_stdio_usb(test_semaphore_sdk_lock 1)
