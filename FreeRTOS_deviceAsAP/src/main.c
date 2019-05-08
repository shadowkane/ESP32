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

// this function will get 4 paramaters of type unsigned integer 32 bits, representing each octet of the IP address and return it as one value of type 32 bits
// an IP address is divided in 4 words (octets) ex: 192.168.1.1, where the fourth octet (192) is actualy the first octet in the network byte order which is used by all TCP/IP protocols
// it is the same thing as big endian or the MSB first. 
u32_t makeU32_Address(u32_t firstSet, u32_t secondSet, u32_t thirdSet, u32_t fourthSet){
    u32_t address = firstSet;
    address += secondSet << 8;
    address += thirdSet << 16;
    address += fourthSet << 24;
    return address;
}

esp_err_t eventHandler(void *ctx, system_event_t *event){
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_START:
        // run this section when the wifi driver started as AP
        printf("System AP started.\n");
        // set a local IP@ along with a gatway @ and subnet mask
        // the addr variable is of type u32_t    
        tcpip_adapter_ip_info_t defaultIPConfig = {
            .ip.addr = makeU32_Address(192,168,1,1),
            .gw.addr = makeU32_Address(192,168,1,1),
            .netmask.addr = makeU32_Address(255,255,255,0)
        };
        // before set the TCPIP configuration, stop the DHCP server
        ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
        // set the tcpip configuration
        esp_err_t ret = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &defaultIPConfig);
        ESP_ERROR_CHECK(ret);
        // start the DHCP if the ip info setup was right
        if(ret == ESP_OK){
            // the lease time option is to give the client the option to allocate a place in the server for a periode of time.
            // but in our context we are using lease to allocate IP addresses so the server assign them to incoming clients
            dhcps_lease_t leaseConfig = {
                .enable = true,
                .start_ip.addr = makeU32_Address(192,168,1,2),
                .end_ip.addr = makeU32_Address(192,168,1,11)
            };
            // tcpip_adapter_dhcps_option this function to set or get the DHCP server option
            // using TCPIP_ADAPTER_OP_SET option to set an option to the DHCP server
            // TCPIP_ADAPTER_REQUESTED_IP_ADDRESS to set IP address
            ESP_ERROR_CHECK(tcpip_adapter_dhcps_option((tcpip_adapter_option_mode_t)TCPIP_ADAPTER_OP_SET, (tcpip_adapter_option_id_t)TCPIP_ADAPTER_REQUESTED_IP_ADDRESS,(void*) &leaseConfig, sizeof(dhcps_lease_t)));
            ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
        }
        tcpip_adapter_ip_info_t IPInfo;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &IPInfo);
        printf("AP IP from ip info@: %s\n", ip4addr_ntoa(&IPInfo.ip.addr));

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
    xTaskCreate(&AP_task, "AP task", 10000, NULL, 1, NULL);
}