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
  private:
    std::vector<Pin *>  P;
    uint8_t machine_num;
    //Pin **P;

  public:
    int m_pins_col;
    
    Machine (uint8_t m=0){
      this->machine_num = m;
      this->m_pins_col = 0;

      //P = new Pin*[1];
      #ifdef MACH_DEBUG
        Serial.printf("Machine: Create machine ------------------\n");
      #endif //M_DEBUG
    };

    void add (Pin *pin_){
      P.push_back(pin_);
      /*
      m_pins_col ++;
      Pin **temp_arr = new Pin*[this->m_pins_col]; // создаем новый массив указателей на Pin
      for(int i = 0; i < this->m_pins_col - 1; i++){ //переносим данные
        temp_arr[i] = P[i];
      }
      temp_arr[m_pins_col - 1] = pin_; //новые данные добавляем в новую ячейку
      delete [] P; // освобождаем память от старого массива
      P = temp_arr;
      */
    };

    void stopall(uint8_t _state){
      for(std::vector<Pin*>::iterator it = P.begin(); it != P.end(); ++it){
        if((*it)->get_type()==MOTOR){
          (*it)->set_state(_state);
        } 
      }
    };
    
    bool remove(Pin* pin_){
      return false;
    };

    Pin *at(uint8_t id){
      return P.at(id);
    };

    uint8_t get_machinenum(){
      return this->machine_num;
    };

    std::vector<Pin *> pins(){
      return P;
    };

};

#endif //MACHINE_H