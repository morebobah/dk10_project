#ifndef SENSOR_H
#define SENSOR_H

#include <arduino.h>
#include "Pin.h"

class Sensor : public Pin {
  public:
    Sensor (Adafruit_MCP23X17 *mcp_, uint8_t pin_):Pin(mcp_, pin_, SENSOR){
      m_mcp->pinMode(pin_, INPUT);
      #ifdef SENSOR_DEBUG
        Serial.printf("Sensor: Create sensor mcp %d pin %d\n", 0, pin_);
      #endif //DEBUG
    }
};


#endif //SENSOR_H