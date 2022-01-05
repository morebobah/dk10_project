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
//#include <WiFi.h> 
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
  boolean bSD = SD.begin(SS);
  while(!bSD){
    Serial.println("SD failed");
    delay(1000);
    bSD = SD.begin(SS);
  }
  
  WiFiManager wifiManager;
  /*
  if (!WiFi.config("", "", "255.255.255.0")) { //WiFi.config(ip, gateway, subnet, dns1, dns2);
    Serial.println("WiFi STATION Failed to configure Correctly");
  }
  */
  wifiManager.autoConnect("AutoConnectAP");
  /*
  WiFi.mode(WIFI_STA);
  WiFi.begin("universe", "UmuiDvjy");
  while (WiFi.status() != WL_CONNECTED){
    Serial.println(WiFi.status());
    Serial.println(WiFi.macAddress());
    delay(3000);
  }
  */
  Serial.println("WiFi was connected.)");
  delay(1000);
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  MH.begin();
  std::vector<int> a;
  std::vector<int>::iterator it = a.begin();
  a.push_back(0);
  a.pop_back();
  a.insert(a.end(), 3);
  it++;

}

void loop() {
  webSocket.loop();
  delay(10);
  MH.process();
  delay(10);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String upload = "Unknown command";
  switch (type) {
    case WStype_DISCONNECTED:  // Событие происходит при отключени клиента 
      Serial.println("web Socket disconnected");
      break;
    case WStype_CONNECTED: // Событие происходит при подключении клиента
        Serial.println("web Socket Connected"); 
      break;
    case WStype_TEXT: // Событие происходит при получении данных текстового формата из webSocket
      Serial.println("Text recived");
      switch (MH.brancher((char*)payload)) {
        case 1:
        Serial.println("1");
        case 2:
         Serial.println("2");
          upload = MH.config();
          break;
        case 3:
        Serial.println("3");
          upload = MH.handwork((char*)payload);
          break;
        case 4:
          Serial.println("4");
          break;
        case 5:
          Serial.println("5");
          break;
        case 6:
          Serial.println("6");
          break;
        case 7:
          Serial.println("7");
          break;
        default:
          Serial.println("default case unknown command");
          break;
        break;
      }
      Serial.println(upload.substring(0, 20));
      webSocket.broadcastTXT(upload);
      break;
    case WStype_BIN:      // Событие происходит при получении бинарных данных из webSocket
      // webSocket.sendBIN(num, payload, length);
      break;
    default:
        break;
  }
}