#ifndef MHANDLER_H
#define MHANDLER_H

#include <Arduino.h>
#include "Pin.h"
#include "Motor.h"
#include "Sensor.h"
#include "Weigher.h"
#include "ArduinoJson.h"
#include "SD.h"
#include "Machine.h"

extern File jsonFile;


class MHandler {
  public:
    int machines_col;
    Machine **m_machines_arr;
    
    MHandler(){
      for (int i = 0; i < 4; i++ ) {
        this->add(i);
      }
    };

    void add (int number_){
      Machine *machine = new Machine (number_);
      machines_col++;
      Machine **temp_arr = new Machine*[this->machines_col];
      for (int i = 0; i < machines_col-1; i++){
        temp_arr[i] = m_machines_arr[i];
      }
      temp_arr[machines_col - 1] = machine;
      delete [] m_machines_arr;
      m_machines_arr = temp_arr;
    };
};

#endif //MHANDLER_H