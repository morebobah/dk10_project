#ifndef MOTOR_H
#define MOTOR_H

#include <arduino.h>
#include "Pin.h"

class Motor : public Pin {
  public:
    Motor(Adafruit_MCP23X17 *mcp_, uint8_t pin_):Pin(mcp_, pin_, MOTOR){
      m_mcp->pinMode(m_pin, OUTPUT);
      #ifdef M_DEBUG
        Serial.printf("Create motor pin %d\n", pin_);
      #endif //DEBUG
    }
};


#endif // MOTOR_H