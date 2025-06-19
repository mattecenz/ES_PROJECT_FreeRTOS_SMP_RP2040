#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"
#include "hardware/claim.h"

#define type_t int32_t
#define N_ITER 100

static type_t shared_variable = 0;

#ifdef USE_FREERTOS_LOCK
static SemaphoreHandle_t bin_sem;
#endif

#pragma GCC push_options
#pragma GCC optimize ("O0")

static void shared_addition(){
    for(uint32_t i=0;i<N_ITER;++i){
        #ifdef USE_FREERTOS_LOCK
        if (xSemaphoreTake(bin_sem, portMAX_DELAY)) {
        #endif
        #ifdef USE_SDK_LOCK
        uint32_t lock = hw_claim_lock();
        #endif
        shared_variable++;
        #ifdef USE_FREERTOS_LOCK
        xSemaphoreGive(bin_sem);
        }
        #endif
        #ifdef USE_SDK_LOCK
        hw_claim_unlock(lock);
        #endif
    }
}

static void shared_subtraction(){
    for(uint32_t i=0;i<N_ITER;++i){
        #ifdef USE_FREERTOS_LOCK
        if (xSemaphoreTake(bin_sem, portMAX_DELAY)) {
        #endif
        #ifdef USE_SDK_LOCK
        uint32_t lock = hw_claim_lock();
        #endif
        shared_variable--;
        #ifdef USE_FREERTOS_LOCK
        xSemaphoreGive(bin_sem);
        }
        #endif
        #ifdef USE_SDK_LOCK
        hw_claim_unlock(lock);
        #endif
    }
}

#pragma GCC pop_options

create_test_pipeline_void_functions(test_ts, 
    type_t, 
    "%ld", 
    DEFAULT_CHECK, 
    0,
    shared_variable
    shared_addition, 
    shared_subtraction)

int main(void) {

    start_hw();

    #ifdef USE_FREERTOS_LOCK
    bin_sem = xSemaphoreCreateBinary();
    if (bin_sem == NULL) {
        //printf("Errore creazione semaforo\n");
        vTaskDelete(NULL);
    }
    xSemaphoreGive(bin_sem);
    #endif

    start_test_pipeline(test_ts);

    start_FreeRTOS();
}