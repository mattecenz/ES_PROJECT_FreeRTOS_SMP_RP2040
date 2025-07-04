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
    (NOT USED)

    Macro used to create the function associated to the master task.

    Arguments:
        - test_name         : unique identifier of the test name
        - conversion_char   : identifier to convert the result of the function in a string.
        - return_variable   : name of the variable where the results of the test are stored
        - check_function    : function used to check the results of the two cores.

    The master is designated to create the tasks to be launched on each core,
    and it waits for their completion to collect and print their results.
*/

#define create_master_function(test_name, conversion_char, return_variable, check_function)             \
static void vMasterFunction_##test_name() {                                                             \
    bool check_result = false;                                                                          \
    while(!check_result){                                                                               \
        TaskHandle_t vSlaveFunctionHandles[RP2040config_testRUN_ON_CORES];                              \
        for(int i=0;i<RP2040config_testRUN_ON_CORES; ++i){                                              \
            xTaskCreate(vSlaveFunction_##test_name,                                                     \
                STRING(vSlaveFunction_##test_name)STRING(n),                                            \
                RP2040config_tskSLAVE_STACK_SIZE,                                                       \
                &return_variable[i],                                                                    \
                RP2040config_tskSLAVE_PRIORITY,                                                         \
                &vSlaveFunctionHandles[i]);                                                             \
            vTaskCoreAffinitySet(vSlaveFunctionHandles[i], (1 << i));                                   \
        }                                                                                               \
        for (int i = 0; i<RP2040config_testRUN_ON_CORES; i++) {                                         \
            ulTaskNotifyTake(pdFALSE, portMAX_DELAY);                                                   \
        }                                                                                               \
        CHECK_GENERATION(check_function, return_variable)                                               \
        if(!check_result){                                                                              \
            printf(STRING(test_name)"> check_result: NOT_EQUALS\n");                                    \
        }                                                                                               \
    }                                                                                                   \
    printf(STRING(test_name)" has ended correctly!\n");                                                 \
    for(int i=0;i<RP2040config_testRUN_ON_CORES; ++i){                                                  \
        printf(                                                                                         \
            STRING(test_name)"> return_core_%d:\t" conversion_char"\n",                                 \
            i,  return_variable[i].return_value);                                                       \
        printf(                                                                                         \
            STRING(test_name)"> return_time_%d:\t %llu \n",                                             \
            i,  return_variable[i].return_time);                                                        \
    }                                                                                                   \
    vTaskDelete(NULL);                                                                                  \
}                                                                                                       \

/*
    Macro used to check the results of the cores.

    It is called after all the tasks have been created and executed.
    The function passed is used to check all values are equal by pair.

    Arguments:
        - check_function : function used to check the results of the cores.
*/

#define CHECK_GENERATION(check_function, variables)                                                     \
    bool equal = true;                                                                                  \
    for(int i=0; i<RP2040config_testRUN_ON_CORES-1; i++){                                               \
        if(!check_function(variables[i].return_value, variables[i+1].return_value)){                    \
            equal = false;                                                                              \
            break;                                                                                      \
        }                                                                                               \
    }                                                                                                   \
    check_result = equal;                                                                               \



/*
    Macro used to define the default behavior of the master function
    when the check_function is not cutomized.

    It simply returns the negated XOR of the two return values (so if they are the same).
*/

#define DEFAULT_CHECK(return_val_0, return_val_1) \
    !(return_val_0 ^ return_val_1)                 \

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
        - check_function    : function used to check the results of the two cores.
        - ...               : arguments passed as inputs to the function.

    It creates RP2040config_testRUN_ON_CORES + 1 tasks specified in this way:

    RP2040config_testRUN_ON_CORES slave tasks, to be submitted one per core. 
    The slave tasks call the function specified in the input of the macro.

    1 master task, which creates the two slaves, assigns and schedules them.
    Then it will pause until both tasks are finished and it prints the result on the serial port.

    For the moment it is assumed that the type returned by the task is a simple type.

    At the end all the values produced by each core will be compared, expecting them to be all equal. 

 */

#define create_test_pipeline_function(test_name, return_type, conversion_char, function_name, check_function, ...)          \
static TaskHandle_t masterTaskHandle_##test_name = NULL;                                                    \
                                                                                                            \
struct return_info_##test_name{                                                                             \
    return_type return_value;                                                                               \
    uint64_t    return_time;                                                                                \
};                                                                                                          \
static struct return_info_##test_name return_info_##test_name[RP2040config_testRUN_ON_CORES];               \
                                                                                                            \
static void vSlaveFunction_##test_name(void *pvParameters){                                                 \
    save_time_now();                                                                                        \
    ((struct return_info_##test_name *) pvParameters)->return_value=function_name(__VA_ARGS__);             \
    ((struct return_info_##test_name *) pvParameters)->return_time=calc_time_diff();                        \
    xTaskNotifyGive(masterTaskHandle_##test_name);                                                          \
    vTaskDelete(NULL);                                                                                      \
}                                                                                                           \
                                                                                                            \
static void vMasterFunction_##test_name() {                                                                 \
    bool check_result = false;                                                                              \
    while(!check_result){                                                                                   \
        TaskHandle_t vSlaveFunctionHandles[RP2040config_testRUN_ON_CORES];                                  \
        for(int i=0;i<RP2040config_testRUN_ON_CORES; ++i){                                                  \
            xTaskCreate(vSlaveFunction_##test_name,                                                         \
                STRING(vSlaveFunction_##test_name)STRING(n),                                                \
                RP2040config_tskSLAVE_STACK_SIZE,                                                           \
                &return_info_##test_name[i],                                                                \
                RP2040config_tskSLAVE_PRIORITY,                                                             \
                &vSlaveFunctionHandles[i]);                                                                 \
            vTaskCoreAffinitySet(vSlaveFunctionHandles[i], (1 << i));                                       \
        }                                                                                                   \
        for (int i = 0; i<RP2040config_testRUN_ON_CORES; i++) {                                             \
            ulTaskNotifyTake(pdFALSE, portMAX_DELAY);                                                       \
        }                                                                                                   \
        CHECK_GENERATION(check_function, return_info_##test_name)                                           \
        if(!check_result){                                                                                  \
            printf(STRING(test_name)"> check_result: NOT_EQUALS\n");                                        \
        }                                                                                                   \
    }                                                                                                       \
    printf(STRING(test_name)" has ended correctly!\n");                                                     \
    for(int i=0;i<RP2040config_testRUN_ON_CORES; ++i){                                                      \
        printf(                                                                                             \
            STRING(test_name)"> return_core_%d:\t" conversion_char"\n",                                     \
            i,  return_info_##test_name[i].return_value);                                                   \
        printf(                                                                                             \
            STRING(test_name)"> return_time_%d:\t %llu \n",                                                 \
            i,  return_info_##test_name[i].return_time);                                                    \
    }                                                                                                       \
    vTaskDelete(NULL);                                                                                      \
}                                                                                                           \

/**
    Macro which creates the testing pipeline for void functions.

    Arguments:

        - test_name             : unique identifier of the test name
        - return_type           : return value of the function.
        - conversion_char       : identifier to convert the result of the function in a string (e.g. int32_t -> "%d").
        - check_function        : name of the function used for error checking.
        - return_name           : name of the shared variable accessed by the void functions.
        - expected_value        : constexpr containing the expected value of return_name at the end of the execution.
        - ...                   : name of the functions launched (one per core).

    It has the same structure as "create_test_pipeline_function", but it is adapted for working with void functions.

    Indeed for each core you submit a function having the prototype:
        void fnc();

    With the assumptions that it will modify the shared variable stored in return_name.

    At the end the value in return_name will be compared with the value contained in expected_value, using the check_function.
 */

#define create_test_pipeline_void_functions(test_name, return_type, conversion_char, check_function, expected_value, return_name, ...)     \
static TaskHandle_t masterTaskHandle_##test_name = NULL;                                                    \
                                                                                                            \
struct return_info_##test_name{                                                                             \
    void (*fn_ptr)();                                                                                       \
    return_type return_value;                                                                               \
    uint64_t    return_time;                                                                                \
};                                                                                                          \
static struct return_info_##test_name return_info_##test_name[RP2040config_testRUN_ON_CORES];               \
                                                                                                            \
static void vSlaveFunction_##test_name(void *pvParameters){                                                 \
    save_time_now();                                                                                        \
    ((struct return_info_##test_name *) pvParameters)->fn_ptr();                                            \
    ((struct return_info_##test_name *) pvParameters)->return_value=return_name;                            \
    ((struct return_info_##test_name *) pvParameters)->return_time=calc_time_diff();                        \
    xTaskNotifyGive(masterTaskHandle_##test_name);                                                          \
    vTaskDelete(NULL);                                                                                      \
}                                                                                                           \
                                                                                                            \
static void vMasterFunction_##test_name() {                                                                 \
    void(*ptrs[RP2040config_testRUN_ON_CORES])() = { __VA_ARGS__ };                                         \
    for (unsigned int i = 0; i < sizeof ptrs / sizeof ptrs[0]; i++)                                         \
        return_info_##test_name[i].fn_ptr=ptrs[i];                                                          \
    TaskHandle_t vSlaveFunctionHandles[RP2040config_testRUN_ON_CORES];                                      \
    for(int i=0;i<RP2040config_testRUN_ON_CORES; ++i){                                                      \
        xTaskCreate(vSlaveFunction_##test_name,                                                             \
            STRING(vSlaveFunction_##test_name)STRING(n),                                                    \
            RP2040config_tskSLAVE_STACK_SIZE,                                                               \
            &return_info_##test_name[i],                                                                    \
            RP2040config_tskSLAVE_PRIORITY,                                                                 \
            &vSlaveFunctionHandles[i]);                                                                     \
        vTaskCoreAffinitySet(vSlaveFunctionHandles[i], (1 << i));                                           \
    }                                                                                                       \
    for (int i = 0; i<RP2040config_testRUN_ON_CORES; i++) {                                                 \
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);                                                           \
    }                                                                                                       \
    bool check_result=check_function(return_name, expected_value);                                          \
    if(!check_result){                                                                                      \
        printf(STRING(test_name)"> check_result: NOT_EQUALS\n");                                            \
    }                                                                                                       \
    printf(STRING(test_name)" has ended!\n");                                                               \
    for(int i=0;i<RP2040config_testRUN_ON_CORES; ++i){                                                      \
        printf(                                                                                             \
            STRING(test_name)"> return_core_%d:\t" conversion_char"\n",                                     \
            i,  return_info_##test_name[i].return_value);                                                   \
        printf(                                                                                             \
            STRING(test_name)"> return_time_%d:\t %llu \n",                                                 \
            i,  return_info_##test_name[i].return_time);                                                    \
    }                                                                                                       \
    vTaskDelete(NULL);                                                                                      \
}                                                                                                           \

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