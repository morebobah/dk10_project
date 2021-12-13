#define M_DEBUG

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include "Motor.h"
#include "Sensor.h"
#include "Weigher.h"
#include <ArduinoJson.h>
#include "MHandler.h"
#include "SD.h"
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WebSocketsServer.h> 


Adafruit_MCP23X17 mcp[4];
//File jsonFile;
SoftwareSerial SWSerial;
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

void setup() {
  /* Hardware address A3A2A1
  0 0 0 = 0x20
  0 0 1 = 0x21
  0 1 0 = 0x22
  0 1 1 = 0x23
  1 0 0 = 0x24
  1 0 1 = 0x25
  1 1 0 = 0x26
  1 1 1 = 0x27
  */
  mcp[0].begin_I2C(0x20);
  mcp[1].begin_I2C(0x21);
  mcp[2].begin_I2C(0x22);
  mcp[3].begin_I2C(0x23);
  pinMode(9, INPUT);
  Serial.begin(9600);
  Serial.println();
  Serial.println("Serial begin");
  SWSerial.begin(9600,SWSERIAL_8N1, D3, D4);
  Serial.println("SWSerial begin");
  Serial.setTimeout(50);
  SWSerial.setTimeout(50);
  if(SD.begin(SS)) Serial.println("SD ok");
  else Serial.println("SD failed");
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("connected...yeey :)");
  delay(1000);
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  MH.begin();
  
}

void loop() {
  webSocket.loop();
  delay(10);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:  // Событие происходит при отключени клиента 
      Serial.println("web Socket disconnected");
      break;
    case WStype_CONNECTED: // Событие происходит при подключении клиента
        Serial.println("web Socket Connected"); 
      break;
    case WStype_TEXT: // Событие происходит при получении данных текстового формата из webSocket
      if (strcmp((char*)payload, "getms") == 0){
        File f = SD.open("/MACHINE_STRUCT.json");
        String s = "";
        while(f.available()){
          s += (char)f.read();
        }
        webSocket.broadcastTXT(s);
        f.close();
      }
      else{
        Serial.println((char*)payload);
        //Serial.println();
      }
      
      break;
    case WStype_BIN:      // Событие происходит при получении бинарных данных из webSocket
      // webSocket.sendBIN(num, payload, length);
      break;
  }
}