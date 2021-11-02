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
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonFile);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      JsonArray MACHINE = doc["MACHINE"];
      #ifdef M_DEBUG
        Serial.printf("Create machine %d \n", machine_number_);
      #endif //DEBUG
      Pin *p_;
      for(JsonObject MACHINE_PINS_item : MACHINE[machine_number_]["PINS"].as<JsonArray>()){
        String s = MACHINE_PINS_item["PINTYPE"];
        int pin_ = MACHINE_PINS_item["HWObj"];
        Adafruit_MCP23X17 *mcp_ = &mcp[pin_];
        uint8_t adr_ = MACHINE_PINS_item["ADR"];
        if(s == "M") p_ = new Motor(mcp_, adr_);
        else if(s == "S") p_ = new Sensor(mcp_, adr_);  
        else if(s == "W") {
          p_ = new Weigher(&Serial, adr_);
        }
           
        this->add(p_);
        delete p_;
      }
    };

    void add (Pin *pin_){
      m_pins_col ++;
      Pin **temp_arr = new Pin*[this->m_pins_col]; 
      for(int i = 0; i < this->m_pins_col; i++){
        temp_arr[i] = m_pins_arr[i];
      }
      Serial.println("OK2");
      temp_arr[m_pins_col - 1] = pin_;
      m_pins_arr = temp_arr;
      delete [] temp_arr; 
      
    };
    
    bool remove(Pin* pin_){
      return false;
    };

};

#endif //MACHINE_H