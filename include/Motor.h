#ifndef MOTOR_H
#define MOTOR_H

#include <arduino.h>
#include "Pin.h"

class Motor : public Pin {
  public:
    Motor(Adafruit_MCP23X17 *mcp_, uint8_t pin_):Pin(mcp_, pin_, MOTOR){
      m_mcp->pinMode(pin_, OUTPUT);
      m_mcp->digitalWrite(pin_, 1);
      #ifdef MOTOR_DEBUG
        Serial.printf("Motor: Create motor pin %d\n", pin_);
      #endif //DEBUG
    }
};


#endif // MOTOR_H