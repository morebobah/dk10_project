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
extern Adafruit_MCP23X17 mcp[4];
extern File jsonFile;

class MHandler {
  public:
    int machines_col;
    Machine **M;
    
    MHandler(){
      this->machines_col = 0;
      M = new Machine*[1];
    };
    void begin(){
      jsonFile = SD.open("/MACHINE_STRUCT.json");
      if(!jsonFile) Serial.println("Don't open file");
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonFile);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      for (int i = 0; i < 4; i++) {
        this->add();
        JsonObject MACHINE_item = doc["MACHINE"][i];
        for(JsonObject MACHINE_PINS_item : MACHINE_item["PINS"].as<JsonArray>()){
          String s = MACHINE_PINS_item["PINTYPE"];
          int pin_ = MACHINE_PINS_item["HWObj"];
          Adafruit_MCP23X17 *mcp_ = &mcp[pin_];
          uint8_t adr_ = MACHINE_PINS_item["ADR"];
          if(s == "M") M[i]->add(new Motor(mcp_, adr_));
          else if(s == "S") M[i]->add(new Sensor(mcp_, adr_));  
          else if(s == "W") {
            M[i]->add(new Weigher(&Serial, adr_));
          }
        }
      }
      jsonFile.close();
    };

    void add (){
      machines_col++;
      Machine *machine = new Machine ();
      Machine **temp_arr = new Machine*[this->machines_col];
      for (int i = 0; i < this->machines_col-1; i++){
        temp_arr[i] = M[i];
      }
      temp_arr[machines_col - 1] = machine;
      delete [] M;
      M = temp_arr;
    };
};
MHandler MH;
#endif //MHANDLER_H