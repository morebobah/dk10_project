#ifndef SENSOR_H
#define SENSOR_H

#include <arduino.h>
#include "Pin.h"

class Sensor : public Pin {
  public:
    Sensor (Adafruit_MCP23X17 *mcp_, uint8_t pin_):Pin(mcp_, pin_){
      m_pin_type = SENSOR;
      m_mcp->pinMode(m_pin, INPUT);
      #ifdef M_DEBUG
        Serial.printf("Create sensor mcp %d pin %d\n", 0, pin_);
      #endif //DEBUG
    }
};


#endif //SENSOR_H