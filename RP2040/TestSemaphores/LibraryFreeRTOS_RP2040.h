#ifndef LIBRARY_FREE_RTOS_RP2040_H
#define LIBRARY_FREE_RTOS_RP2040_H

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include <stdio.h>
#include <stdlib.h>
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

/**
Method which starts the FreeRTOS scheduler.
*/
void start_FreeRTOS(){
    vTaskStartScheduler();
}

/**
Macro which creates 3 tasks specified in this way:

2 slave tasks, to be submitted one per core. 
The slave tasks call the function specified in the input of the macro.

1 master task, which creates the two slaves, assigns and schedules them.
Then it will pause until both tasks are finished and it prints the result on the serial port.

NB: it is assumed that the type returned by the task for the moment is a simple type.

 */

#define create_test_pipeline_function(return_type, conversion_char, function_name, ...)         \
static TaskHandle_t masterTaskHandle = NULL;                                                    \
static return_type return_core_0;                                                               \
static return_type return_core_1;                                                               \
create_slave_function(0, return_type, function_name, __VA_ARGS__)                               \
create_slave_function(1, return_type, function_name, __VA_ARGS__)                               \
create_master_function(conversion_char, function_name, return_core_0, return_core_1)            \

#define create_slave_function(n, return_type, function_name, ...)           \
static void vSlaveFunctionCore_##n(void* pvParameters){                     \
    return_core_##n=function_name(__VA_ARGS__);                             \
    xTaskNotifyGive(masterTaskHandle);                                      \
    vTaskDelete(NULL);                                                      \
}                                                                           \

/**
With this method we pass two functions, to be executed one per core.
Moreover we assume that both of the functions access the same shared variable, accessed by "return_name".
 */

#define create_test_pipeline_void_functions(return_type, conversion_char, return_name, function_name_core1, function_name_core2)    \
static TaskHandle_t masterTaskHandle = NULL;                                                    \
create_slave_void_function(0, function_name_core1)                                              \
create_slave_void_function(1, function_name_core2)                                              \
create_master_function(conversion_char, function_name, return_name, return_name)                \

#define create_slave_void_function(n, function_name)                        \
static void vSlaveFunctionCore_##n(void* pvParameters){                     \
    function_name();                                                        \
    xTaskNotifyGive(masterTaskHandle);                                      \
    vTaskDelete(NULL);                                                      \
}                                                                           \

#define create_task_on_core(n, task_name, return_handle)                            \
xTaskCreate(task_name,                                                              \
    "task_name",                                                                    \
    configMINIMAL_STACK_SIZE,                                                       \
    NULL,                                                                           \
    tskIDLE_PRIORITY + 1,                                                           \
    &return_handle);                                                                \
vTaskCoreAffinitySet(return_handle, (1 << n));                                      \

#define create_master_function(conversion_char, function_name, return_val_0, return_val_1)              \
static void vMasterFunction(void *pvParameters) {                                                       \
    masterTaskHandle = xTaskGetCurrentTaskHandle();                                                     \
    TaskHandle_t slaveTaskHandleCore_0 = NULL;                                                          \
    TaskHandle_t slaveTaskHandleCore_1 = NULL;                                                          \
    vTaskSuspendAll();                                                                                  \
    create_task_on_core(0, vSlaveFunctionCore_0, slaveTaskHandleCore_0)                                 \
    create_task_on_core(1, vSlaveFunctionCore_1, slaveTaskHandleCore_1)                                 \
    xTaskResumeAll();                                                                                   \
    for (int i = 0; i < 2; i++) {                                                                       \
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);                                                        \
    }                                                                                                   \
    printf("\n=== FINAL RESULTS ===\n");                                                                \
    printf("return_core_0:\t" conversion_char"\n", return_val_0);                                       \
    printf("return_core_1:\t" conversion_char"\n", return_val_1);                                       \
    printf("========================\n");                                                               \
    vTaskDelete(NULL);                                                                                  \
}                                                                                                       \

/**
Method which creates the master task.
 */

#define start_test_pipeline()       \
xTaskCreate(vMasterFunction,        \
    "MasterTask",                   \
    configMINIMAL_STACK_SIZE,       \
    NULL,                           \
    tskIDLE_PRIORITY + 1,           \
    NULL);                          \

#endif