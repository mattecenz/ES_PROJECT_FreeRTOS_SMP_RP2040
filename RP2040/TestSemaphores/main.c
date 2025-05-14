#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"

#define type_t int32_t
#define N_ITER 100

static type_t shared_variable = 0;

#ifdef THREAD_SAFE
static SemaphoreHandle_t bin_sem;
#endif

static void shared_addition(){
    for(uint32_t i=0;i<N_ITER;++i){
        #ifdef THREAD_SAFE
        if (xSemaphoreTake(bin_sem, portMAX_DELAY)) {
        #endif
        shared_variable++;
        #ifdef THREAD_SAFE
        xSemaphoreGive(bin_sem);
        }
        #endif
    }
}

static void shared_subtraction(){
    for(uint32_t i=0;i<N_ITER;++i){
        #ifdef THREAD_SAFE
        if (xSemaphoreTake(bin_sem, portMAX_DELAY)) {
        #endif
        shared_variable--;
        #ifdef THREAD_SAFE
        xSemaphoreGive(bin_sem);
        }
        #endif
    }
}

create_test_pipeline_void_functions(type_t, "%d", shared_variable, shared_addition, shared_subtraction)

int main(void) {

    start_hw();

    #ifdef THREAD_SAFE
    bin_sem = xSemaphoreCreateBinary();
    if (bin_sem == NULL) {
        //printf("Errore creazione semaforo\n");
        vTaskDelete(NULL);
    }
    xSemaphoreGive(bin_sem);
    #endif

    start_test_pipeline();

    start_FreeRTOS();
}