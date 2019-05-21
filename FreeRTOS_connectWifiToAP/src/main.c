/********************************
 * 
 * connect this device to your AP. so you need your wifi SSID and PASSWORD
 * 
 ********************************/
// include our default configuration
#include <sdkconfig.h>
#include <stdio.h>
// include freeRTOS library
#include <freertos/FreeRTOS.h>
#include <freertos/task.h> // for tasks
// include flash storage library
#include <nvs_flash.h>
// include esp wifi library
#include <esp_wifi.h> // for wifi driver APIs
#include <esp_event_loop.h> // for event task
#include <esp_log.h> // for error check and log display

// AP configurations
#define AP_SSID "Your_SSID"
#define AP_PASSWORD "Your_password"

void wifiDisconnectionTask(void *pvParamaters){
    // disconnect from AP after a delay
    vTaskDelay(5000/portTICK_PERIOD_MS);
    printf("disconnecting from AP\n");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    vTaskDelete(NULL);
}

void displayWifiInformations(){
    // display the mac address of the STA
    uint8_t macAddress[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, macAddress));
    printf("MAC address: %02X", macAddress[0]);
    for(int i=1; i<6; i++){
        printf(":%02X", macAddress[i]);
    }
    printf("\n");

    // display the IP address of the STA
    tcpip_adapter_ip_info_t ipAddress;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipAddress));
    
    /*
    // convert the ip address to str using sprintf but it will store it as a hex value
    char str[256];
    sprintf(str, "%x", ipAddress.ip.addr);
    printf("the address ip is : %s\n", str);
    */
    
    // display the ip address field by field using bitwise operations
    printf("IP address: %u", ipAddress.ip.addr & 0x000000ff);
    for(int i=1; i<4; i++){
        u32_t mask = 0x000000ff << (i * 8);
        u32_t ipPart = ipAddress.ip.addr & mask;
        ipPart = ipPart >> (i * 8);
        printf(".%u", ipPart);
    }
    printf("\n");
    
   /*
    // display the ip addres using the tcpip_ntoa() function
    printf("IP address: %s\n", ip4addr_ntoa(&ipAddress.ip.addr));
    */
    // display the hostname of the STA
    char *hostname;
    ESP_ERROR_CHECK(tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname));
    printf("Hostname: %s\n", hostname);
    
}

static esp_err_t main_event_task_callback(void *ctx, system_event_t *event){
    printf("inside event handler\n");
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        printf("Wifi system started\n");
        // set hostname
        printf("set new hostname\n");
        ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "its me"));
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        printf("Wifi system conncted to the AP\n");        
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        printf("Wifi system get its own IP address\n");
        // display the ip address of the STA that given by the AP
        printf("IP address is %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        // we can display all information using this function but we need to call it after getting the ip address or it won't be available. (example: in STA connected event the ip address won't be available and it will display 0.0.0.0)
        displayWifiInformations();
        xTaskCreate(&wifiDisconnectionTask, "disconnection task", 1024, NULL, 1, NULL);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        printf("Wifi system disconnected from the AP\n");
        break;
    default:
        break;
    }

    return ESP_OK;
}

void wifiConnectionTask(void *pvParameters){
    printf("Start wifi connection task\n");
    // init the tcp/ip adapter
    tcpip_adapter_init();
    
    // create and init the event task loop
    ESP_ERROR_CHECK(esp_event_loop_init(main_event_task_callback, NULL));
    // create and init wifi driver task
    wifi_init_config_t defaultWifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&defaultWifiInitConfig));
    // create our Application task
    //// set the wifi mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //// configure wifi station
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    // start wifi
    ESP_ERROR_CHECK(esp_wifi_start());
    // connect wifi to AP
    ESP_ERROR_CHECK(esp_wifi_connect());
    printf("Exist wifi connection task\n");
    vTaskDelete(NULL);
}

void app_main(){
    printf("Start Wifi connection application\n");
    // init and erase flash
    printf("init and erase flash\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nvs_flash_erase());
    // launch wifi connection task
    xTaskCreate(&wifiConnectionTask, "Connect STA to AP", 2048, NULL, 1, NULL);
}
