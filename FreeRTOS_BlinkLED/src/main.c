/*******************************************
 * 
 * use freeRTOS to create 2 tasks
 * task to send message via serial monitor evey second
 * task to blink led every 200 milli second
 * 
*******************************************/
#include <stdio.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_system.h>
#include <driver/gpio.h>

#define LED_PIN 25

void BlinLED_Task(void *pvParameters){

    const TickType_t nbrTickDelay = 200 / portTICK_PERIOD_MS;
    
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while(1){
        //printf("turing led ON");
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(nbrTickDelay);
        //printf("turning led OFF");
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(nbrTickDelay);
    }
}

void PrintSomething(void *pvParamaters){
    // nbrTickDelay is the nember of ticks we need to pass to the vTaskDelay to stop the thread for a desire periode which is the delay periode in ms devided by the periode of one tick in ms.
    const TickType_t nbrTickDelay = 1000 / portTICK_PERIOD_MS;
    while(1){
        printf("String to print\n");
        vTaskDelay(nbrTickDelay);
    }
}

void app_main(){
    xTaskCreate(&BlinLED_Task, "Blink Led Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(&PrintSomething, "Print Something Task", 2048, NULL, 1, NULL);
}
