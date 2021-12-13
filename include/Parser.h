#pragma once

#include <Arduino.h>

#define IS_COMAND(a)   ((a) == 'G' || (a) == 'M' || (a) == 'W' || (a) == 'S')  
#define IS_NUMERIC(a) ((a) >= '0' && (a) <= '9')         

class Parser {

  public:
    uint8_t m_machine;
    uint8_t m_pin;
    uint8_t m_value;
    uint16_t m_weight;

    bool parse (char *p){
      uint8_t *code;
      while (*p !='\0'){
        if (IS_COMAND(*p)){
          switch (*P){
            case 'G':
              code = &m_machine;
              break;
            case 'M':
              code = &m_pin;
              break;
            case 'V':
              code = &m_value;
              break;
            case 'W':
              code = &m_weight;
              break;
          }
          p++;
        }
        else if (IS_NUMERIC(*p)){
          *code = (*code ) * 10 + (*p) - '0';
          p++;
        }
        else return false;

      return true;
    }
};


