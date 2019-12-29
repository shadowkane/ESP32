/**
 * 
 * Simple program to run different inter-task communication like semaphore and mutex etc..
 * Counting semaphore:
 *  definition: counting semahpore is like a token holder where it can accept (allow to run) n tasks. so one or more task can enter the shared resurce.
 *  exp:  5 tasks will try to access a common resource (in our case it is the print function) that has a limite of 3 tasks at most.
 *  logic:  the logic i used in the example is they are 3 tokens are available, each time a task want to enter the shared resource it will take one token and when it leaves it will 
 *          give it back, so when no more tokens left, no more tasks can enter the shared resource, that's why i initialized the init value with the max value.
 *          the opposite logic is the token handler is empty, so each time a task wants to enter the shared resource it will give a token (like presence mark) 
 *          and when it leaves it will take it back. if the given tokens reach the max, no more tasks can enter the shared resource.
 * 
 *  Mutex:
 *  definition: a mutex is a shared cretical resource system where only one task can enter that resource. it act like a token where only one task can take a token and only that task can give it back.
 *              any task will stay in hold waiting for that token to be available again.
 *  exp:  3 task will try to acces a common critical resource (print function) which can run one task at a time.
 * 
 *  Binary semaphore:
 *  definition: Binary semaphore can allow one task or at most two tasks to enter a shared resource.
 * 
 * Queue
 * 
 * exp: 3 tasks will send message to queue and other 3 task will read that message from that queue.
 *          task will broadcast a message to all receiver tasks
 *          2 tasks will send message to a specific receiver tasks
 *          task will listen (read) to any message stored in the queue
 *          2 tasks will receive (wait) for a specific message that was send for them by a task sender or will read a broadcast message.
 *      to make this works, a manager task (which is one of the receive task) will peek on the queue and manage which receive task should be resumed to peek on the received message (read and do not remove)
 *      to make a broadcast message, the broadcast task will send a message to the queue with a destination = -1 and the queue manager task will resume all tasks.
 *      to remove the received message each receive task will peek on the message and read the message will notify the queue manager about that with the semaphore technique. the queue
 *          manager will give <<number>> of semaphores to the "xBroadcastQueue_semaphore", if it's a message with one destination then will give 1 semaphore if it's a broadcast
 *          it will give semaphores as much as the number of tasks and when each taks peed on the received message will take the semaphore and when the number of semaphores in "xBroadcastQueue_semaphore"
 *          reachs 0, the queue will receive the message just to remove it from qeueu.
 *          
 **/
#include "global.h"
#include <stdio.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>


/* Define, struct and enum */
typedef struct newType_s{
    int fromId;
    int toId;
    char data_type;
    char data[50];
} newType_t;

/* Constants */

/* Variables */
TaskHandle_t xQueueReader1_tskHandler=NULL;
TaskHandle_t xQueueReader2_tskHandler=NULL;
EventGroupHandle_t xsimulatorEvent;
EventGroupHandle_t xMutexEvent;
SemaphoreHandle_t xSemaphore_3max;
SemaphoreHandle_t xMutex;
QueueHandle_t xQueue;
SemaphoreHandle_t xBroadcastQueue_semaphore;

/* Function declations */
void tskDisplaySimulator();
void vVirtualResource_3MAX(int taskId, int runTime_ms);
void vTaskUseSemaphore_1();
void vTaskUseSemaphore_2();
void vTaskUseSemaphore_3();
void vTaskUseSemaphore_4();
void vTaskUseSemaphore_5();
void vVirtualResource_1MAX(int taskId, int runTime_ms);
void vTaskUseMutex_1();
void vTaskUseMutex_2();
void vTaskUseMutex_3();
void vTaskQueueWriter_broadcast();
void vTaskQueueWriter_1();
void vTaskQueueWriter_2();
void vTaskQueueReader_all();
void vTaskQueueReader_1();
void vTaskQueueReader_2();

/* Main function */
int app_main(){
    printf("Start simulator\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nvs_flash_erase());
    xsimulatorEvent = xEventGroupCreate();
    xMutexEvent = xEventGroupCreate();
    // Semaphore counting
    xSemaphore_3max = xSemaphoreCreateCounting(3, 3);
    if(xSemaphore_3max!=NULL){
        xTaskCreate(vTaskUseSemaphore_1, "vTaskUseSemaphore_1", 1024*10, NULL, 1, NULL);
        xTaskCreate(vTaskUseSemaphore_2, "vTaskUseSemaphore_2", 1024*10, NULL, 1, NULL);
        xTaskCreate(vTaskUseSemaphore_3, "vTaskUseSemaphore_3", 1024*10, NULL, 1, NULL);
        xTaskCreate(vTaskUseSemaphore_4, "vTaskUseSemaphore_4", 1024*10, NULL, 1, NULL);
        xTaskCreate(vTaskUseSemaphore_5, "vTaskUseSemaphore_5", 1024*10, NULL, 1, NULL);
        xTaskCreate(tskDisplaySimulator, "tskDisplaySimulator", 1024*10, NULL, 1, NULL);
    }
    else{
        printf("Couldn't create semaphore\n");
    }
    
    // Mutex
    xMutex = xSemaphoreCreateMutex();
    if(xMutex!=NULL){
        xTaskCreate(vTaskUseMutex_1, "vTaskUseMutex_1", 1024*10, NULL, 1, NULL);
        vTaskDelay(10);
        xTaskCreate(vTaskUseMutex_2, "vTaskUseMutex_2", 1024*10, NULL, 1, NULL);
        vTaskDelay(10);
        xTaskCreate(vTaskUseMutex_3, "vTaskUseMutex_3", 1024*10, NULL, 1, NULL);
    }
    else {
        printf("Couldn't create mutex\n");
    }
    
    // Queue
    xQueue = xQueueCreate(5, sizeof(newType_t));
    if(xQueue!=NULL){
        xTaskCreate(vTaskQueueReader_all, "vTaskQueueReader_all", 1024*40, NULL, 1, NULL);
        xTaskCreate(vTaskQueueReader_1, "vTaskQueueReader_1", 1024*10, NULL, 1, &xQueueReader1_tskHandler);
        vTaskSuspend(xQueueReader1_tskHandler);
        xTaskCreate(vTaskQueueReader_2, "vTaskQueueReader_2", 1024*10, NULL, 1, &xQueueReader2_tskHandler);
        vTaskSuspend(xQueueReader2_tskHandler);
        xTaskCreate(vTaskQueueWriter_broadcast, "vTaskQueueWriter_broadcast", 1024*30, NULL, 1, NULL);
        xTaskCreate(vTaskQueueWriter_1, "vTaskQueueWriter_1", 1024*10, NULL, 1, NULL);
        xTaskCreate(vTaskQueueWriter_2, "vTaskQueueWriter_2", 1024*10, NULL, 1, NULL);

    }
    else{
         printf("Couldn't create queue\n");
    }
    vTaskDelete(NULL);
    return 0;
}

/* Function definition */
void tskDisplaySimulator(){
    while(1){
        EventBits_t semaphoreEvents = xEventGroupGetBits(xsimulatorEvent);
        EventBits_t mutexEvents = xEventGroupGetBits(xMutexEvent);
        char msg[200];
        sprintf(msg,\
        "\nSimulator\\Tasks ID | 1 | 2 | 3 | 4 | 5 |\n"\
        "Semaphore (%d/3)    |   |   |   |   |   |\n"\
        "Mutex              |   |   |   | X | X |\n\n",3-uxSemaphoreGetCount(xSemaphore_3max));
        if((semaphoreEvents&EVENTMASK_SEMAPHORETSK1)==EVENTIDLE_SEMAPHORETSK1){
            msg[63]=' ';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK1)==EVENTWAIT_SEMAPHORETSK1){
            msg[63]='-';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK1)==EVENTRUN_SEMAPHORETSK1){
            msg[63]='*';
        }
        if((semaphoreEvents&EVENTMASK_SEMAPHORETSK2)==EVENTIDLE_SEMAPHORETSK2){
            msg[67]=' ';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK2)==EVENTWAIT_SEMAPHORETSK2){
            msg[67]='-';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK2)==EVENTRUN_SEMAPHORETSK2){
            msg[67]='*';
        }
        if((semaphoreEvents&EVENTMASK_SEMAPHORETSK3)==EVENTIDLE_SEMAPHORETSK3){
            msg[71]=' ';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK3)==EVENTWAIT_SEMAPHORETSK3){
            msg[71]='-';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK3)==EVENTRUN_SEMAPHORETSK3){
            msg[71]='*';
        }
        if((semaphoreEvents&EVENTMASK_SEMAPHORETSK4)==EVENTIDLE_SEMAPHORETSK4){
            msg[75]=' ';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK4)==EVENTWAIT_SEMAPHORETSK4){
            msg[75]='-';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK4)==EVENTRUN_SEMAPHORETSK4){
            msg[75]='*';
        }
        if((semaphoreEvents&EVENTMASK_SEMAPHORETSK5)==EVENTIDLE_SEMAPHORETSK5){
            msg[79]=' ';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK5)==EVENTWAIT_SEMAPHORETSK5){
            msg[79]='-';
        }
        else if((semaphoreEvents&EVENTMASK_SEMAPHORETSK5)==EVENTRUN_SEMAPHORETSK5){
            msg[79]='*';
        }
        
        if((mutexEvents&EVENTMASK_MUTEXTSK1)==EVENTIDLE_MUTEXTSK1){
            msg[104]=' ';
        }
        else if((mutexEvents&EVENTMASK_MUTEXTSK1)==EVENTWAIT_MUTEXTSK1){
            msg[104]='-';
        }
        else if((mutexEvents&EVENTMASK_MUTEXTSK1)==EVENTRUN_MUTEXTSK1){
            msg[104]='*';
        }
        if((mutexEvents&EVENTMASK_MUTEXTSK2)==EVENTIDLE_MUTEXTSK2){
            msg[108]=' ';
        }
        else if((mutexEvents&EVENTMASK_MUTEXTSK2)==EVENTWAIT_MUTEXTSK2){
            msg[108]='-';
        }
        else if((mutexEvents&EVENTMASK_MUTEXTSK2)==EVENTRUN_MUTEXTSK2){
            msg[108]='*';
        }
        if((mutexEvents&EVENTMASK_MUTEXTSK3)==EVENTIDLE_MUTEXTSK3){
            msg[112]=' ';
        }
        else if((mutexEvents&EVENTMASK_MUTEXTSK3)==EVENTWAIT_MUTEXTSK3){
            msg[112]='-';
        }
        else if((mutexEvents&EVENTMASK_MUTEXTSK3)==EVENTRUN_MUTEXTSK3){
            msg[112]='*';
        }

        printf("%s",msg);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
// this print function will print out the current task id evey 10 tick for a determinant period
void vVirtualResource_3MAX(int taskId, int runTime_ms){
    int delay_tick = 10;
    int currentTime_tick = 0;
    int stopTime_tick = runTime_ms/portTICK_PERIOD_MS;
    //printf("Task %d took sem\n", taskId);
    // notify that the task with <<task id>> is currently running and using this resource
    switch (taskId)
    {
    case 1:
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK1);
        xEventGroupSetBits(xsimulatorEvent, EVENTRUN_SEMAPHORETSK1);
        break;
    case 2:
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK2);
        xEventGroupSetBits(xsimulatorEvent, EVENTRUN_SEMAPHORETSK2);
        break;
    case 3:
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK3);
        xEventGroupSetBits(xsimulatorEvent, EVENTRUN_SEMAPHORETSK3);
        break;
    case 4:
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK4);
        xEventGroupSetBits(xsimulatorEvent, EVENTRUN_SEMAPHORETSK4);
        break;
    case 5:
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK5);
        xEventGroupSetBits(xsimulatorEvent, EVENTRUN_SEMAPHORETSK5);
        break;
    default:
        break;
    }
    // start running the resource
    while(currentTime_tick<stopTime_tick){
        //printf("Task %d is currentrly using 3max print resource. only %d places left.\n", taskId,uxSemaphoreGetCount(xSemaphore_3max));
        vTaskDelay(delay_tick);
        currentTime_tick += delay_tick;
    }
}

// Virtual resource function it's a critical resource simulator where it takes the task id and the run periode
void vVirtualResource_1MAX(int taskId, int runTime_ms){
    int delay_tick = 10;
    int currentTime_tick = 0;
    int stopTime_tick = runTime_ms/portTICK_PERIOD_MS;    
    // notify that the task with <<task id>> is currently running and using this resource
    switch (taskId)
    {
    case 1:
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK1);
        xEventGroupSetBits(xMutexEvent, EVENTRUN_MUTEXTSK1);
        break;
    case 2:
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK2);
        xEventGroupSetBits(xMutexEvent, EVENTRUN_MUTEXTSK2);
        break;
    case 3:
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK3);
        xEventGroupSetBits(xMutexEvent, EVENTRUN_MUTEXTSK3);
        break;
    default:
        break;
    }
    // start running the resource
    while(currentTime_tick<stopTime_tick){
        vTaskDelay(delay_tick);
        currentTime_tick += delay_tick;
    }
}

void vTaskUseSemaphore_1(){
    while(1){
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK1);
        xEventGroupSetBits(xsimulatorEvent, EVENTWAIT_SEMAPHORETSK1);
        // wait for Resource to be available and then obtain semaphore
        if(xSemaphoreTake(xSemaphore_3max, portMAX_DELAY)==true){
            vVirtualResource_3MAX(1, 5000);
            // release semaphore
            if(xSemaphoreGive(xSemaphore_3max)!=true){
                printf("Something wrong in giving semaphore in task id: 1\n");
            }
         }
        else{
            printf("Something wrong in taking semaphore in task id: 1\n");
        }
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK1);
        xEventGroupSetBits(xsimulatorEvent, EVENTIDLE_SEMAPHORETSK1);
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseSemaphore_2(){
    while(1){
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK2);
        xEventGroupSetBits(xsimulatorEvent, EVENTWAIT_SEMAPHORETSK2);
        // wait for Resource to be available and then obtain semaphore
        if(xSemaphoreTake(xSemaphore_3max, portMAX_DELAY)==true){
            vVirtualResource_3MAX(2, 2000);
            // release semaphore
            if(xSemaphoreGive(xSemaphore_3max)!=true){
                printf("Something wrong in giving semaphore in task id: 2\n");
            }
        }
        else{
            printf("Something wrong in taking semaphore in task id: 2\n");
        }
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK2);
        xEventGroupSetBits(xsimulatorEvent, EVENTIDLE_SEMAPHORETSK2);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseSemaphore_3(){
    while(1){
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK3);
        xEventGroupSetBits(xsimulatorEvent, EVENTWAIT_SEMAPHORETSK3);
        // wait for Resource to be available and then obtain semaphore
        if(xSemaphoreTake(xSemaphore_3max, portMAX_DELAY)==true){
            vVirtualResource_3MAX(3, 6000);
            // release semaphore
            if(xSemaphoreGive(xSemaphore_3max)!=true){
                printf("Something wrong in giving semaphore in task id: 3\n");
            }
        }
        else{
            printf("Something wrong in taking semaphore in task id: 3\n");
        }
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK3);
        xEventGroupSetBits(xsimulatorEvent, EVENTIDLE_SEMAPHORETSK3);
        vTaskDelay(6000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseSemaphore_4(){
    while(1){
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK4);
        xEventGroupSetBits(xsimulatorEvent, EVENTWAIT_SEMAPHORETSK4);
        // wait for Resource to be available and then obtain semaphore
        if(xSemaphoreTake(xSemaphore_3max, portMAX_DELAY)==true){
            vVirtualResource_3MAX(4, 10000);
            // release semaphore
            if(xSemaphoreGive(xSemaphore_3max)!=true){
                printf("Something wrong in giving semaphore in task id: 4\n");
            }
        }
        else{
            printf("Something wrong in taking semaphore in task id: 4\n");
        }
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK4);
        xEventGroupSetBits(xsimulatorEvent, EVENTIDLE_SEMAPHORETSK4);
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseSemaphore_5(){
    while(1){
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK5);
        xEventGroupSetBits(xsimulatorEvent, EVENTWAIT_SEMAPHORETSK5);
        // wait for Resource to be available and then obtain semaphore
        if(xSemaphoreTake(xSemaphore_3max, portMAX_DELAY)==true){
            vVirtualResource_3MAX(5, 20000);
            // release semaphore
            if(xSemaphoreGive(xSemaphore_3max)!=true){
                printf("Something wrong in giving semaphore in task id: 5\n");
            }
        }
        else{
            printf("Something wrong in taking semaphore in task id: 5\n");
        }
        xEventGroupClearBits(xsimulatorEvent, EVENTMASK_SEMAPHORETSK5);
        xEventGroupSetBits(xsimulatorEvent, EVENTIDLE_SEMAPHORETSK5);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseMutex_1(){
    while(1){
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK1);
        xEventGroupSetBits(xMutexEvent, EVENTWAIT_MUTEXTSK1);
        // wait for Resource to be available (which mean wait until the mutex get the token back, become unblocked) then block mutex by taking the token
        if(xSemaphoreTake(xMutex, portMAX_DELAY)==true){
            vVirtualResource_1MAX(1, 10000);
            // gives back the token unblick resource
            if(xSemaphoreGive(xMutex)!=true){
                printf("Something wrong in giving mutex's token in task id: 1\n");
            }
        }
        else{
            printf("Something wrong in taking mutex's token in task id: 1\n");
        }
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK1);
        xEventGroupSetBits(xMutexEvent, EVENTIDLE_MUTEXTSK1);
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseMutex_2(){
    while(1){
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK2);
        xEventGroupSetBits(xMutexEvent, EVENTWAIT_MUTEXTSK2);
        // wait for Resource to be available (which mean wait until the mutex get the token back, become unblocked) then block mutex by taking the token
        if(xSemaphoreTake(xMutex, portMAX_DELAY)==true){
            vVirtualResource_1MAX(2, 5000);
            // gives back the token unblick resource
            if(xSemaphoreGive(xMutex)!=true){
                printf("Something wrong in giving mutex's token in task id: 2\n");
            }
        }
        else{
            printf("Something wrong in taking mutex's token in task id: 2\n");
        }
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK2);
        xEventGroupSetBits(xMutexEvent, EVENTIDLE_MUTEXTSK2);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskUseMutex_3(){
    while(1){
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK3);
        xEventGroupSetBits(xMutexEvent, EVENTWAIT_MUTEXTSK3);
        // wait for Resource to be available (which mean wait until the mutex get the token back, become unblocked) then block mutex by taking the token
        if(xSemaphoreTake(xMutex, portMAX_DELAY)==true){
            vVirtualResource_1MAX(3, 20000);
            // gives back the token unblick resource
            if(xSemaphoreGive(xMutex)!=true){
                printf("Something wrong in giving mutex's token in task id: 3\n");
            }
        }
        else{
            printf("Something wrong in taking mutex's token in task id: 3\n");
        }
        xEventGroupClearBits(xMutexEvent, EVENTMASK_MUTEXTSK3);
        xEventGroupSetBits(xMutexEvent, EVENTIDLE_MUTEXTSK3);
        vTaskDelay(6000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskQueueWriter_broadcast(){
    srand(1000);
    newType_t msg;
    msg.fromId = -1;
    msg.toId = -1;
    while(1){
        msg.data_type = 's';
        sprintf(msg.data, "broadcast message");
        if(xQueueSend(xQueue, (void *)&msg, 10)==true){
            printf("writer broadcast send message to queue\n");
        }
        else{
            printf("writer broadcast couldn't send message to queue\n");
        }
        vTaskDelay(20000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskQueueWriter_1(){
    srand(1000);
    newType_t msg;
    msg.fromId = -1;
    while(1){
        // get random value between 0 and number of tasks-1
        int chosenTask = rand() % 5; // 5 is number of tasks
        msg.toId = chosenTask;
        msg.data_type = 's';
        sprintf(msg.data, "this is a message from writer 1 to %d", chosenTask);
        if(xQueueSend(xQueue, (void *)&msg, 10)==true){
            printf("writer 1 send message to queue\n");
        }
        else{
            printf("writer 1 couldn't send message to queue\n");
        }
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskQueueWriter_2(){
    srand(1000);
    newType_t msg;
    msg.fromId = 2;
    while(1){
        // get random value between 0 and number of tasks-1
        int chosenTask = rand() % 5; // 5 is number of tasks
        msg.toId = chosenTask;
        msg.data_type = 's';
        sprintf(msg.data, "this is a message from writer 2 to %d", chosenTask);
        if(xQueueSend(xQueue, (void *)&msg, 10)==true){
            printf("writer 2 send message to queue\n");
        }
        else{
            printf("writer 2 couldn't send message to queue\n");
        }
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void vTaskQueueReader_all(){
    xBroadcastQueue_semaphore = xSemaphoreCreateCounting(2, 0);
    if(xBroadcastQueue_semaphore == NULL){
        printf("couldn't create xBroadcastQueue_semaphore\n");
    }
    newType_t msg;
    while(1){
        if(xQueuePeek(xQueue, &msg,portMAX_DELAY)==true){
            printf("reader all: received msg type:'%c' data=>'%s' from:'%d' to '%d'\n", msg.data_type, msg.data, msg.fromId, msg.toId);
            // resume the chosen task
            switch (msg.toId)
            {
            case -1:
                if(xSemaphoreGive(xBroadcastQueue_semaphore)==true){
                    if(xSemaphoreGive(xBroadcastQueue_semaphore)==false){
                        printf("second give failed\n");
                    }
                }
                else{
                    printf("first give failed\n");
                }
                vTaskResume(xQueueReader1_tskHandler);
                vTaskResume(xQueueReader2_tskHandler);
                break;
            case 1:
                if(xSemaphoreGive(xBroadcastQueue_semaphore)==false){
                        printf("give failed\n");
                    }
                vTaskResume(xQueueReader1_tskHandler);
                break;
            case 2:
                if(xSemaphoreGive(xBroadcastQueue_semaphore)==false){
                        printf("give failed\n");
                    }
                vTaskResume(xQueueReader2_tskHandler);
                break;
            default:
                break;
            }
            // wait for receive task read queue message
            while(uxSemaphoreGetCount(xBroadcastQueue_semaphore) != 0);
            if(xQueueReceive(xQueue, &msg,portMAX_DELAY)==false){
                 printf("reader all: qeueu message failed to be received by task(s)\n");
            }
        }
        else{
            printf("reader all: nothing received from queue\n");
        }
    }
    vTaskDelete(NULL);
}

void vTaskQueueReader_1(){
    newType_t msg;
    while(1){
        if(xQueuePeek(xQueue, &msg,portMAX_DELAY)==true){
            if(xSemaphoreTake(xBroadcastQueue_semaphore, portMAX_DELAY)==false){
                printf("take failed");
            }
            if(msg.toId == 1){
                printf("reader 1: received msg type:'%c' data=>'%s' from:'%d'\n", msg.data_type, msg.data, msg.fromId);
            }
            else if(msg.toId == -1){
                printf("reader 1: received broadcast msg type:'%c' data=>'%s'\n", msg.data_type, msg.data);
            }
            else{
                printf("reader 1: not for me\n");
            }
        }
        else{
            printf("reader 1: nothing received from queue\n");
        }
        // after reading the queue suspend this task
        vTaskSuspend(NULL);
    }
    vTaskDelete(NULL);
}

void vTaskQueueReader_2(){
    newType_t msg;
    while(1){
        if(xQueuePeek(xQueue, &msg,portMAX_DELAY)==true){
            if(xSemaphoreTake(xBroadcastQueue_semaphore, portMAX_DELAY)==false){
                printf("take failed");
            }
            if(msg.toId == 1){
                printf("reader 2: received msg type:'%c' data=>'%s' from:'%d'\n", msg.data_type, msg.data, msg.fromId);
            }
            else if(msg.toId == -1){
                printf("reader 2: received broadcast msg type:'%c' data=>'%s'\n", msg.data_type, msg.data);
            }
            else{
                printf("reader 2: not for me\n");
            }
        }
        else{
            printf("reader 2: nothing received from queue\n");
        }
        // after reading the queue suspend this task
        vTaskSuspend(NULL);
    }
    vTaskDelete(NULL);
}
