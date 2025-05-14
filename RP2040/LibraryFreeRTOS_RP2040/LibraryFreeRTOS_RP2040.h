#ifndef LIBRARY_FREE_RTOS_RP2040_H
#define LIBRARY_FREE_RTOS_RP2040_H

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

/**
    Method to setup all the pico hw underneath.
*/

void start_hw(){
    /* Want to be able to printf */
    stdio_init_all();
    /* And flash LED */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    /* Wait some seconds for setup finish */
    sleep_ms(5000);
}

void start_FreeRTOS(){
    vTaskStartScheduler();
}

#define create_test_setup_core_n(return_type,function_name, __VA_ARGS__)    \
static void vSlaveTaskCore_##n(void* pvParameters){                         \
    return_core_##n=function_name(__VA_ARGS__);                             \
    xTaskNotifyGive(masterTaskHandle);                                      \
    vTaskDelete(NULL);                                                      \
}                                                                           \

/**
Macro which creates 3 tasks specified in this way:

2 slave tasks, to be submitted one per core. 
The slave tasks call the function specified in the input of the macro.

1 master task, which creates the two slaves, assigns and schedules them.
Then it will pause until both tasks are finished and it prints the result on the serial port.

NB: it is assumed that the type returned by the task for the moment is a simple type.

 */

#define create_test_setup_pipeline(handle, return_type, conversion_char, function_name, __VA_ARGS__)    \
static TaskHandle_t* masterTaskHandle = NULL;                                                           \
static return_type return_core_0;                                                                       \
static return_type return_core_1;                                                                       \
create_test_setup_core_0(return_type, function_name, __VA_ARGS__)                                       \
create_test_setup_core_1(return_type, function_name, __VA_ARGS__)                                       \
static void vMasterTask(void *pvParameters) {                                                           \
    masterTaskHandle = xTaskGetCurrentTaskHandle();                                                     \
    TaskHandle_t* slaveTaskHandleCore_0 = NULL;                                                         \
    TaskHandle_t* slaveTaskHandleCore_1 = NULL;                                                         \
    xTaskSuspendAll();                                                                                  \
    xTaskCreate(vSlaveTaskCore_0,                                                                       \
        function_name,                                                                                  \
        configMINIMAL_STACK_SIZE,                                                                       \
        params,                                                                                         \
        tskIDLE_PRIORITY + 1,                                                                           \
        slaveTaskHandleCore_0);                                                                         \
    vTaskCoreAffinitySet(slaveTaskHandleCore_0, (1 << 0));                                              \
    xTaskCreate(vSlaveTaskCore1,                                                                        \
        function_name,                                                                                  \
        configMINIMAL_STACK_SIZE,                                                                       \
        params,                                                                                         \
        tskIDLE_PRIORITY + 1,                                                                           \
        slaveTaskHandleCore_1);                                                                         \
    vTaskCoreAffinitySet(slaveTaskHandleCore_1, (1 << 1));                                              \
    xTaskResumeAll();                                                                                   \
    for (int i = 0; i < 2; i++) {                                                                       \
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);                                                        \
    }                                                                                                   \
    printf("\n=== FINAL RESULTS ===\n");                                                                \
    printf("return_core_0:\t ##conversion_char\n", return_core_0);                                      \
    printf("return_core_1:\t ##conversion_char\n", return_core_1);                                      \
    printf("========================\n");                                                               \
    vTaskDelete(NULL);                                                                                  \
}                                                                                                       \


#endif