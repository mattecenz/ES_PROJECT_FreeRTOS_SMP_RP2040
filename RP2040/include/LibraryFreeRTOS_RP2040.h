// ------------------------------------------------------------------------ //
//  INTRODUCTION                                                            //
// ------------------------------------------------------------------------ //

/*

LIBRARY FOR USING FreeRTOS on Raspberry Pico 2040

PREREQUISITES:

Having pico-sdk installed           : https://github.com/raspberrypi/pico-sdk
Having FreeRTOS kernel installed    : https://github.com/FreeRTOS/FreeRTOS-Kernel

Authors:

- Matteo Briscini
- Matteo Cenzato
- Michele Adorni

*/

#ifndef LIBRARY_FREE_RTOS_RP2040_H
#define LIBRARY_FREE_RTOS_RP2040_H

// ------------------------------------------------------------------------ //
//  GENERIC INCLUDES                                                        //
// ------------------------------------------------------------------------ //

#include "FreeRTOS.h" /* Must come first. */
#include "LibraryFreeRTOS_RP2040Config.h"
#include "task.h"     /* RTOS task related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "pico/multicore.h"

// ------------------------------------------------------------------------ //
//  UTILITIES MACROS                                                        //
// ------------------------------------------------------------------------ //

/*
    Wrapper to convert macro argument into string.
*/

#define STRING(s) #s

/*
    Creator of the return struct type and static variable.
    It needs to be specified the name of the test in order to create a unique struct per test.
    For now it contains the return value associated to the functions 
    deployed on each core and their respective timings.
*/

#define create_return_struct(test_name, return_type)                            \
struct struct_return_info_##test_name{                                          \
    return_type return_core_0;                                                  \
    return_type return_core_1;                                                  \
    uint64_t    return_time_0;                                                  \
    uint64_t    return_time_1;                                                  \
};                                                                              \
static struct struct_return_info_##test_name return_info_##test_name;           \

/*
    Macro used to store in an internal variable the time read 
    from the internal hw timer of the RP2040.
*/

#define save_time_now()                                 \
absolute_time_t saved_time = get_absolute_time()        \

/*
    Macro callable only after "save_time_now()".
    It returns the difference between the instant of time saved before
    and the current time as a uint64_t value.
*/

#define calc_time_diff()                                \
absolute_time_diff_us(saved_time,get_absolute_time())   \

/*
    Macro used for creating the function wrapper which runs on a specific core.

    Arguments:
        - test_name     : unique identifier of the test name
        - n             : designated core.
        - return_type   : return value of the function (specified by the dev).
        - function_name : name of the function to be called (specified by the dev).
        - ...           : arguments passed as inputs to the function (specified by the dev).

    The task executes the function and stores 
    both the output value and the time taken to run it.
*/

#define create_slave_function(test_name, n, return_type, function_name, ...)        \
static void vSlaveFunction_##test_name##n(){                                        \
    save_time_now();                                                                \
    return_info_##test_name.return_core_##n=function_name(__VA_ARGS__);             \
    return_info_##test_name.return_time_##n=calc_time_diff();                       \
    xTaskNotifyGive(masterTaskHandle_##test_name);                                  \
    vTaskDelete(NULL);                                                              \
}                                                                                   \

/*
    Macro used for creating a wrapper for a void function to be launched on a specific core.

    Arguments:
        - test_name     : unique identifier of the test name
        - n             : designated core.
        - function_name : name of the function to be called (specified by the dev).

    The task executes the function and stores
    the time taken to run it.
*/

#define create_slave_void_function(test_name, n, function_name)             \
static void vSlaveFunction_##test_name##n(){                                \
    save_time_now();                                                        \
    function_name();                                                        \
    return_info_##test_name.return_time_##n=calc_time_diff();               \
    xTaskNotifyGive(masterTaskHandle_##test_name);                          \
    vTaskDelete(NULL);                                                      \
}                                                                           \

/*
    Macro used to wrap the FreeRTOS function used for creating tasks, 
    and assign the task to a specific core.

    Arguments:
        - n             : designated core.
        - task_name     : unique identifier used to recognize the task.
        - return_handle : TaskHandle_t object used to store the information about the task.

    The priorities of the tasks and the task size are taken by the Config file defined by the user.
*/

#define create_slave_task_on_core(n, task_name, return_handle)                      \
xTaskCreate(task_name,                                                              \
    STRING(task_name),                                                              \
    RP2040config_tskSLAVE_STACK_SIZE##n,                                            \
    NULL,                                                                           \
    RP2040config_tskSLAVE_PRIORITY##n,                                              \
    &return_handle);                                                                \
vTaskCoreAffinitySet(return_handle, (1 << n));                                      \

/*
    Macro used to create the function associated to the master task.

    Arguments:
        - test_name         : unique identifier of the test name
        - conversion_char   : identifier to convert the result of the function in a string.
        - return_val_0      : name of the variable where the result of the core 0 is stored.
        - return_val_1      : name of the variable where the result of the core 1 is stored.

    The master is designated to create the tasks to be launched on each core,
    and it waits for their completion to collect and print their results.
*/

#define create_master_function(test_name, conversion_char, return_val_0, return_val_1)                  \
static void vMasterFunction_##test_name() {                                                             \
    TaskHandle_t vSlaveFunctionHandle0_##test_name = NULL;                                              \
    TaskHandle_t vSlaveFunctionHandle1_##test_name = NULL;                                              \
    create_slave_task_on_core(0, vSlaveFunction_##test_name##0, vSlaveFunctionHandle0_##test_name)      \
    create_slave_task_on_core(1, vSlaveFunction_##test_name##1, vSlaveFunctionHandle1_##test_name)      \
    for (int i = 0; i < 2; i++) {                                                                       \
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);                                                       \
    }                                                                                                   \
    printf(STRING(test_name)" has ended correctly!\n");                                                 \
    printf(STRING(test_name)"> return_core_0:\t" conversion_char"\n", return_val_0);                    \
    printf(STRING(test_name)"> return_core_1:\t" conversion_char"\n", return_val_1);                    \
    printf(STRING(test_name)"> return_time_0:\t %llu \n", return_info_##test_name.return_time_0);       \
    printf(STRING(test_name)"> return_time_1:\t %llu \n", return_info_##test_name.return_time_1);       \
    vTaskDelete(NULL);                                                                                  \
}                                                                                                       \

// ------------------------------------------------------------------------ //
//  PUBLIC INTERFACE                                                        //
// ------------------------------------------------------------------------ //

/**
    Method which calls all the initialization functions of the pico hardware.

    It then waits for 5 seconds just to be sure that everything has connected properly.
*/

void start_hw(){
    // Want to be able to printf
    stdio_init_all();
    // Flash LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    // Wait some time for all the setup to complete (just to be sure)
    sleep_ms(5000);
}

/**
    Method which starts the FreeRTOS scheduler.

    It should be called after the tasks have already been created.
*/

void start_FreeRTOS(){
    vTaskStartScheduler();
}

/**
    Macro which creates the testing pipeline for non-void functions.

    Arguments:

        - test_name         : unique identifier of the test name
        - return_type       : return value of the function.
        - conversion_char   : identifier to convert the result of the function in a string (e.g. int32_t -> "%d").
        - function_name     : name of the function to be called.
        - ...               : arguments passed as inputs to the function.

    It creates 3 tasks specified in this way:

    2 slave tasks, to be submitted one per core. 
    The slave tasks call the function specified in the input of the macro.

    1 master task, which creates the two slaves, assigns and schedules them.
    Then it will pause until both tasks are finished and it prints the result on the serial port.

    For the moment it is assumed that the type returned by the task for the moment is a simple type.

 */

#define create_test_pipeline_function(test_name, return_type, conversion_char, function_name, ...)          \
static TaskHandle_t masterTaskHandle_##test_name = NULL;                                                    \
create_return_struct(test_name, return_type)                                                                \
create_slave_function(test_name, 0, return_type, function_name, __VA_ARGS__)                                \
create_slave_function(test_name, 1, return_type, function_name, __VA_ARGS__)                                \
create_master_function(test_name, conversion_char, return_info_##test_name.return_core_0, return_info_##test_name.return_core_1)    \

/**
    Macro which creates the testing pipeline for void functions.

    Arguments:

        - test_name             : unique identifier of the test name
        - return_type           : return value of the function.
        - conversion_char       : identifier to convert the result of the function in a string (e.g. int32_t -> "%d").
        - return_name           : name of the shared variable accessed by the void functions.
        - function_name_core0   : name of the function to be deployed on core 0.
        - function_name_core1   : name of the function to be deployed on core 1.

    It has the same structure as "create_test_pipeline_function",
    but it is adapted for working with void functions.

 */

#define create_test_pipeline_void_functions(test_name, return_type, conversion_char, return_name, function_name_core1, function_name_core2)     \
static TaskHandle_t masterTaskHandle_##test_name = NULL;                                        \
create_return_struct(test_name, return_type)                                                    \
create_slave_void_function(test_name, 0, function_name_core1)                                   \
create_slave_void_function(test_name, 1, function_name_core2)                                   \
create_master_function(test_name, conversion_char, return_name, return_name)                    \

/**
    Method which creates the master task assigned to a specific test name.

    The master task launches the master function.

    By default it has minimal stack size, no input parameters, and a priority equal to the
    one of the slave tasks + 1.

    Moreover it saves the return handle in a shared variable, used for notifying that the slaves 
    have terminated their execution.
*/

#define start_test_pipeline(test_name)      \
xTaskCreate(vMasterFunction_##test_name,    \
    "vMasterFunction"STRING(test_name),     \
    RP2040config_tskMASTER_STACK_SIZE,      \
    NULL,                                   \
    RP2040config_tskMASTER_PRIORITY,        \
    &masterTaskHandle_##test_name);         \

#endif