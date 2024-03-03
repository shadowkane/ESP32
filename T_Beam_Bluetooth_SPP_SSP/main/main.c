#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
// BT
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
// BT-GAP (Generic Access Profile)
#include "esp_gap_bt_api.h"
// BT-SPP (Serial Port Profile)
#include "esp_spp_api.h"

/** Macros **/
#define LED_PIN 14
#define LOG_TAG "T_BEAM_LOG_TAG"
#define SPP_SERVER_NAME "T_BEAM_SERVER"
#define BT_DEVICE_NAME "T_BEAM_Bluetooth_SPP_SSP"
#define CMD_MAX_LEN 15
/** Functions **/
// Led Config & Init
void vLedConfig();
void vLedInit();
// Bluetooth
//  Init functions
void vBtInitialization();
void vBtGapInitialization();
void vBtSppInitialization();
// tools
void vBtData2Str(uint8_t* pcHexBuffer, char pcStr[], uint8_t u8Len);
bool bHandleCmd(uint8_t* pcHexBuffer, uint8_t u8Len);
//  Callback functions
void vGap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void vSpp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);


void app_main(void)
{
    esp_err_t ret;
    uint8_t* pu8DeviceAddr;

    // nvs init
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(LOG_TAG, "Start T_Beam_Bluetooth_SPP_SSP");
    // Led setup
    vLedConfig();
    vLedInit();

    // Bluetooth setup
    vBtInitialization();
    vBtGapInitialization();
    vBtSppInitialization();
    // display this device bluetooth address
    pu8DeviceAddr = esp_bt_dev_get_address();
    if(pu8DeviceAddr!=NULL){
        ESP_LOGI(LOG_TAG, "Own address:[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]", pu8DeviceAddr[0], pu8DeviceAddr[1], pu8DeviceAddr[2], pu8DeviceAddr[3], pu8DeviceAddr[4], pu8DeviceAddr[5]);
    }
    else{
        ESP_LOGI(LOG_TAG, "Own address not found");
    }
}

// Bluetooth Initialization
void vBtInitialization(){
    ESP_LOGI(LOG_TAG, "vBtInitialization");
    // BT setup is:
    //  use Classic BT only
    //  use Bluedroid because bluedroid supports Classice BT and BLE when nimble supports only BLE
    //  inside Bluedroid enable SSP (Secure Simple Pairing)
    
    esp_err_t ret;
    esp_bt_controller_config_t xBtConfig = BT_CONTROLLER_INIT_CONFIG_DEFAULT(); // BT configuration variable with default configuration
    esp_bluedroid_config_t xBluedroidConfig = BT_BLUEDROID_INIT_CONFIG_DEFAULT(); // SSP is true by default which is valid for our case

    // BT CONTROLLER CONFIGURATION
    //  Since we will use "Classic BT" only, then, we need to release or free the BLE mode from the BT controller
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    //  Init BT controller with default configuration
    ESP_ERROR_CHECK(esp_bt_controller_init(&xBtConfig));
    //  Enable BT controller with "Classic BT" mode
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    
    // BT CONFIGURATION
    //  Init Bluedroid with default configuration
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&xBluedroidConfig));
    //  Enable Bluedroid
    ESP_ERROR_CHECK(esp_bluedroid_enable());
}

void vBtGapInitialization(){
    ESP_LOGI(LOG_TAG, "vBtGapInitialization");
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(vGap_callback));
    // Set default parameters for SSP (Secure Simple Pairing)
    // in this version of ESP32 firmware only IOCAP_MODE is supported
    // This BT device can only display message and the user BT device (example: mobile) has keyboard and display capabilities, so CAP is IO, ESP32 will choose random value and the user will verify if it matches the numeric value in mobile screen before confirming the connection
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    ESP_ERROR_CHECK(esp_bt_gap_set_security_param(param_type, &iocap, sizeof(esp_bt_io_cap_t)));
}

void vBtSppInitialization(){
    ESP_LOGI(LOG_TAG, "vBtSppInitialization");
    esp_spp_cfg_t bt_spp_cfg = {
        .mode = ESP_SPP_MODE_CB, // use Callback mode
        .enable_l2cap_ertm = true, // enable L2CAP because SPP (transport layer) in made on top of I2CAP (logic link control and adaptation protocol) L2 in OSI
        .tx_buffer_size = 0, /* Only used for ESP_SPP_MODE_VFS mode */
    };
    ESP_ERROR_CHECK(esp_spp_register_callback(vSpp_callback));
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&bt_spp_cfg));
}

// Led Config & Init
void vLedConfig(){
    ESP_LOGI(LOG_TAG, "vLedConfig");
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_DEF_OUTPUT);
}
void vLedInit(){
    ESP_LOGI(LOG_TAG, "vLedInit");
    gpio_set_level(LED_PIN, 0);
}

void vBtData2Str(uint8_t* pcHexBuffer, char pcStr[], uint8_t u8Len){
    // exclude all line feed and carry (0x0D 0x0A)return located in the end of the data
    while(pcHexBuffer[u8Len-1]==0x0D || pcHexBuffer[u8Len-1]==0x0A){
        u8Len--;
    }
    memcpy(pcStr, pcHexBuffer, u8Len);
}

bool bHandleCmd(uint8_t* pcHexBuffer, uint8_t u8Len){
    bool bRet = true;
    char pcCmd[CMD_MAX_LEN];

    if(u8Len>CMD_MAX_LEN){
        bRet = false;
    }
    vBtData2Str(pcHexBuffer, pcCmd, u8Len);
    ESP_LOGI(LOG_TAG, "CMD(%d)=%s", strlen(pcCmd), pcCmd);
    if (strncmp(pcCmd, "SET_LED_ON", 10) == 0)
    {
        gpio_set_level(LED_PIN, 1);
        ESP_LOGI(LOG_TAG, "Turn LED ON");
    }
    else if (strncmp(pcCmd, "SET_LED_OFF", 11) == 0)
    {
        gpio_set_level(LED_PIN, 0);
        ESP_LOGI(LOG_TAG, "Turn LED OFF");
    }
    else
    {
        bRet = false;
    }
    return bRet;
}

void vGap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param){
    switch(event){
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT=%d", event);
            break;
        }
        case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT=%d", event);
            break;
        }
        case ESP_BT_GAP_AUTH_CMPL_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_AUTH_CMPL_EVT=%d", event);
            if(param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS){
                ESP_LOGI(LOG_TAG, "authentication success: %s bda: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", param->auth_cmpl.device_name, param->auth_cmpl.bda[0], param->auth_cmpl.bda[1], param->auth_cmpl.bda[2], param->auth_cmpl.bda[3], param->auth_cmpl.bda[4], param->auth_cmpl.bda[5]);
            }
            else {
                ESP_LOGI(LOG_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;
        }
        case ESP_BT_GAP_CFM_REQ_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_CFM_REQ_EVT=%d", event);
            ESP_LOGI(LOG_TAG, "Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
            ESP_ERROR_CHECK(esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true));
            break;
        }
        case ESP_BT_GAP_KEY_NOTIF_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_KEY_NOTIF_EVT=%d", event);
            break;
        }
        case ESP_BT_GAP_KEY_REQ_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_KEY_REQ_EVT=%d", event);
            break;
        }
        case ESP_BT_GAP_MODE_CHG_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_MODE_CHG_EVT=%d", event);
            break;
        }
        case ESP_BT_GAP_PIN_REQ_EVT:{
            ESP_LOGI(LOG_TAG, "GAP Event is ESP_BT_GAP_PIN_REQ_EVT=%d", event);
            break;
        }
        default:{
            ESP_LOGI(LOG_TAG, "GAP Event is default=%d", event);
            break;
        }
    }
} 

void vSpp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
    switch(event){
        case ESP_SPP_INIT_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_INIT_EVT=%d | status=%d", event, param->init.status);
            if (param->init.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(LOG_TAG,"SPP Start server.");
                // we will use a secured connection based on authentication
                // also this device will accept requests from a client which mean the server will act as a slave
                // use any channel (local_scn=0)
                // use pre defined server name
                ESP_ERROR_CHECK(esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME));
            } else {
                ESP_LOGE(LOG_TAG, "SPP failed to init.");
            }
            break;
        }
        case ESP_SPP_UNINIT_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_UNINIT_EVT=%d", event);
            break;
        }
        case ESP_SPP_DISCOVERY_COMP_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_DISCOVERY_COMP_EVT=%d", event);
            break;
        }
        case ESP_SPP_OPEN_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_OPEN_EVT=%d", event);
            break;
        }
        case ESP_SPP_CLOSE_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_CLOSE_EVT=%d", event);
            break;
        }
        case ESP_SPP_START_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_START_EVT=%d handle:%"PRIu32" security_id:%d channel_number:%d", event, param->start.handle, param->start.sec_id, param->start.scn);
            if (param->start.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(LOG_TAG, "BT set device name to=%s", BT_DEVICE_NAME);
                ESP_ERROR_CHECK(esp_bt_dev_set_device_name(BT_DEVICE_NAME));
                ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));
            } else {
                ESP_LOGE(LOG_TAG, "SPP failed to start.");
            }
            break;
        }
        case ESP_SPP_CL_INIT_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_CL_INIT_EVT=%d", event);
            break;
        }
        case ESP_SPP_DATA_IND_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_DATA_IND_EVT=%d", event);
            // display received data
            ESP_LOGI(LOG_TAG, "Received Data len:%d handle:%"PRIu32, param->data_ind.len, param->data_ind.handle);
            if (param->data_ind.len < 128) {
                esp_log_buffer_hex("", param->data_ind.data, param->data_ind.len);
            }
            // handle received cmd
            if(bHandleCmd(param->data_ind.data, param->data_ind.len)){
                param->data_ind.len = 2;
                param->data_ind.data[0] = 'O';
                param->data_ind.data[1] = 'K';
            }
            else{
                param->data_ind.len = 3;
                param->data_ind.data[0] = 'N';
                param->data_ind.data[1] = 'O';
                param->data_ind.data[2] = 'K';
            }
            esp_spp_write(param->data_ind.handle, param->data_ind.len, param->data_ind.data);
            break;
        }
        case ESP_SPP_CONG_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_CONG_EVT=%d", event);
            break;
        }
        case ESP_SPP_WRITE_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_WRITE_EVT=%d", event);
            break;
        }
        case ESP_SPP_SRV_OPEN_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_SRV_OPEN_EVT=%d | remote device address: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", event, param->srv_open.rem_bda[0], param->srv_open.rem_bda[1], param->srv_open.rem_bda[2], param->srv_open.rem_bda[3], param->srv_open.rem_bda[4], param->srv_open.rem_bda[5]);
            break;
        }
        case ESP_SPP_SRV_STOP_EVT:{
            ESP_LOGI(LOG_TAG, "SPP Event is ESP_SPP_SRV_STOP_EVT=%d", event);
            break;
        }
        default:{
            ESP_LOGI(LOG_TAG, "SPP Event is default=%d", event);
            break;
        }
    }
}