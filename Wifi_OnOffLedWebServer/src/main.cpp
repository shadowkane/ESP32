/**************************
 * 
 * Create web server that control led
 * client will connect to the local server of the device to turn the led ON or OFF using http requests
 * 
***************************/

#include <Arduino.h>

#include <WiFi.h>
#include <ESPmDNS.h>

const char* AP_SSID = "J_family";
const char* AP_Password = "1@&R_h&w_jomaa";

WiFiServer myServer(80);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Start program");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println("Connect to AP");
  WiFi.begin(AP_SSID, AP_Password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Estableshing connection ...");
    delay(1000);
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.println("Setting up mDNS");
  if(!MDNS.begin("myLocalWifi")){
    Serial.println("Error in setting up mDNS");
    while(1){
      delay(1000);
    }
  }
  Serial.println("mDNS ready");

  

}

void loop() {
  // put your main code here, to run repeatedly:
  WiFiClient client = myServer.available();
  if(client){
    Serial.println("------------------------------- new client ---------------------------");
    String clientMsg = "";
    while(client.available()){
      clientMsg += client.read();
    }
    Serial.println(clientMsg);
  }
}