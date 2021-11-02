#ifndef MACHINE_H
#define MACHINE_H

#include <Arduino.h>
#include "Pin.h"
#include "Motor.h"
#include "Sensor.h"
#include "Weigher.h"
#include "ArduinoJson.h"
#include "SD.h"

extern HardwareSerial Serial;
extern File jsonFile;
extern Adafruit_MCP23X17 mcp[4];

class Machine {
  public:
    int m_pins_col;
    Pin **m_pins_arr;
    
    Machine (int machine_number_){
      this->m_pins_col = 0;
      this->m_pins_arr = new Pin*[1];
      #ifdef M_DEBUG
        Serial.printf("Create machine %d ------------------\n", machine_number_);
      #endif //M_DEBUG
    };

    void add (Pin *pin_){
      m_pins_col ++;
      Pin **temp_arr = new Pin*[this->m_pins_col]; 
      for(int i = 0; i < this->m_pins_col - 1; i++){
        temp_arr[i] = m_pins_arr[i];
      }
      temp_arr[m_pins_col - 1] = pin_;
      delete [] m_pins_arr;
      m_pins_arr = temp_arr;
      
      
    };
    
    bool remove(Pin* pin_){
      return false;
    };

};

#endif //MACHINE_H