/***************************************
 * 
 * Run this device as AP
 * General Scenaro:
 *  1: Create and init all blocks (Event task, WlIP task, WiFi task), (the application task is the task where you init and create the previous tasks it may be app_main or other taks you create)
 *  2: Configure the WiFi task (for AP purpose use the AP variable in the wifi_config_t type to setup the configureation you want like the SSID and PASSWORD and channel and number max of connections)
 *  3: start Wifi driver using esp_wifi_start function, this will trigger the WIFI_EVENT_AP_START event
 *  4: each time a station connected to this AP, an event will be triggered "wifi_event_ap_staconnected"
 *  5: if a station disconnected from this AP, an event call "wifi_event_ap_stadisconnected" will be triggered
 *  6: to stop wifi use esp_wifi_disconnect to disconnect the wifi connectivity then stop the wifi driver using esp_wifi_stop and then undload the wifi driver using esp_wifi_deinit)
 * 
 ***************************************/

#include <stdio.h>
#include <string.h>
// freeRTOS libraries
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// esp libraries
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>

esp_err_t eventHandler(void *ctx, system_event_t *event){
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_START:
        // run this section when the wifi driver started as AP
        printf("System AP started.\n");
        // get the AP IP@ 
        printf("AP IP@: %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        // get the AP MAC@
        uint8_t macAddr[6];
        ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_AP, macAddr));
        printf("AP MAC@: %02X",  macAddr[0]);
        for(int i=1; i<6; i++){
            printf(":%02X", macAddr[i]);
        }
        printf("\n");
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        // run this section when a new STA connected to this AP.
        // this event will have the IP address of the new client
        printf("STA just connected.\n");
        wifi_sta_list_t nbcClient;
        ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&nbcClient));
        printf("Total number of connected STA: %d\n", nbcClient.num);
        printf("STA IP@: %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    default:
        break;
    }
    return ESP_OK;
} 

void AP_task(void* pvParamaters){
    // init TCPIP task
    tcpip_adapter_init();
    // create and init event task handler
    ESP_ERROR_CHECK(esp_event_loop_init(&eventHandler, NULL));
    // create and init wifi driver task
    wifi_init_config_t defaultWifiConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&defaultWifiConfig));
    // set wifi mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    // configure AP
    wifi_config_t AP_config = {
        .ap = {
            .ssid = "myAP",
            .ssid_len = strlen("myAP"),
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .password = "00000000",
            .channel = 1,
            .max_connection = 1,
            .ssid_hidden = 0,
            .beacon_interval = 100
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &AP_config));
    // start wifi
    ESP_ERROR_CHECK(esp_wifi_start());
    while(1){
        vTaskDelay(1000/portTICK_PERIOD_MS);
    };
}

void app_main(){
    printf("Start AP Program.\n");
    // init flash memory and erase it
    printf("Init and erase flash.\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nvs_flash_erase());
    // start AP task
    xTaskCreate(&AP_task, "AP task", 8000, NULL, 1, NULL);
}