cmake_minimum_required(VERSION 3.13)

project(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../include)

pico_sdk_init()

add_executable(picow_test_wifi_freertos
        main.c
        picow_tcp_client.c
        )
target_compile_definitions(picow_test_wifi_freertos PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
        )
target_include_directories(picow_test_wifi_freertos PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${PROJECT_INCLUDE_DIR}
        )

target_link_libraries(picow_test_wifi_freertos
        pico_cyw43_arch_lwip_threadsafe_background
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap1
        pico_stdlib
        pico_multicore
        )
target_compile_options( picow_test_wifi_freertos PRIVATE
        ### Gnu/Clang C Options
        $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
        #$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
        )

pico_add_extra_outputs(picow_test_wifi_freertos)
