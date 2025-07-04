#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"
#include "hardware/claim.h"
#include "FreeRTOSConfig.h"
#include "pico/stdlib.h"
#include "pico/rand.h"

#define TEMPERATURE_QUEUE_LENGTH 100
#define TEMPERATURE_GENERATION_PERIOD 1000
#define MASTER_SLAVE_QUEUE_LENGTH 2 // since there are only 2 slaves it should be enough.
#define MASTER_DELAY 500 // half of the period to popolate the temperature queue.

// Define a shared queue and the relative lock between a generic task and the master.
static QueueHandle_t temperature_queue;

// Generic task
void vTaskTemperatureGenerator(){
    // NB: the time is in ticks
    vTaskDelay(TEMPERATURE_GENERATION_PERIOD);

    // Generate a random uniform number (in Kelvin) between 10 and 30 degrees (Celsius).
    uint32_t temp = get_rand_32()%21 + 273 +10;

    // This post to the queue copies the value. 
    if(xQueueSendToBack(temperature_queue, (void*) &temp, 0) != pdPASS){
        // In case it is full then it returns immediately (our value is simulated, nothing happens if its lost).
    }
}

// Define a shared queue where master and slave can communicate safely 
// NB: it could be done at library level too, or the values could be passed as parameters too
static QueueHandle_t master_slave_queue;

// This function will be executed once at the start of the master.
void vTaskMasterSetup(){
    // Create the master slave queue
    master_slave_queue = xQueueCreate(MASTER_SLAVE_QUEUE_LENGTH, sizeof(uint32_t));

    if(temperature_queue == NULL){
        // printf("Error during creation of master-slave queue\n");
        vTaskDelete(NULL);
    }
}

// This function will be launched at each iteration of the master.
// After the execution of this function the master will submit the work to the slaves.
// Hence here we need to prepare the values for the slaves.
void vTaskMasterLoop(){
    // Read the data from the shared queue
    uint32_t temp_read;

    while(xQueueReceive(temperature_queue, &temp_read, 0) == errQUEUE_EMPTY);

    // Copy the data two times in the slave queue
    for(uint32_t i=0;i<2;++i){
        if(xQueueSendToBack(master_slave_queue, (void*) &temp_read, 0)!=pdPASS){
            // There should be always space in the queue if everything is synchronized well.
            // printf("Errore sending temperature on master-slave queue from master. We should never arrive here");
            vTaskDelete(NULL);
            // Here we should also delete the slaves.
        }
    }

    // From here on the library will wake up the two slave tasks which will do their job
}

// This method will be called after the slaves have finished.
// In our case it will pop the values from the queue and check if they are equal.
bool vTaskMasterCheck(){
    uint32_t temp_slaves[2];

    for(int i=0;i<2;++i){
        if(xQueueReceive(temperature_queue, &(temp_slaves[i]), 0) == errQUEUE_EMPTY){
            // printf("Master error when reading the master-slave queue. We should never arrive here\n");
            vTaskDelete(NULL);
            // Here we should also delete the slaves.
        }
    }
    
    // Check equality
    return temp_slaves[0]==temp_slaves[1];
}

// This function will be executed once at the start of the slave.
// NB: Maybe it is not needed at all
void vTaskSlaveSetup(){
    // Do nothing
}

// This function will be called every time the slave is woken up by the master.
// The value will be rewritten into the shared queue.
uint32_t vTaskSlaveLoop(){
    // Read the data from the master-slave queue.
    // Both of the tasks will read from the
    uint32_t temp_read;

    // In theory the queue should never be empty
    if(xQueueReceive(temperature_queue, &temp_read, 0) == errQUEUE_EMPTY){
        // printf("Slave error when reading the master-slave queue. Return an invalid value")
        return -1;
    }

    // Return the temperature in Celsius in the master-slave queue.
    temp_read=temp_read-273;

    if(xQueueSendToBack(master_slave_queue, (void*) &temp_read, 0)!=pdPASS){
        // There should be always space in the queue if everything is synchronized well.
        // printf("Errore sending temperature on master-slave queue from slave. We should never arrive here");
        return;
    }
}

// Prototype of library call.
// create_test_pipeline(test_temperature // name of the test
//      vTaskMasterSetup,
//      vTaskMasterLoop,
//      vTaskMasterCheck,
//      vTaskSlaveSetup,
//      vTaskSlaveLoop,
//      uint32_t, // type returned by the slave loop
//      "%ld", // useful if we want to print the value
//)

int main(void) {

    start_hw();

    // NB: this is dynamically allocated. Maybe worth to explore xQueueCreateStatic https://syop.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/02-xQueueCreateStatic ?
    temperature_queue = xQueueCreate(TEMPERATURE_QUEUE_LENGTH, sizeof(uint32_t));

    if(temperature_queue == NULL){
        ////printf("Errore creazione coda temperatura\n");
        return;
    }

    // Prototype of library call.
    // start_test_pipeline(test_tempetaure);

    // Here we create the taks for the temperature generator.
    xTaskCreate(vTaskTemperatureGenerator,    
        "vTaskTemperatureGenerator",     
        configMINIMAL_STACK_SIZE,      
        NULL,  // no params                                 
        tskIDLE_PRIORITY,        
        NULL);         

    start_FreeRTOS();
}