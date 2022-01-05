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
#include "SoftwareSerial.h"
#include "inifile.h"

extern HardwareSerial Serial;
extern Adafruit_MCP23X17 mcp[4];
//extern File jsonFile;
extern SoftwareSerial SWSerial;


struct COMMAND{
    byte  type = 0; //use for weight from 1 to 254 kg, 0kg mean without weighter
    byte machine = 0;
    byte pin = 0;
    uint8_t value=0;
    uint64_t time_start = 0; //Start value for millis function
    uint16_t time = 0; //Duration
};


class MHandler {
  private:
    std::vector<Machine *> M;
    //Machine **M;
    int machines_col;
    bool bauto = false; //Set regime as hand worked
    int numofque = 0; //Number of work queue, 0 is for semiautomat, 1 is for fullyautomat
    String defaultJson;
    uint8_t on = 1; //define level of On 
    uint8_t off = 0; //define level of Off
    const char *iniFileName = "/machine.ini"; //Where ini file we can find
    uint8_t count_of_command = 0;
    String gcode = "";
    std::vector<COMMAND> hand_queue;
    std::vector<COMMAND> auto_queue;
    int printCnt = 0;

    
    void stopallmotors(){
      for(std::vector<Machine*>::iterator it = M.begin(); it != M.end(); ++it){
        (*it)->stopall(off);
      };
    };

    uint8_t digitizer(char from){
      return (from>47 and from<58) ? (from-48):-1;
    };

    void add_item(COMMAND tmp_command, unsigned int pre_time, unsigned int pre_val, uint8_t tmp_val){
      if(tmp_command.type ==1){
        if(pre_val==0){
          tmp_command.value = this->off;
        }else{
          tmp_command.value = this->on;
        }
        if(pre_time>0){
          tmp_command.time = pre_time;
          tmp_command.time_start = millis();
        }
      }else{
            tmp_command.value = tmp_val;
      }
      if(bauto){
        auto_queue.push_back(tmp_command);
      }else{
        Serial.print("type ");
        Serial.println(tmp_command.type);
        Serial.print("machine ");
        Serial.println(tmp_command.machine);
        Serial.print("pin ");
        Serial.println(tmp_command.pin);
        Serial.print("value ");
        Serial.println(tmp_command.value);
        Serial.print("time_start ");
        Serial.println(tmp_command.time_start);
        Serial.print("time ");
        Serial.println(tmp_command.time);
        hand_queue.push_back(tmp_command);
        Serial.println("Just append size of hand queue");
        Serial.println(hand_queue.size());
      }
    };
    
    String item_to_queue(String gcode){
      Serial.println(gcode);
      COMMAND tmp_command;
      unsigned int pre_time = 0;
      unsigned int pre_val = 0;
      uint8_t selector = 0;
      uint8_t tmp_val = 0;
      for(uint8_t i_for=0; i_for<gcode.length(); i_for++){
        char c_for = gcode.charAt(i_for);
        Serial.println(c_for);
        tmp_val = digitizer(c_for);
        Serial.println(tmp_val);
        if(tmp_val==255){
          if(c_for=='T'){
            selector = 8;
          }
          if(c_for=='M'){
            Serial.println("Инициализируем мотор");
            tmp_command.type = 1;
            selector = 1;
          };
          if(c_for=='S'){
            tmp_command.type = 2;
            selector = 2;
          };
          if(c_for=='W'){
            tmp_command.type = 4;
            selector = 4;
          };
          if(c_for=='V'){
            selector = 5;
          };
          if(c_for=='+'){
            add_item(tmp_command, pre_time, pre_val, tmp_val);
            tmp_command.type = 0;
            //tmp_command.machine = 0;
            tmp_command.pin = 0;
            tmp_command.value = 0;
            pre_time = 0;
            pre_val = 0;
          }
        }else{
          switch(selector){
            case 0:
                tmp_command.machine = tmp_command.machine*10 + tmp_val;
                break;
            case 1:
            case 2:
            case 4:
                tmp_command.pin = tmp_command.pin*10 + tmp_val;
                break;
            case 5:
                pre_val = pre_val*10 + tmp_val;
                break;
            case 8:
                pre_time = pre_time*10 + tmp_val;
                break;
          }
        }
      }
      if(tmp_command.type>0){
        add_item(tmp_command, pre_time, pre_val, tmp_val);
      }
      return "";
    };

    String next_command(){
      if(this->gcode.length()>0){
        int end_pos = 0;
        Serial.println(this->gcode);
        end_pos = this->gcode.indexOf('G', 1);
        if(end_pos>0){
          item_to_queue(this->gcode.substring(1, end_pos));
          this->gcode = this->gcode.substring(end_pos, gcode.length());
        }else{
          item_to_queue(this->gcode.substring(1, gcode.length()));
          this->gcode = "";
        }
        return "started";
      }else{
        return "";
      }
    };
  
  public:
    MHandler(){};

    void resetIni(){
      File fp = SD.open( iniFileName, "w" );
      Serial.println("create new ini");
      if(fp){
        Serial.println("file created");
        fp.write("[HEADER]\n");
        fp.write("defaultJson=/MACHINE_STRUCT.json\n");
        fp.write("onoffinvert=true\n");
        fp.close();
      }
    };

    void begin(){
      char * iniHeader = "HEADER";
      iniFile iFile;
      File fp = SD.open( iniFileName, "r" );
      Serial.println(fp);
      if(fp){
        Serial.println("ini opened");
        defaultJson = iFile.inifileString( fp, iniHeader, "defaultJson", "/MACHINE_STRUCT.json" );
        bool bOnOff= iFile.inifileBool( fp, iniHeader, "onoffinvert", false );
        if(bOnOff){
          on = 0;
          off = 1;
        };
        fp.close();
      }else{
        resetIni();
      }


      File jsonFile = SD.open(defaultJson);
      if(!jsonFile) Serial.println("Don't open file");
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, jsonFile);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      for (int i = 0; i < 4; i++) {
        M.push_back(new Machine ());
        JsonObject MACHINE_item = doc["MACHINE"][i];
        for(JsonObject MACHINE_PINS_item : MACHINE_item["PINS"].as<JsonArray>()){
          String s = MACHINE_PINS_item["PINTYPE"];
          int pin_ = MACHINE_PINS_item["HWObj"];
          Adafruit_MCP23X17 *mcp_ = &mcp[pin_];
          uint8_t adr_ = MACHINE_PINS_item["ADR"];
          if(s == "M") M[i]->add(new Motor(mcp_, adr_));
          else if(s == "S") M[i]->add(new Sensor(mcp_, adr_));  
          else if(s == "W") {
            M[i]->add(new Weigher((HardwareSerial*)&SWSerial, adr_));
          }
        }
      }
      jsonFile.close();
    };

    byte brancher(char * payload){ //this method set commands PC-ESP conversation
      if(strncmp(payload, "config_upload", 13)==0) return 1; //upload config from PC to ESP
      if(strncmp(payload, "config_download", 15)==0) return 2; //load config to PC from ESP
      if(strncmp(payload, "G", 1)==0) return 3; //gcode list
      if(strncmp(payload, "start_prg", 9)==0) return 4; //start saved program
      if(strncmp(payload, "save_prg", 8)==0) return 5; //save program to SD
      if(strncmp(payload, "del_prg", 7)==0) return 6; //delete saved program
      if(strncmp(payload, "restore_to_factory_settings[lDDQD]", 34)==0) return 7; //save blank ini file to SD card
      return 0;
    }

    String handwork(String gcode){
      printCnt = 0;
      stopallmotors();
      count_of_command = 0;
      this->gcode = gcode;
      return next_command();
    };

    String automatic(String gcode){
      return "automatic";
    };

    String config(String gcode=""){
      String result = "";
      if(gcode.isEmpty()){
        File f = SD.open(defaultJson);
        if(!f)return "Error no struct json file";
        while(f.available()){
          result += (char)f.read();
        }
        f.close();
      }
      return result;
    };

    void process(){
      if(this->bauto){
        if(this->auto_queue.size()>0){
          Serial.println(auto_queue.size());
          auto_queue.pop_back();
        }
      }else{
        if(this->hand_queue.size()>0){
          for(std::vector<COMMAND>::iterator it = this->hand_queue.begin(); it != this->hand_queue.end(); ++it){
            if(printCnt<1){Serial.println((*it).type);printCnt++;}
            switch((*it).type){
              case 1:
                if(((*it).time_start +  1000 * (*it).time)<millis() and this->hand_queue.size()<2){
                  this->M.at((*it).machine)->at((*it).pin)->set_state((*it).value);
                  Serial.println("Должно быть удаление");
                  this->hand_queue.pop_back();
                  Serial.println(this->hand_queue.size());
                  next_command();
                }
                break;
              case 2:
                if((bool)((*it).value) and this->M.at((*it).machine)->at((*it).pin)->get_state()){
                  this->hand_queue.erase(it);
                }
                break;
              case 4:
                int weight = ((Weigher *)(this->M.at((*it).machine)->at((*it).pin)))->getV();
                if(weight>=(*it).value or weight<0){
                  this->hand_queue.erase(it);
                }
                break;
            }
            if(this->hand_queue.size()==0){
              break;
            }
          }
          if(this->hand_queue.size()>2){
            Serial.println("size after check");
            Serial.println(this->hand_queue.size());
          }
        }
      }
    };
};
MHandler MH;
#endif //MHANDLER_H