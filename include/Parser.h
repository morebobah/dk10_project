#pragma once

#include <Arduino.h>

#define IS_COMAND(a)   ((a) == 'G' || (a) == 'M' || (a) == 'W' || (a) == 'S')

class Parser {

  public:
    uint8_t m_machine;
    uint8_t m_pin;
    uint8_t value;

    bool parse (char *p){
      
      if (*p == 'G'){
        p++;
        while (!IS_COMAND (*p)){
          m_machine = m_machine * 10 + *p++ - '0';
        }

      }
      else  return false;

      return true;
    }
};


