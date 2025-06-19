#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"
#include "hardware/claim.h"

#define type_t int32_t
#define N_ITER 100

static type_t shared_variable_nolock = 0;
static type_t shared_variable_freertos_lock = 0;
static type_t shared_variable_sdk_lock = 0;

static SemaphoreHandle_t bin_sem;

#pragma GCC push_options
#pragma GCC optimize ("O0")

static void shared_addition_nolock(){
    for(uint32_t i=0;i<N_ITER;++i){
        shared_variable_nolock++;
    }
}

static void shared_subtraction_nolock(){
    for(uint32_t i=0;i<N_ITER;++i){
        shared_variable_nolock--;
    }
}

static void shared_addition_freertos_lock(){
    for(uint32_t i=0;i<N_ITER;++i){
        if (xSemaphoreTake(bin_sem, portMAX_DELAY)) {
            shared_variable_freertos_lock++;
            xSemaphoreGive(bin_sem);
        }
    }
}

static void shared_subtraction_freertos_lock(){
    for(uint32_t i=0;i<N_ITER;++i){
        if (xSemaphoreTake(bin_sem, portMAX_DELAY)) {
            shared_variable_freertos_lock--;
            xSemaphoreGive(bin_sem);
        }
    }
}

static void shared_addition_sdk_lock(){
    for(uint32_t i=0;i<N_ITER;++i){
        uint32_t lock = hw_claim_lock();
        shared_variable_sdk_lock++;
        hw_claim_unlock(lock);
    }
}

static void shared_subtraction_sdk_lock(){
    for(uint32_t i=0;i<N_ITER;++i){
        uint32_t lock = hw_claim_lock();
        shared_variable_sdk_lock--;
        hw_claim_unlock(lock);
    }
}

#pragma GCC pop_options

create_test_pipeline_void_functions(test_nolock, 
    type_t, 
    "%ld", 
    DEFAULT_CHECK,
    shared_variable_nolock, 
    0,
    shared_addition_nolock, 
    shared_subtraction_nolock)

create_test_pipeline_void_functions(test_freertos_lock, 
    type_t, 
    "%ld", 
    DEFAULT_CHECK,
    shared_variable_freertos_lock,
    0, 
    shared_addition_freertos_lock, 
    shared_subtraction_freertos_lock)

create_test_pipeline_void_functions(test_sdk_lock, 
    type_t, 
    "%ld", 
    DEFAULT_CHECK,
    shared_variable_sdk_lock,
    0,
    shared_addition_sdk_lock, 
    shared_subtraction_sdk_lock)

int main(void) {

    start_hw();

    bin_sem = xSemaphoreCreateBinary();
    if (bin_sem == NULL) {
        //printf("Errore creazione semaforo\n");
        vTaskDelete(NULL);
    }
    xSemaphoreGive(bin_sem);

    start_test_pipeline(test_nolock);
    start_test_pipeline(test_freertos_lock);
    start_test_pipeline(test_sdk_lock)

    start_FreeRTOS();
}