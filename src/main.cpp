#include <Arduino.h>
#include <FreeRTOS.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include "Motor.h"
#include "Sensor.h"
#include "Weigher.h"

#define RX 4
#define TX 5

HardwareSerial Serial485(1);

Adafruit_MCP23X17 mcp1;
Adafruit_MCP23X17 mcp2;
Adafruit_MCP23X17 mcp3;
Adafruit_MCP23X17 mcp4;

Motor m1(&mcp1, 0);
Sensor s1(&mcp1, 1);
Weigher w1(&Serial485,1);

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
  mcp1.begin_I2C(0x20);
  mcp2.begin_I2C(0x21);
  mcp3.begin_I2C(0x22);
  mcp4.begin_I2C(0x23);
  Serial485.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
}