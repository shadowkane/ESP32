
/**
 * 
 * This Program will send message to socket server through TCP/IP connection
 * How to use:
 *      set your router SSID
 *      set your router password
 *      set the server ip
 *      set port
 * 
 * List of libraries:
 *      FreeRTOS for the real time os. Create tasks (blink task to control led and socket task to create, connect and send message via TCP/ip socket) and use event groups technique to communicate between tasks
 *      lwip: light weight ip. to use the socket library (tools)
 *      esp_* (esp libraries) to connect to wifi
 *      driver/gpio to control the ESP32 ports
 * 
 * 
 * 
**/

#include <sdkconfig.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <lwip/sockets.h>
#include <driver/gpio.h>


/* define section */
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 1) // tskIDLE_PRIORITY = 0 so the priority of the blink task will take much more priority then IDLE by 1
#define DELAY_MS_IN_TICK(delay) (delay/portTICK_PERIOD_MS)
#define LED_PIN 25
#define LED_EVENT_MASK 0xff
#define LED_OFF (1 << 5)
#define LED_ON (1 << 0)
#define LED_BLINK_START (1 << 1) 
#define LED_BLINK_CONNECT_WIFI (1 << 2) 
#define LED_BLINK_CONNECT_SOCKET (1 << 3) 
#define LED_BLINK_SEND_MESSAGE (1 << 4) 
#define AP_SSID "your router ssid"
#define AP_PASSWORD "your router password"
#define SERVER_ADDR "server ip"
#define SERVER_PORT 1000 // change to the port you want to use

/* constant section */


/* variable section */
TaskHandle_t blinkTaskHandle;
EventGroupHandle_t blinkEvents;

/* functions/tasks declaration section*/
static esp_err_t prvEspEvenLoopHandle(void *ctx, system_event_t *event);
void prvConnectToAP();
void prvSetupHardware();
void vBinkTask();
void vTcpIpSocketTask();


/* main function */
int app_main(){
    printf("start program\n");
    prvSetupHardware();
    xTaskCreate(vBinkTask, "blink", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, &blinkTaskHandle);
    vTaskDelete(NULL);
    return 0;
}

/* function definition */
static esp_err_t prvEspEvenLoopHandle(void *ctx, system_event_t *event){
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        printf("STA started\n");
        xEventGroupClearBits(blinkEvents, LED_EVENT_MASK);
        xEventGroupSetBits(blinkEvents, LED_BLINK_CONNECT_WIFI);
        ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "esp socket client"));
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        printf("STA connected\n");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        printf("SAT IP %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xTaskCreate(vTcpIpSocketTask, "socket", 2048., NULL, BLINK_TASK_PRIORITY, NULL);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void prvConnectToAP(){
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(prvEspEvenLoopHandle, NULL));
    wifi_init_config_t defaultWifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&defaultWifiInitConfig));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t wifiConfig = {
        .sta = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void prvSetupHardware(){
    printf("init and erase flash\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nvs_flash_erase());
    printf("Init environment\n");
    blinkEvents = xEventGroupCreate();
    xEventGroupSetBits(blinkEvents, LED_BLINK_START);
    printf("setup wifi station\n");
    prvConnectToAP();
    printf("setup GPIO\n");
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

void prvLedBlinkStart(){
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(DELAY_MS_IN_TICK(2000));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(DELAY_MS_IN_TICK(2000));
}

void prvLedBlinkConnectWifi(){
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(DELAY_MS_IN_TICK(1000));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(DELAY_MS_IN_TICK(1000));
}

void prvLedBlinkConnectSocket(){
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(DELAY_MS_IN_TICK(500));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(DELAY_MS_IN_TICK(500));
}

void prvLedBlinkSendMessage(){
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(DELAY_MS_IN_TICK(100));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(DELAY_MS_IN_TICK(100));
}

void vBinkTask(){
    printf("start blink task\n");
    EventBits_t blinkEventBitStatus;
     while(1){
        blinkEventBitStatus = xEventGroupGetBits(blinkEvents);
        switch (blinkEventBitStatus)
        {
        case LED_OFF:
            gpio_set_level(LED_PIN, 0);
            break;
        case LED_BLINK_START:
            prvLedBlinkStart();
            break;
        case LED_BLINK_CONNECT_WIFI:
            prvLedBlinkConnectWifi();
            break;
        case LED_BLINK_CONNECT_SOCKET:
            prvLedBlinkConnectSocket();
            break;
        case LED_BLINK_SEND_MESSAGE:
            prvLedBlinkSendMessage();
            break;
        case LED_ON:
            gpio_set_level(LED_PIN, 1);
            break;
        default:
            break;
        }
    }
    vTaskDelete(NULL);
}

void vTcpIpSocketTask(){
    printf("Start socket task\n");
    xEventGroupClearBits(blinkEvents, LED_EVENT_MASK);
    xEventGroupSetBits(blinkEvents, LED_BLINK_CONNECT_SOCKET);
    const short sConnectAttempts = 10;
    short sConnectAttemptsCntr = 0;
    const short sSendAttempts = 10;
    short sSendAttemptsCntr = 0;
    int serverSocket_fd;
    struct sockaddr_in serverSocket;
    serverSocket.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    serverSocket.sin_port = htons(SERVER_PORT);
    serverSocket.sin_family = AF_INET;
    while(1){
        printf("Try to create socket\n");
        serverSocket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(serverSocket_fd<0){
            printf("Couldn't create socket\n");
            printf("Exit socket task\n");
            vTaskDelete(NULL);
        }
        printf("Socket created\n");
        printf("Try to connect to server\n");
        if(connect(serverSocket_fd, (struct sockaddr*)&serverSocket, sizeof(struct sockaddr_in))==-1){
            printf("Failed to connect to server.\n");
            sConnectAttemptsCntr++;
            if(sConnectAttemptsCntr>sConnectAttempts){
                printf("Reach number of attemps (%i)\n", sConnectAttempts);
                goto exit_socket;
            }
            else{
                printf("Try again after 10s...\n");
                close(serverSocket_fd);
                vTaskDelay(DELAY_MS_IN_TICK(10000));
            }
        }
        else{
            break;
        }
    }
    printf("Socket connected to server\n");
    printf("Try to send message to server\n");
    xEventGroupClearBits(blinkEvents, LED_EVENT_MASK);
    xEventGroupSetBits(blinkEvents, LED_BLINK_SEND_MESSAGE);
    while(1){
        if(send(serverSocket_fd, "hello from esp32 client", 24, 0)==-1){
            printf("Failed to send message to server.\n");
            sSendAttemptsCntr++;
            if(sSendAttemptsCntr>sSendAttempts){
                printf("Reach number of attemps (%i)\n", sSendAttempts);
                goto exit_socket;
            }
            else{
                printf("Try again after 10s...\n");
                vTaskDelay(DELAY_MS_IN_TICK(10000));
            }
        }
        else{
            break;
        }
    }
    printf("Message sent to server successfully.\n");
    xEventGroupClearBits(blinkEvents, LED_EVENT_MASK);
    xEventGroupSetBits(blinkEvents, LED_ON);
    exit_socket:
    printf("Exit socket task\n");
    close(serverSocket_fd);
    vTaskDelete(NULL);
}
