/**************************
 * 
 * Create web server that control led
 * client will connect to the local server of the device to turn the led ON or OFF using http requests
 * 
***************************/

#include <Arduino.h>

#include <WiFi.h>
#include <ESPmDNS.h>

#define LEDPIN 25

const char* AP_SSID = "J_family";
const char* AP_Password = "1@&R_h&w_jomaa";

WiFiServer myServer(80);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Start program");
  // config pin
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  // config wifi
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
  
  // start server
  Serial.println("Start server");
  myServer.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  WiFiClient client = myServer.available();
  if(client){
    // run the script for the connected client while its still connected
    while(client.connected()){
      Serial.println("------------------------------- new client ---------------------------");
      // check if client sending data. if it's the case, store it in the clientMsg variable
      String clientMsg = "";
      while(client.available()){
        clientMsg += (char)client.read();
      }
      Serial.println(clientMsg);
      client.println("HTTP/1.1 200 OK");
      client.println("content-type:text/html");
      client.println("Connection: close");
      client.println();

      // if client request for root "/"
      if(clientMsg.indexOf("GET /led/on")>=0){
        digitalWrite(LEDPIN, HIGH);
      }
      else if(clientMsg.indexOf("GET /led/off")>=0){
        digitalWrite(LEDPIN, LOW);
      }
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<a href=\"/led/on\"> Trun Led ON</a>");
      client.println("<br />");
      client.println("<a href=\"/led/off\"> Trun Led OFF</a>");
      client.println("<br />");
      client.print("Led is ");
      if(digitalRead(LEDPIN) == HIGH){
        client.println("ON");
      }else{
        client.println("OFF");
      }
      client.println("</html>");
      client.println();
      

      client.stop();
      Serial.println("Client left");
    }
  }
  delay(100);
}