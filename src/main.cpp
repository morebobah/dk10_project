#define MAIN_DEBUG
#define MH_DEBUG
#define MACH_DEBUG
#define PIN_DEBUG
#define MOTOR_DEBUG
#define SENSOR_DEBUG
#define WEI_DEBUG
//#define INI_DEBUG


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
#include <liquidcrystal_i2c.h>

#define COLUMS           16
#define ROWS             2

Adafruit_MCP23X17 mcp[4];
//File jsonFile;
SoftwareSerial SWSerial;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

void setup() {
  Serial.begin(9600);
  /*while (lcd.begin(COLUMS, ROWS, LCD_5x8DOTS, D2, D1) != 1) //colums - 20, rows - 4, pixels - 5x8, SDA - D2, SCL - D1
  {
    Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);
  }*/
  boolean bLCD = lcd.begin(COLUMS, ROWS, LCD_5x8DOTS, D2, D1);
  if(bLCD){
    lcd.print(F("Start")); //(F()) saves string to flash & keeps dynamic memory free
    delay(500);
    lcd.clear();
  }
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
  //pinMode(9, INPUT);
  
  #ifdef MAIN_DEBUG
    Serial.println();
    Serial.println("Main: Serial begin");
    SWSerial.begin(9600,SWSERIAL_8N1, D4, D3);
  #endif
  pinMode(D4, INPUT);
  #ifdef MAIN_DEBUG
    Serial.println("Main: SWSerial begin");
    Serial.setTimeout(50);
  #endif
  SWSerial.setTimeout(100);

  boolean bSD = SD.begin(SS);
  while(!bSD){
    #ifdef MAIN_DEBUG
      Serial.println("Main: SD failed");
    #endif
    if(bLCD) lcd.print(F("SD failed")); 
    delay(1000);
    bSD = SD.begin(SS);
  }
  WiFiManager wifiManager;
 
  wifiManager.autoConnect("AutoConnectAP");
  if(bLCD){
    lcd.setCursor(0, 0);
    lcd.print(WiFi.localIP());
  }
  #ifdef MAIN_DEBUG
    Serial.println("Main: WiFi was connected.)");
    delay(1000);
  #endif

  MH.begin();  
  MH.webSocket.begin();
  MH.webSocket.onEvent(webSocketEvent);
  MH.bLCD = bLCD;
}

void loop() {
  MH.process();
  delay(10);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  String upload = (char*)payload;
  switch (type) {
    case WStype_DISCONNECTED:  // Событие происходит при отключени клиента 
      #ifdef MAIN_DEBUG
        Serial.println("Main: web Socket disconnected");
      #endif
      if(MH.bLCD){
        lcd.setCursor(0, 1);
        lcd.print(F("Socet disconnect"));
      }
      break;
    case WStype_CONNECTED: // Событие происходит при подключении клиента
        #ifdef MAIN_DEBUG
          Serial.println("Main: web Socket Connected");
        #endif
        if(MH.bLCD){
          lcd.setCursor(0, 1);
          lcd.print(F("Socet Connected "));
        }
      break;
    case WStype_TEXT: // Событие происходит при получении данных текстового формата из webSocket
      #ifdef MAIN_DEBUG
        Serial.println("Main: Text recived");
        Serial.println((char*)payload);
      #endif
      MH.brancher((char*)payload);
      //MH.webSocket.broadcastTXT(upload);
      break;
    case WStype_BIN:      // Событие происходит при получении бинарных данных из webSocket
      // webSocket.sendBIN(num, payload, length);
      break;
    default:
        break;
  }
}