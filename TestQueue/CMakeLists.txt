cmake_minimum_required(VERSION 3.13)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../include)

pico_sdk_init()

add_executable(test_queue
        test_queue.c)

target_include_directories(test_queue PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PROJECT_INCLUDE_DIR}
        )

target_link_libraries(test_queue
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap1
        pico_stdlib
        pico_rand
        pico_multicore)

target_compile_options( test_queue PRIVATE
        ### Gnu/Clang C Options
        $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
        #$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
        )

pico_add_extra_outputs(test_queue)
pico_enable_stdio_usb(test_queue 1)
