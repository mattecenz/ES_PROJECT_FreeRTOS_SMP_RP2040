#include "LibraryFreeRTOS_RP2040.h"
#include "ApplicationHooks.h"
#include "hardware/claim.h"
#include "FreeRTOSConfig.h"
#include "pico/stdlib.h"
#include "pico/rand.h"

#define TEMPERATURE_QUEUE_LENGTH 100
#define TEMPERATURE_GENERATION_PERIOD 1000
#define TEMPERATURE_READS 20 // Number of temperature readings to be generated before the test ends.
#define MASTER_READS 5 // Number of temperature readings to be processed by the master before the test ends.
#define MASTER_DELAY 500 // half of the period to popolate the temperature queue.

// Define a shared queue and the relative lock between a generic task and the master.
static QueueHandle_t temperature_queue;

// Generic task
static void vTaskTemperatureGenerator(){
    // NB: the time is in ticks
    for(int i=0; i<TEMPERATURE_READS; ++i){
        vTaskDelay(TEMPERATURE_GENERATION_PERIOD);

    // Generate a random uniform number (in Kelvin) between 10 and 30 degrees (Celsius).
        uint32_t temp = get_rand_32()%21 + 273 +10;
        printf("Temperature generated: %ld K\n", temp);
        // This post to the queue copies the value. 
        if(xQueueSendToBack(temperature_queue, (void*) &temp, 0) != pdPASS){
        // In case it is full then it returns immediately (our value is simulated, nothing happens if its lost).
        }
    }
    vTaskDelete(NULL); // Delete the task when done.
}

static void vTaskMasterSetup();
static void vTaskMasterLoop();
static void vTaskSlaveSetup();
static uint32_t vTaskSlaveLoop(void* param);

create_test_pipeline(
    test_temperature, // name of the test
    vTaskMasterSetup,
    vTaskMasterLoop,
    vTaskSlaveSetup,
    vTaskSlaveLoop,
    uint32_t,
    "%lu" // useful if we want to print the value
)
static int count;
// This function will be executed once at the start of the master.
static void vTaskMasterSetup(){
    if(temperature_queue == NULL){
        // printf("Error during creation of master-slave queue\n");
        vTaskDelete(NULL);
    }
    count = 0;
}

// This function will be launched at each iteration of the master.
// After the execution of this function the master will submit the work to the slaves.
// Hence here we need to prepare the values for the slaves.
static void vTaskMasterLoop(){
    // Read the data from the shared queue
    uint32_t temp_read;
    uint32_t result; //returned value from the slaves
    bool outcome; // outcome of the check performed by the slaves

    if(count == MASTER_READS){
        exit_test_pipeline(test_temperature)
    }
    while(xQueueReceive(temperature_queue, &temp_read, portMAX_DELAY) == errQUEUE_EMPTY);

    // Copy the data two times in the slave queue
    prepare_input_for_slaves(test_temperature, temp_read)
    
    // From here on the library will wake up the two slave tasks which will do their job
    receive_output_from_slaves(test_temperature, DEFAULT_CHECK, result, outcome)
    if(outcome){
        printf("HELLO, MASTER HERE, just received a temperature: %ld C\n", result);
    } else {
        printf("HELLO, MASTER HERE, something went wrong with the slaves!\n");
    }
    count++;
}


// This function will be executed once at the start of the slave.
// NB: Maybe it is not needed at all
static void vTaskSlaveSetup(){
    //slave body
}

// This function will be called every time the slave is woken up by the master.
// The value will be rewritten into the shared queue.
static uint32_t vTaskSlaveLoop(void* param){                                                                                 
    int32_t temp_read = *((int32_t*) param);
    
    // Return the temperature in Celsius in the master-slave queue.
    temp_read=temp_read-273+get_rand_32()%2; // add some noise to the temperature reading

    return temp_read;
}


int main(void) {

    start_hw();

    // NB: this is dynamically allocated. Maybe worth to explore xQueueCreateStatic https://syop.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/02-xQueueCreateStatic ?
    temperature_queue = xQueueCreate(TEMPERATURE_QUEUE_LENGTH, sizeof(uint32_t));

    if(temperature_queue == NULL){
        ////printf("Errore creazione coda temperatura\n");
        return -1;
    }

    // Prototype of library call.
    start_test_pipeline(test_temperature);

    // Here we create the taks for the temperature generator.
    xTaskCreate(vTaskTemperatureGenerator,    
        "vTaskTemperatureGenerator",     
        configMINIMAL_STACK_SIZE,      
        NULL,  // no params                                 
        tskIDLE_PRIORITY,        
        NULL);
    
    start_FreeRTOS();
}