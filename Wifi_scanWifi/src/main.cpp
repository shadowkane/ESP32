#include <Arduino.h>
#include <string.h>
// OLED display library
#include <Wire.h>
#include <SSD1306Wire.h>
// wifi library
#include <WiFi.h>

/***** Constants and variables *****/
// wifi logo
#define WiFi_Logo_width 60
#define WiFi_Logo_height 36
const uint8_t WiFi_Logo_bits[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0x07, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF,
  0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
  0xFF, 0x03, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
  0x00, 0xFF, 0xFF, 0xFF, 0x07, 0xC0, 0x83, 0x01, 0x80, 0xFF, 0xFF, 0xFF,
  0x01, 0x00, 0x07, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x0C, 0x00,
  0xC0, 0xFF, 0xFF, 0x7C, 0x00, 0x60, 0x0C, 0x00, 0xC0, 0x31, 0x46, 0x7C,
  0xFC, 0x77, 0x08, 0x00, 0xE0, 0x23, 0xC6, 0x3C, 0xFC, 0x67, 0x18, 0x00,
  0xE0, 0x23, 0xE4, 0x3F, 0x1C, 0x00, 0x18, 0x00, 0xE0, 0x23, 0x60, 0x3C,
  0x1C, 0x70, 0x18, 0x00, 0xE0, 0x03, 0x60, 0x3C, 0x1C, 0x70, 0x18, 0x00,
  0xE0, 0x07, 0x60, 0x3C, 0xFC, 0x73, 0x18, 0x00, 0xE0, 0x87, 0x70, 0x3C,
  0xFC, 0x73, 0x18, 0x00, 0xE0, 0x87, 0x70, 0x3C, 0x1C, 0x70, 0x18, 0x00,
  0xE0, 0x87, 0x70, 0x3C, 0x1C, 0x70, 0x18, 0x00, 0xE0, 0x8F, 0x71, 0x3C,
  0x1C, 0x70, 0x18, 0x00, 0xC0, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x08, 0x00,
  0xC0, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x0C, 0x00, 0x80, 0xFF, 0xFF, 0x1F,
  0x00, 0x00, 0x06, 0x00, 0x80, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x07, 0x00,
  0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0xF8, 0xFF, 0xFF,
  0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x00,
  0x00, 0x00, 0xFC, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF,
  0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x80, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// I2C Pins
#define SDA_pin 21 // data
#define SCL_pin 22 // clock

// oledDisplay configuration
SSD1306Wire oledDisplay(0x3C, SDA_pin, SCL_pin, GEOMETRY_128_64);

void setup() {
  // put your setup code here, to run once:
  // configure serial monitoring
  Serial.begin(115200);
  Serial.println("Start program");
  // configure and initiate OLED display
  oledDisplay.init();
  oledDisplay.flipScreenVertically();
  oledDisplay.clear();
  oledDisplay.setFont(ArialMT_Plain_24);
  // alingnment is the postion of the string, left or right or center of the string, not the display. left alignment mean left of the string at the left of the screen, right alignment mean right of the string start from the left of the screen, center alignment mean center for the string start from the left of the screen.
  oledDisplay.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  oledDisplay.drawString(64, 32, "Welcome");
  oledDisplay.display();
  delay(5000);

  // wifi setup
  oledDisplay.clear();
  oledDisplay.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  oledDisplay.display();
  // set wifi mode to station
  WiFi.mode(WIFI_STA);
  // disconnection device from any AP
  WiFi.disconnect();
  delay(5000);

  // scan network until finding a specific AP (using the AP SSID)
  do{

  Serial.println("Scan wifi");
  int AP_nbr = WiFi.scanNetworks();
  Serial.print("nbr of AP found => ");
  Serial.println(AP_nbr);
  
    if(AP_nbr > 0){
      for(int i=0; i<AP_nbr; i++){
        Serial.print("AP SSID: ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (Strength: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(") ");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)? "":"Need Password");

        if(WiFi.SSID(i).compareTo("Your_SSID") == 0){
          if(WiFi.isConnected() == false){
            Serial.println("  try to connect");
            WiFi.begin("Your_SSID","Your_password");
            while(WiFi.status() != WL_CONNECTED){
              delay(1000);
              Serial.println("  Establishing connection to WiFi..");
            }
            Serial.println("  Connected to network");
            Serial.print("  Device Mac address: ");
            Serial.println(WiFi.macAddress());
            Serial.print("  Our Local IP address: ");
            Serial.println(WiFi.localIP());
          }
        }
      }
    }
  }while(!WiFi.isConnected());
}

void loop() {
  // put your main code here, to run repeatedly:
  
  delay(5000);
}
