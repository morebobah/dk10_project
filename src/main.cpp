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


Adafruit_MCP23X17 mcp[4];
//File jsonFile;
SoftwareSerial SWSerial;

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
  
  
  MH.begin();
  
}

void loop() {
  
  Weigher* w = (Weigher*)MH.M[0]->P[5];
  //Serial.println((int)w->getV());
  w->getV();
  Serial.println(millis());
  delay(10);
 
}