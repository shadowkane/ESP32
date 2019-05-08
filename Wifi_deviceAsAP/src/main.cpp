#include <Arduino.h>
#include <WiFi.h>

const char *mySSID = "myWifiApp";
const char *myPassword = "00000000";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  WiFi.softAP(mySSID, myPassword);
  Serial.print("AP IP is ");
  Serial.println(WiFi.softAPIP());
  
}

void loop() {
  Serial.println("Working");
  delay(10000);
}