#include <Arduino.h>
#include <string>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN 25

// AP settings
const char AP_ssid[] = "Your_SSID";
const char AP_password[] = "Your_password";
// MQTT Broker config
const char Broker_server[] = "m24.cloudmqtt.com";
const int Broker_port = 11981;
const char Broker_user[] ="slqmuxlm";
const char Broker_password[] ="kxDQ9KOoJSkh";
// MQTT client settings
const char MQTT_client_id[] ="esp32Client";
// create a client socket
// WiFiClient is socket: connect, write, flush, read, close. (Note: HTTPClient handles HTTP protocol over WiFiClient: get, post, parse response)
WiFiClient clientSocket;
// create the mqtt client
PubSubClient mqtt_client(clientSocket);

// usefull functions
void printPublishState(String source, bool pubState){
  Serial.print(source);
  if(pubState == false){
    Serial.println(" failed to publish the message.");
  }else{
    Serial.println(" successfuly published the message.");
  }
}

// MQTT callback
void MQTTCallback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message received from the Broker about the topic ");
  Serial.println(topic);
  Serial.print("Message: ");
  for(int i=0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  if(length == 1){
    if((char)payload[0] == 'H'){
      digitalWrite(LED_PIN, HIGH);
    }
    else if((char)payload[0] == 'L'){
      digitalWrite(LED_PIN, LOW);
    }
  }
 
  Serial.println();
}

void setup() {
  //************ config the serial communication ************//
  Serial.begin(115200);
  Serial.println("Start program");

  //************ config GPIO ************//
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  //************ config Wifi ************//
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(AP_ssid, AP_password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Estableshing connection...");
    delay(1000);
  }
  Serial.println("Device connected to AP");
  randomSeed(micros());
  //************ config MQTT ************//
  // set the broker settings
  mqtt_client.setServer(Broker_server, Broker_port);
  // set the subscription callback function. this function will fire each time a message received from the broker about the topic we subscribed to.
  mqtt_client.setCallback(MQTTCallback);
  // connect to broker
  while(!mqtt_client.connected()){
    Serial.println("Connecting to the Broker...");
    if(mqtt_client.connect(MQTT_client_id, Broker_user, Broker_password)){
      Serial.println("Client connected to the broker");
    }else{
      Serial.println("Couldn't connect to server.");
      Serial.println(mqtt_client.state());
      Serial.println("Try again in 5 seconds");
      delay(5000);
    }
  }
  // subscribe to a topi
  Serial.println("Subscribe to the ledCmd topic");
  mqtt_client.subscribe("esp32/ledCmd");
}

void loop() {
  Serial.println("Read hall sensor");
  int hall_val = hallRead();
  char msg[5];
  bool pubState;
  sprintf(msg, "%d", hall_val);
  Serial.println("Publiss the hall sensor value to the MQTT broker under the hallSensor topic");
  // using a normal publish function
  pubState = mqtt_client.publish("esp32/hallSensor", msg);
  printPublishState("sending hall sensor", pubState);
  // send a very long messages using begin/stop publish
  char long_msg[]="this is a very long message from esp client device i wan't to send to the broker to check if it works or not because i know that the defautl maximum size to send is 127 bytes";
    // to prouve that a regular publish won't work
  pubState = mqtt_client.publish("esp32/longPub", long_msg);
  printPublishState("sending a long string with one pub", pubState);
    // begin the pusblishing
  mqtt_client.beginPublish("esp32/longPub", strlen(long_msg), false);
  char sub_long_msg[5]; 
  strncpy(sub_long_msg, long_msg, 5);
  mqtt_client.write((byte*)sub_long_msg, 5);
    // send the rest of the message
  for(int i=5; i<strlen(long_msg); i++){
    mqtt_client.write(long_msg[i]);
  }
    // close the publishing
  mqtt_client.endPublish();
  mqtt_client.loop();
  delay(10000);
}
