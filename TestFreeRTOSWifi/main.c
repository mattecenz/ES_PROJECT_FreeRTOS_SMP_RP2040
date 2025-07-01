#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"
#include "picow_tcp_client.h"

void connect_to_wifi(){

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
    }
}

int main(void) {

    start_hw();

    xTaskCreate(connect_to_wifi, "connect_to_wifi", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL);

    start_FreeRTOS();
}