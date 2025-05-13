/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "stdio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "stdlib.h"


#define mainRUN_FREE_RTOS_ON_CORE 0 //free RTOS pinned to core 0

#define RAND_THRESHOLD 0.5 //threshold to simulate error rate

/* Priorities at which the tasks are created.  The event semaphore task is
given the maximum priority of ( configMAX_PRIORITIES - 1 ) to ensure it runs as
soon as the semaphore is given. */
#define mainOPERATIONS_PRIORITY             ( tskIDLE_PRIORITY + 4 )
#define mainSDK_MUTEX_USE_TASK_PRIORITY     ( tskIDLE_PRIORITY + 3 )
#define mainSDK_SEMAPHORE_USE_TASK_PRIORITY ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_RECEIVE_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_SEND_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1 )
#define mainEVENT_SEMAPHORE_TASK_PRIORITY   ( configMAX_PRIORITIES - 1 )

/* The rate at which data is sent to the queue, specified in milliseconds, and
converted to ticks using the pdMS_TO_TICKS() macro. */
#define mainQUEUE_SEND_PERIOD_MS            pdMS_TO_TICKS( 200 )

/* The period of the example software timer, specified in milliseconds, and
converted to ticks using the pdMS_TO_TICKS() macro. */
#define mainSOFTWARE_TIMER_PERIOD_MS        pdMS_TO_TICKS( 1000 )

/* The number of items the queue can hold.  This is 1 as the receive task
has a higher priority than the send task, so will remove items as they are added,
meaning the send task should always find the queue empty. */
#define mainQUEUE_LENGTH                    ( 10 )

/*-----------------------------------------------------------*/						

typedef struct dataToSend {
    float params[2];
    char operation;
}data_t;

/*
 * USED
 */
static void prvSetupHardware( void );
static float readFloatFromSerial();
static char readCharFromSerial();
static void operativeTask(void *pvParameters);
static void init();
static void launchOPTasks(void*parameters);
static float sum2nums(float a, float b);
static float sub2nums(float a, float b);
static float mul2nums(float a, float b);
static float div2nums(float a, float b);
static void handleRequests(void *pvParameters);
static void flushSerialInput();
/*
 * NOT USED
 */
static void vExampleTimerCallback( TimerHandle_t xTimer );
static void prvEventSemaphoreTask( void *pvParameters );
static void prvSDKMutexUseTask( void *pvParameters );
static void prvSDKSemaphoreUseTask( void *pvParameters );

/*-----------------------------------------------------------*/

/* The queue used by the queue send and queue receive tasks. */
static QueueHandle_t xQueue = NULL;

/* The semaphore (in this case binary) that is used by the FreeRTOS tick hook
 * function and the event semaphore task.
 */
static SemaphoreHandle_t xEventSemaphore = NULL;

/* The counters used by the various examples.  The usage is described in the
 * comments at the top of this file.
 */
static volatile uint32_t ulCountOfTimerCallbackExecutions = 0;
static volatile uint32_t ulCountOfItemsSentOnQueue = 0;
static volatile uint32_t ulCountOfItemsReceivedOnQueue = 0;
static volatile uint32_t ulCountOfReceivedSemaphores = 0;
static volatile uint32_t ulCountOfSDKMutexEnters = 0;
static volatile uint32_t ulCountOfSDKSemaphoreAcquires = 0;

/*-----------------------------------------------------------*/

#include "pico/mutex.h"
#include "pico/sem.h"


auto_init_mutex(xSDKMutex);
static semaphore_t xSDKSemaphore;

//static int params0[2]; //dummy values to test execution on core 0
//static int params1[2]; //dummy values to test execution on core 1


static void init() {
    
    //seems like they are already saved into task structure the value will be updated and not lost, even if pointer gets freed 
    TaskHandle_t xHandleRequests = NULL;
    xTaskCreate(handleRequests,
                "HR0",
                configMINIMAL_STACK_SIZE,
                NULL,
                mainQUEUE_RECEIVE_TASK_PRIORITY,
                &xHandleRequests);
    vTaskCoreAffinitySet(xHandleRequests, (1 << 0)); // Set task to run on core 0
}



int main(void) {
    //TEST DESCRIPTION: this test will create two tasks on core 0 and core 1, each task will sum two numbers and send the result to a queue. The queue will be read by a task on core 0.
    //TEST is performed with SMP and the queue address is static, therefore is accessible by both cores
    prvSetupHardware();
    sleep_ms(5000); //wait 5 sec for serial output to be ready 
    printf("Core %d: Launching FreeRTOS scheduler\n", get_core_num());

    /* Create the queue used by the queue send and queue receive tasks. */
    xQueue = xQueueCreate(     /* The number of items the queue can hold. */
            mainQUEUE_LENGTH,
    /* The size of each item the queue holds. */
            sizeof(float));


    init();
    vTaskStartScheduler();
    
    printf("Core %d: ended\n", get_core_num());

}

/*-----------------------------------------------------------*/
static void flushSerialInput() {
    // Consume all characters in the serial input buffer
    while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {
        // Do nothing, just clear the buffer
        // Maybe convert to just remove special characters like \n or \r later on
    }
}

static float readFloatFromSerial() {
    char buffer[32] = {0}; // Buffer to store the input
    int index = 0;
    char c;
    
    do{
        c = getchar();
    } while (c == ' ');
    
    // Read characters until Enter is pressed or buffer is full
    while (index < sizeof(buffer) - 1) {
        if (c == '\n' || c == '\r' || c =='\t' || c == ' '){ //in "Live ending" from sertial monitor, use LF(\n) or CR(\r), not both
            break; // Stop reading on Enter key
        }else if ((c< '0' || c > '9') && c != '.') { // non-numeric characters
            ungetc(c, stdin); // Put back the character to the input stream
            break;
        }
        if (c != PICO_ERROR_TIMEOUT) { // Ignore timeout errors
            buffer[index++] = c;
        }
        c = getchar(); // Non-blocking read with timeout
    }

    buffer[index] = '\0'; // Null-terminate the string

    // Convert the string to an float
    return atof(buffer);
}
static char readCharFromSerial() {
    int index = 0;
    char c;
    
    // Read characters until Enter is pressed or buffer is full
    while (1) {
        c = getchar(); // Non-blocking read with timeout
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') { // Ignore spaces and tabs
            return c;
        }
    }
}


static void handleRequests(void *pvParameters){ //parameters are not used, but needed to match the function signature
    printf("Core %d: Launching handleRequests\n", get_core_num());
    static int numsToCheck = 2; //number of result collected, maybe we want to expand it to more than 2 for redundancy
    float sum;
    data_t data; //data to be passed to the task
    float result[numsToCheck]; //result of the sum, to be passed to the queue
    bool equal = false; 
    for( ;; )
    {
        printf("Enter an operation:");
        flushSerialInput();
        //Wait for input from the user
        data.params[0] = readFloatFromSerial(); //read first number from serial port
        data.operation = readCharFromSerial(); //read operation from serial port
        data.params[1] = readFloatFromSerial(); //read second number from serial port
        printf("Core %d: received %f %c %f\n", get_core_num(), data.params[0], data.operation ,data.params[1]); //reset equal flag
        do{
            equal = true; //reset equal flag
            launchOPTasks(&data); //launch tasks on core 0 and core 1
            //wait for tasks to finish
            for(int j = 0; j < numsToCheck; j++){ 
                xQueueReceive( xQueue, result+j, portMAX_DELAY ); //this read value from queue if present, otherwise block
                printf("Core %d - Thread '%s': Queue receive %f\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()), (float)result[j]);
                }
            
            sum = result[0];
            for(int j = 1; j < numsToCheck && equal; j++){ //NOTE: have I to wait for all tasks to finish? or I can check while recieving
                equal = (sum == result[j]); //check if all results are equal
            }
            if(!equal) {
                printf("-----ERROR: RESULTS ARE NOT EQUAL\n"); //print error message
            }
        }while(!equal);

        printf("Results are equal, sum is %f\n", sum); //print result
        
    }
}

static void launchOPTasks(void* parameters){
    TaskHandle_t xTask0 = NULL; 
    TaskHandle_t xTask1 = NULL;
     //launch tasksk on core 0 and core 1
    vTaskSuspendAll(); // Suspend the scheduler, it's not beautiful, but otherwise I cannot set the mask in time
    xTaskCreate(     /* The function that implements the task. */
        operativeTask,
        /* Text name for the task, just to help debugging. */
        "OT0",
        /* The size (in words) of the stack that should be created
        for the task. */
        configMINIMAL_STACK_SIZE,
        /* A parameter that can be passed into the task */
        parameters,
        /* The priority to assign to the task.  tskIDLE_PRIORITY
        (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1
        is the highest priority. */
        mainOPERATIONS_PRIORITY,
        /* Used to obtain a handle to the created task.  Not used in
        this simple demo, so set to NULL. */
        &xTask0);
    vTaskCoreAffinitySet(xTask0, (1 << 0)); // Set task to run on core 0
    
    xTaskCreate(
        operativeTask,
        "OT1",
        configMINIMAL_STACK_SIZE,
        parameters,
        mainOPERATIONS_PRIORITY,
        &xTask1);
    vTaskCoreAffinitySet(xTask1, (1 << 1)); // Set task to run on core 1

    xTaskResumeAll(); // Resume the scheduler
}

/*-----------------------------------------------------------*/
static float sum2nums(float a, float b) {
    float ranNum = 0;
    if((float)rand()/RAND_MAX < RAND_THRESHOLD) { //simulate core error 
        ranNum = rand() % 10 + 1;
    }
    return a + b + ranNum; //return sum of two numbers + random number to simulate error
}
static float sub2nums(float a, float b) {
    float ranNum = 0;
    if((float)rand()/RAND_MAX < RAND_THRESHOLD) { //simulate core error 
        ranNum = rand() % 10 + 1;
    }
    return a - b + ranNum; //return sum of two numbers + random number to simulate error
}
static float mul2nums(float a, float b) {
    int ranNum = 0;
    if((float)rand()/RAND_MAX < RAND_THRESHOLD) { //simulate core error 
        ranNum = rand() % 10 + 1;
    }
    return a * b + ranNum; //return sum of two numbers + random number to simulate error
}
static float div2nums(float a, float b) {
    float ranNum = 0;
    if((float)rand()/RAND_MAX < RAND_THRESHOLD) { //simulate core error 
        ranNum = rand() % 10 + 1;
    }
    return a / b + ranNum; //return sum of two numbers + random number to simulate error
}

static void operativeTask(void *pvParameters){
        printf("Operative task is running on core %d\n", get_core_num());
        data_t *data  = (data_t*)pvParameters;
        float a = data->params[0]; //get first number from data structure
        float b = data->params[1]; //get second number from data structure
        char operation = data->operation; //get operation from data structure
        float sum = 0; //result of the operation
        switch (operation)
        {
        case '+':
            sum = sum2nums(a, b); //sum two numbers
            break;
        case '-':
            sum = sub2nums(a, b); //substract two numbers
            break;
        case '*':
            sum = mul2nums(a, b); //multiply two numbers
            break;
        case '/':
            if(b == 0) { //check for division by zero
                printf("Core %d - Thread '%s': Division by zero\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()));
                vTaskDelete(NULL); //terminate task, no more needed
            }else{
                sum = div2nums(a, b); //divide two numbers
            }
            break;
        default:
            printf("Core %d - Thread '%s': Unknown operation\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()));
            vTaskDelete(NULL); //terminate task, no more needed
            break;
        }

        printf("Core %d - Thread '%s': Result = %f\n", get_core_num(), pcTaskGetName(xTaskGetCurrentTaskHandle()), sum);
        xQueueSend( xQueue,  &sum, 0 ); //this send valiue to queue
        
        vTaskDelete(NULL); //terminate task, no more needed
}

/*-----------------------------------------------------------*/
//NOT USED
static void vExampleTimerCallback( TimerHandle_t xTimer )
{
    /* Argument xTimer is not used due to this callback fucntion is not reused and use one timer only. */
    ( void )xTimer;

    /* The timer has expired.  Count the number of times this happens.  The
    timer that calls this function is an auto re-load timer, so it will
    execute periodically. */
    ulCountOfTimerCallbackExecutions++;
}

//NOT USED
void vApplicationTickHook( void )
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint32_t ulCount = 0;

    /* The RTOS tick hook function is enabled by setting configUSE_TICK_HOOK to
    1 in FreeRTOSConfig.h.

    "Give" the semaphore on every 500th tick interrupt. */
    ulCount++;
    if( ulCount >= 50UL )
    {
        /* This function is called from an interrupt context (the RTOS tick
        interrupt),    so only ISR safe API functions can be used (those that end
        in "FromISR()".

        xHigherPriorityTaskWoken was initialised to pdFALSE, and will be set to
        pdTRUE by xSemaphoreGiveFromISR() if giving the semaphore unblocked a
        task that has equal or higher priority than the interrupted task.
        NOTE: A semaphore is used for example purposes.  In a real application it
        might be preferable to use a direct to task notification,
        which will be faster and use less RAM. */
        gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
        xSemaphoreGiveFromISR( xEventSemaphore, &xHigherPriorityTaskWoken );
        ulCount = 0UL;
    }

    /* If xHigherPriorityTaskWoken is pdTRUE then a context switch should
    normally be performed before leaving the interrupt (because during the
    execution of the interrupt a task of equal or higher priority than the
    running task was unblocked).  The syntax required to context switch from
    an interrupt is port dependent, so check the documentation of the port you
    are using.

    In this case, the function is running in the context of the tick interrupt,
    which will automatically check for the higher priority task to run anyway,
    so no further action is required. */
}
/*-----------------------------------------------------------*/
//NOT USED
void vApplicationMallocFailedHook( void )
{
    /* The malloc failed hook is enabled by setting
    configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

    Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    panic("malloc failed");
}
/*-----------------------------------------------------------*/
//NOT USED
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) xTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.  pxCurrentTCB can be
    inspected in the debugger if the task name passed into this function is
    corrupt. */
    for( ;; );
}
/*-----------------------------------------------------------*/
//NOT USED
void vApplicationIdleHook( void )
{
    volatile size_t xFreeStackSpace;

    /* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
    FreeRTOSConfig.h.

    This function is called on each cycle of the idle task.  In this case it
    does nothing useful, other than report the amount of FreeRTOS heap that
    remains unallocated. */
    xFreeStackSpace = xPortGetFreeHeapSize();

    if( xFreeStackSpace > 100 )
    {
        /* By now, the kernel has allocated everything it is going to, so
        if there is a lot of heap remaining unallocated then
        the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
        reduced accordingly. */
    }
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    /* Want to be able to printf */
    stdio_init_all();
    /* And flash LED */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}