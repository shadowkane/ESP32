#include <iostream>
#include <regex.h>
#include <string>

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <webPages.h>

using namespace std;

#define LEDPIN 25

const char* AP_SSID = "J_family";
const char* AP_Password = "1@&R_h&w_jomaa";

WiFiServer myServer(80);

void sendHomePage(WiFiClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(homePage);
  client.println();
}

void sendControlPage(WiFiClient client, int ledStatus){
  client.println("HTTP/1.1 200 OK");
  client.println("content-type:text/html");
  client.println("Connection: close");
  client.println();
  String ledStatus_str = "OFF";
  if(ledStatus == HIGH){
    ledStatus_str = "ON";
  }
  String newControlPage = controlPage;
  Serial.println(newControlPage);
  newControlPage.replace("{ledStatus}", ledStatus_str);
  Serial.println("------------------------------------------------------------------------------------------");
  Serial.println(newControlPage);
  client.println(newControlPage);
  client.println();
}

void sendDashboardPage(WiFiClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(dashboardPage);
  client.println();
}

void sendDataToClient(WiFiClient client, int data){
  client.println("HTTP/1.1 200 OK");
  client.println("content-type:application/json");
  client.println("Connection: close");
  client.println();
  //char data_str[5];
  //itoa(data, data_str, 10);
  client.print("{\"hall_val\":");
  client.print(data);
  client.println("}");
  client.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Start program");
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(AP_SSID, AP_Password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Estableshing connection with AP");
    delay(1000);
  }
  Serial.println("Connected");
  if(!MDNS.begin("homecontrol")){
    Serial.println("Error in setting up the mDNS");
    while(1){
      delay(100000);
    }
  }
  
  myServer.begin();
  Serial.println("Server begin");
}

void loop() {
  WiFiClient client = myServer.available();
  if(client){
    while(client.connected()){
      String clientMsg = "";
      while(client.available()){
        clientMsg += (char)client.read();
      }
      Serial.println(clientMsg);
      // page redirection
      if(clientMsg.indexOf(" / ")>=0){
        sendHomePage(client);
      }
      else if(clientMsg.indexOf(" /controlpage ")>=0){
        sendControlPage(client, digitalRead(LEDPIN));
      }
      else if(clientMsg.indexOf(" /dashboardpage ")>=0){
        sendDashboardPage(client);
      }
      
      // get requestion
      if(clientMsg.indexOf(" /ledOn ")>=0){
        digitalWrite(LEDPIN, HIGH);
        sendControlPage(client, digitalRead(LEDPIN));
      }
      else if(clientMsg.indexOf(" /ledOff ")>=0){
        digitalWrite(LEDPIN, LOW);
        sendControlPage(client, digitalRead(LEDPIN));
      }
      else if(clientMsg.indexOf(" /hallSensor ")>=0){
        sendDataToClient(client, hallRead());
      }

      client.stop();
    }
  }
  delay(10);
}