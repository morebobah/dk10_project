#ifndef PIN_H
#define PIN_H

#include <arduino.h>
#include <Adafruit_MCP23X17.h>

enum PINTYPES{
  MOTOR, 
  SENSOR,
  WEIGHER
};

class Pin {
  protected:
    uint8_t m_pin; 
    PINTYPES m_pin_type;
    Adafruit_MCP23X17 *m_mcp;
    

  public:
    Pin(Adafruit_MCP23X17 *mcp_, uint8_t pin_, PINTYPES m_pin_type=MOTOR){
      this->m_mcp = mcp_;
      this->m_pin = pin_;
      this->m_pin_type = m_pin_type;
    };
    bool get_state(){
      return bool(this->m_mcp->digitalRead(this->m_pin));
    };
    uint8_t get_pin(){
      return this->m_pin;
    };
    PINTYPES get_type(){
      return this->m_pin_type;
    };
    void set_state(uint8_t state_){
      Serial.print("Pin ");
      Serial.print(this->m_pin);
      Serial.print(" ");
      Serial.print(this->m_pin_type);
      Serial.print(" set to ");
      Serial.println(state_);
      this->m_mcp->digitalWrite(this->m_pin, state_);
    };


};

#endif //PIN_H