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

extern HardwareSerial Serial;
extern File jsonFile;
extern Adafruit_MCP23X17 mcp[4];

class MHandler {
  public:
    int machines_col;
    Machine **m_machines_arr;
    
    MHandler(){
      this->machines_col = 0;
      this->m_machines_arr = new Machine*[1];
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonFile);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      for (int i = 0; i < 4; i++) {
        this->add(i);
        JsonObject MACHINE_item = doc["MACHINE"][i];
        for(JsonObject MACHINE_PINS_item : MACHINE_item["PINS"].as<JsonArray>()){
          String s = MACHINE_PINS_item["PINTYPE"];
          int pin_ = MACHINE_PINS_item["HWObj"];
          Adafruit_MCP23X17 *mcp_ = &mcp[pin_];
          uint8_t adr_ = MACHINE_PINS_item["ADR"];
          if(s == "M") m_machines_arr[i]->add(new Motor(mcp_, adr_));
          else if(s == "S") m_machines_arr[i]->add(new Sensor(mcp_, adr_));  
          else if(s == "W") {
            m_machines_arr[i]->add(new Weigher(&Serial, adr_));
          }
        }
      }

    };

    void add (int number_){
      machines_col++;
      Machine *machine = new Machine (number_);
      Machine **temp_arr = new Machine*[this->machines_col];
      for (int i = 0; i < this->machines_col-1; i++){
        temp_arr[i] = m_machines_arr[i];
      }
      temp_arr[machines_col - 1] = machine;
      delete [] m_machines_arr;
      m_machines_arr = temp_arr;
    };
};

#endif //MHANDLER_H