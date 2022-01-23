#ifndef MOTOR_H
#define MOTOR_H

#include <arduino.h>
#include "Pin.h"

class Motor : public Pin {
  private:
    uint8_t motor_state = 0;
  public:
    Motor(Adafruit_MCP23X17 *mcp_, uint8_t pin_):Pin(mcp_, pin_, MOTOR){
      m_mcp->pinMode(pin_, OUTPUT);
      m_mcp->digitalWrite(pin_, 1);
      #ifdef MOTOR_DEBUG
        Serial.printf("Motor: Create motor pin %d\n", pin_);
      #endif //DEBUG
    }

    void set_state(uint8_t state_){
      motor_state = state_;
      Pin::set_state(state_);
    };

    bool get_state(){
      return (bool)motor_state;
    }
};


#endif // MOTOR_H