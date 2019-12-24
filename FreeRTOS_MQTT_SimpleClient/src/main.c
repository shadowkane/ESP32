#include <sdkconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <aws_iot_config.h>
#include <aws_iot_error.h>
#include <aws_iot_log.h>
#include <aws_iot_mqtt_client_common_internal.h>
#include <aws_iot_error.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
//#include <lwip/sockets.h>
#include <driver/gpio.h>

/*
How to generate certificate:
1- create the CA root (root certificate) in x509 type with an extension .pem or .crt (which is the same) because aws iot library read PEM certificates.
2- generate private key for the broker
3- generate a certificate signing request for that broker private key (this certificate will store the public key for that private key and other information about the broker)
4- sign the broker certicate with ther CA root certificate and key. (this is why it's CA root, because it's the end of the certificate chain)
5- repeat the same process (2 to 4) for the mqtt client (iot device)

cmd using openssl:
- CA root
    openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout CAroot.key -out CAroot.crt
- server/broker 
    openssl genrsa -out mqttBroker.key 2048
    openssl req -new -key mqttBroker.key -out mqttBroker.csr
    openssl x509 -req -in mqttBroker.csr -CA CAroot.crt -CAkey CAroot.key -CAcreateserial -out mqttBrokerCert.crt -days 365 -sha256
- client
    openssl genrsa -out mqttClient.key 2048
    openssl req -new -key mqttClient.key -out mqttClient.csr
    openssl x509 -req -in mqttClient.csr -CA CAroot.crt -CAkey CAroot.key -CAcreateserial -out mqttClientCert.crt -days 365 -sha256
 Note: make sure to use the same common name and use the MQTT broker ip address as the common name otherwise the client side will faile to connect to broker if the option to verify the host name is enbaled.
*/

/* Certi and key */
const char *CArootCert = "-----BEGIN CERTIFICATE-----\n\
MIIDrjCCApagAwIBAgIJALl+t9jC0dNEMA0GCSqGSIb3DQEBCwUAMGwxCzAJBgNV\n\
.\
.\
.\
S6e+KOS4CEys1mkA/l98xGVh78raQyNTbfgQNBx/Wn1ak0sfIIzkLHVDflTpVQ4A\n\
xTgBYbEPWOuqlmgV9tslrsoMG0WQ/CwORbjsTFo2UwbrOg==\n\
-----END CERTIFICATE-----\n";

const char *mqttClientCert = "-----BEGIN CERTIFICATE-----\n\
MIIDVDCCAjwCCQC4P76sKOX0CjANBgkqhkiG9w0BAQsFADBsMQswCQYDVQQGEwJ0\n\
.\
.\
.\
oN2nN7FtjOD3XiwPfl49u78UmkJybP5Hr0aM4WbsJNscA+6IJCI7bUlV6fcI6IKw\n\
+WeBqobnNVhqaP8EqYEeJ8QAQZ/rpqXJBt2tmm1BxKb2VBnrKQm4Lw==\n\
-----END CERTIFICATE-----\n";

const char *mqttClientKey = "-----BEGIN RSA PRIVATE KEY-----\n\
MIIEpAIBAAKCAQEA4Z92iD6buQlbdJlEwe1yv1NlhoagbEEY2xUuq1b85UjngB+l\n\
.\
.\
.\
0x6fO9lE4D358qqnYsSVEq6W0KPDFwHyiI5yA+asK3UvCll8Y3urw9rFiLnJMy+Z\n\
GB6hke4d5EJbPnVQPohpsM3cAnfRHambAbtLBQkKh8PX4ydaWiXNGA==\n\
-----END RSA PRIVATE KEY-----\n";

/* define section */
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 1) // tskIDLE_PRIORITY = 0 so the priority of the blink task will take much more priority then IDLE by 1
#define DELAY_MS_IN_TICK(delay) (delay/portTICK_PERIOD_MS)
#define LED_PIN 25
#define EVENTS_MASK 0xff
#define EVENT_SYSTEM_ERROR (1 << 5)
#define EVENT_SYSTEM_START (1 << 0)
#define EVENT_SYSTEM_CONNECTING_TO_WIFI (1 << 1) 
#define EVENT_SYSTEM_CONNECTING_TO_MQTT (1 << 2) 
#define EVENT_SYSTEM_PUB_MSG (1 << 3)
#define EVENT_SYSTEM_WAIT_MSG (1 << 4) 
#define AP_SSID "<you ssid>"
#define AP_PASSWORD "<your ssid password>"

#define ENABLE_IOT_DEBUG
#define ENABLE_IOT_TRACE
#define ENABLE_IOT_INFO
/* constant section */


/* variable section */
TaskHandle_t blinkTaskHandle;
EventGroupHandle_t systemEvents;
IoT_Error_t err;
AWS_IoT_Client myMQTTClient;

/* functions/tasks declaration section*/
static esp_err_t prvEspEvenLoopHandle(void *ctx, system_event_t *event);
void prvConnectToAP();
void prvSetupHardware();
void prvToggleLed(int delayMs);
void vBinkTask();
void vMQTTTask();

void mqttDisconnectionHandler(AWS_IoT_Client *mqttClient, void *data);

/* main function */
int app_main(){
    printf("start program\n");
    prvSetupHardware();
    xTaskCreate(vBinkTask, "blink", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, &blinkTaskHandle);
    xTaskCreate(vMQTTTask, "mqtt", 1024*36, NULL, BLINK_TASK_PRIORITY, NULL);
    vTaskDelete(NULL);
    return 0;
}

/* function definition */
// GPIO functions
void prvToggleLed(int delayMs){
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(DELAY_MS_IN_TICK(delayMs));
    gpio_set_level(LED_PIN, 0);
    vTaskDelay(DELAY_MS_IN_TICK(delayMs));
}

// WiFi functions/callbacks
static esp_err_t prvEspEvenLoopHandle(void *ctx, system_event_t *event){
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        printf("STA started\n");
        xEventGroupClearBits(systemEvents, EVENTS_MASK);
        xEventGroupSetBits(systemEvents, EVENT_SYSTEM_CONNECTING_TO_WIFI);
        ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "esp socket client"));
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        printf("STA connected\n");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        printf("SAT IP %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        // the system now should start initiating the connection with the MQTT broker
        xEventGroupClearBits(systemEvents, EVENTS_MASK);
        xEventGroupSetBits(systemEvents, EVENT_SYSTEM_CONNECTING_TO_MQTT);
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

// MQTT functions/callbacks
void mqttDisconnectionHandler(AWS_IoT_Client *mqttClient, void *data){
    printf("MQTT Client disconnected from broker\n");
    if(!mqttClient){
        return;
    }
    if(aws_iot_is_autoreconnect_enabled(mqttClient)) {
        printf("Reconnecting to broker will start now");
    } 
}

void mqttSubCallback(AWS_IoT_Client *mqttClient, char* topic, uint16_t topicLen, IoT_Publish_Message_Params *msgParams, void *data){
    printf("Message received from MQTT broker\n");
    printf("topic: %.*s, msg: %.*s\n", topicLen, topic, (int)msgParams->payloadLen, (char*)msgParams->payload);
}

// Init function
void prvSetupHardware(){
    // Init and erase flash
    printf("Init and erase flash\n");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nvs_flash_erase());
    printf("Init environment\n");
    // Create Event group
    systemEvents = xEventGroupCreate();
    // Init MQTT
    IoT_Client_Init_Params MQTTClientParams = iotClientInitParamsDefault; //IoT_Client_Init_Params_initializer;
    MQTTClientParams.pHostURL = CONFIG_AWS_IOT_MQTT_HOST;
    MQTTClientParams.port = CONFIG_AWS_IOT_MQTT_PORT;
    MQTTClientParams.enableAutoReconnect = true;
    MQTTClientParams.tlsHandshakeTimeout_ms = 5000;
    MQTTClientParams.mqttCommandTimeout_ms = 20000;
    MQTTClientParams.isSSLHostnameVerify = true;
    MQTTClientParams.disconnectHandlerData = NULL;
    MQTTClientParams.pRootCALocation = (const char*)CArootCert; 
    MQTTClientParams.pDeviceCertLocation = (const char*)mqttClientCert; 
    MQTTClientParams.pDevicePrivateKeyLocation = (const char*)mqttClientKey;
    MQTTClientParams.disconnectHandler = mqttDisconnectionHandler;
    err = aws_iot_mqtt_init(&myMQTTClient, &MQTTClientParams);
    if(err!=SUCCESS){
        IOT_ERROR("Init MQTT failed: Error(%d)n\n", err);
        xEventGroupClearBits(systemEvents, EVENTS_MASK);
        xEventGroupSetBits(systemEvents, EVENT_SYSTEM_ERROR);
        return;
    }
    xEventGroupClearBits(systemEvents, EVENTS_MASK);
    xEventGroupSetBits(systemEvents, EVENT_SYSTEM_START);
    printf("Setup wifi station\n");
    prvConnectToAP();
    printf("Setup GPIO\n");
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}


// Blink task
void vBinkTask(){
    printf("start blink task\n");
    EventBits_t blinkEventBitStatus;
     while(1){
        blinkEventBitStatus = xEventGroupGetBits(systemEvents);
        switch (blinkEventBitStatus)
        {
        case EVENT_SYSTEM_ERROR:
            gpio_set_level(LED_PIN, 0);
            break;
        case EVENT_SYSTEM_START:
            prvToggleLed(1000);
            break;
        case EVENT_SYSTEM_CONNECTING_TO_WIFI:
            prvToggleLed(500);
            break;
        case EVENT_SYSTEM_CONNECTING_TO_MQTT:
            prvToggleLed(100);
            break;
        case EVENT_SYSTEM_PUB_MSG:
            gpio_set_level(LED_PIN, 1);
            
            break;
        case EVENT_SYSTEM_WAIT_MSG:
            prvToggleLed(2000);
            break;
        default:
            break;
        }
    }
    vTaskDelete(NULL);
}

// MQTT task
void vMQTTTask(){
    IoT_Client_Connect_Params mqttClientConnectParams = iotClientConnectParamsDefault; //IoT_Client_Connect_Params_initializer;
    mqttClientConnectParams.keepAliveIntervalInSec = 10;
    mqttClientConnectParams.isCleanSession = true;
    mqttClientConnectParams.MQTTVersion = MQTT_3_1_1;
    mqttClientConnectParams.isWillMsgPresent = false;
    mqttClientConnectParams.pClientID="esp32 client";
    mqttClientConnectParams.clientIDLen = 12;
    IoT_Publish_Message_Params PubMsgParams;
    PubMsgParams.qos = QOS0;
    PubMsgParams.isRetained = true;
    char msg[50];
    int temp=0;
    while(1){
        EventBits_t eventStatus = xEventGroupWaitBits(systemEvents, EVENT_SYSTEM_CONNECTING_TO_MQTT, pdFALSE, pdFALSE, portMAX_DELAY);
        if(eventStatus == EVENT_SYSTEM_CONNECTING_TO_MQTT){
            printf("Start MQTT task\n");
            printf("Estable connection with MQTT broker\n");
            err = aws_iot_mqtt_connect(&myMQTTClient, &mqttClientConnectParams);
            if(err!=SUCCESS){
                IOT_ERROR("Connecting to MQTT broker failed: Error(%d) connecting to %s:%d\n", err, CONFIG_AWS_IOT_MQTT_HOST, CONFIG_AWS_IOT_MQTT_PORT);
                xEventGroupClearBits(systemEvents, EVENTS_MASK);
                xEventGroupSetBits(systemEvents, EVENT_SYSTEM_ERROR);
                vTaskDelete(NULL);
                return;
            }
            printf("Successfully connected to the broker\n"); 
            printf("Subscribe to topic: press\n");
            err = aws_iot_mqtt_subscribe(&myMQTTClient, "press", 5, QOS0, mqttSubCallback, NULL);
            if(err!=SUCCESS){
                printf("couldn't subscribe to this topic: press.\n");
                printf("    Error(%d)\n", err);
            }
            while(1){
                printf("publish message to broker\n");
                sprintf(msg, "temp: %d", temp++);
                PubMsgParams.payload = msg;
                PubMsgParams.payloadLen = strlen(msg);
                err = aws_iot_mqtt_publish(&myMQTTClient, "temp", strlen("temp"), &PubMsgParams);
                if(err!=SUCCESS){
                    printf("couldn't publish msg to this topic: temp.\n");
                    printf("    Error(%d)\n", err);
                }
                err = aws_iot_mqtt_yield(&myMQTTClient, 1000);
                vTaskDelay(DELAY_MS_IN_TICK(10000));
            }
        }
    }
    vTaskDelete(NULL);
}
