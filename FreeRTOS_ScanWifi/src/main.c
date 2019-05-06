/*********************
 * 
 * scan wifi and list all available networks in the area
 *
 * ESP32 WiFi module programming depicated in 4 blocks:
 * the TCP/IP stack that sends events to the event task
 * event task which is a daemon task (a program run in the background) that receives events from TCP/IP stack and/or WiFi Driver and call callback functions of the Application task, example: it receives SYSTEM_STA_CONNECTED event and it will call the tcpip_adapter_start() function to start the DHCP client
 * Application task run/handle evens called by event task and calls APIs to init the system/WiFi (from the WiFi Driver)
 * WiFi driver is a black box that receives API call from application task and post events queue to a specified Queue, wich is iniialized by API esp_wifi_init();
 * for more information, esp_wifi.h library
 * for similar project  https://github.com/espressif/esp-idf/blob/master/examples/wifi/scan/main/scan.c
*********************/

#include <sdkconfig.h>
#include <stdio.h> // to call the printf function
// FreeRTOS library
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// nvs non volatile storage flash function
#include <nvs_flash.h>
// esp library that check if the state of the function's return if it's success or some sort of error with specifying the error type
#include <esp_log.h>
// esp library for wifi functions or APIs
#include <esp_wifi.h>
// #include <esp_event.h>
#include <esp_event_loop.h>

// function that display all APs information
void displayAPInfo(uint16_t ap_nbr, wifi_ap_record_t *ap_record){
    for(int i=0; i<ap_nbr; i++){
        char *auth_mode = "Unknow";
        switch (ap_record[i].authmode)
        {
        case WIFI_AUTH_OPEN:
            auth_mode = "No password";
            break;
        case WIFI_AUTH_WEP:
            auth_mode = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            auth_mode = "WPA PSK";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            auth_mode = "WPA2 Enterprise";
            break;
        case WIFI_AUTH_WPA2_PSK:
            auth_mode = "WPA2 PSK";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            auth_mode = "WPA WPA2 PSK";
            break;
        default:
            break;
        }
        printf("AP SSID is %s (signal strength: %d) and authentification mode is %s\n", (char*)ap_record[i].ssid, ap_record[i].rssi, auth_mode);
    }
}

// event handler that will be called each time an event has been detected (launched)
//static void wifi_event_handler(void *even_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
// the event handler or the callback function for the event handler task when wifi event triggered
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event){
    switch (event->event_id)
    {
        // the WIFI_EVENT_SCAN_DONE event id is SYSTEM_EVENT_SCAN_DONE
        case SYSTEM_EVENT_SCAN_DONE:
        {
            printf("Scan network for APs done\n");
            // get scan results
            printf("Get scan results\n");
            uint16_t nbrOfAPs = 10;
            wifi_ap_record_t listAPs[nbrOfAPs];
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&nbrOfAPs, listAPs));
            printf("message from event said, the number of APs is %u\n", nbrOfAPs);
            displayAPInfo(nbrOfAPs, listAPs);
            break;
        }
        default:
            break;
    }
    return ESP_OK;
} 

void scanWifiTask(void *pvParameters){
    printf("Init flash\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    //printf("Clear flash");
    ESP_ERROR_CHECK(nvs_flash_erase());
    printf("Start scan task\n");
    printf("Start init wifi\n");
    // init the TCP/IP adapter (block)
    tcpip_adapter_init();
    /*
    // create a default event loop for the event task block
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // init the event task to handler a WIFI_EVENT
    ESP_ERROR_CHECK(esp_event_handler_register(SYSTEM_EVENT_SCAN_DONE, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    */
    // instand of using the loop create default function then the event handler register we can create and init the event using event loop init form esp_event_loop library
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL))
    // init the WiFi driver by alloc resources for it like wifi control structure and RX/TX buffer and wifi NVS struction and then start that wifi task by passing the right configuration which is more safer to pass the WIFI_INIT_CONFIG_DEFAULT macro as default configuration.
    wifi_init_config_t defaultWifiConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&defaultWifiConfig));
    // set mode to SAT (station mode) (the difference between STA and AP is that the STA is the client whom connect to the AP, and AP is the Access Point)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // start wifi, wifi need to be initialized first
    printf("Start wifi\n");
    ESP_ERROR_CHECK(esp_wifi_start());
    // start scan for APs, wifi need to be initialized and started
    printf("Start scanning\n");
    wifi_scan_config_t scanWifiConfig;
    scanWifiConfig.ssid = 0;
    scanWifiConfig.bssid = 0;
    scanWifiConfig.channel = 0;
    scanWifiConfig.show_hidden = 0;
    scanWifiConfig.scan_type = WIFI_SCAN_TYPE_ACTIVE; // device wifi will send a prop request and listens for a prop response from an AP. ( passive method means the device wifi radio will just listen for a beacon send periodically from an AP)
    scanWifiConfig.scan_time.active.min = 100; // the minimum time in ms to scan one channel
    scanWifiConfig.scan_time.active.max = 1000; // above (more than) 1500 ms may cause the device (station) to disconnect from the AP that's why it's not recommanded
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scanWifiConfig, false));
    // stop task
    vTaskDelete(NULL);
}

// the app_main is the default function that the RTOS calls to run as the first task. like the main function
void app_main(){
    printf("Start app main task\n");
    xTaskCreate(&scanWifiTask, "Scan wifi task", 2048, NULL, 1, NULL);
}