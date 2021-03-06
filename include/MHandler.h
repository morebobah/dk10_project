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
#include <WebSocketsServer.h> 

extern HardwareSerial Serial;
extern Adafruit_MCP23X17 mcp[4];
//extern File jsonFile;
extern SoftwareSerial SWSerial;

enum MCHN_STATUS{ stoped = 0, paused, inprocess, error };

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
    File file_automatic; //pointer to current programm's file
    uint64_t time_alarm = 0; //Alarm will stop all machines for 5 minutes
    MCHN_STATUS status = MCHN_STATUS::stoped; //machine status
    String prg_id = "";
    
    void stopallmotors(){
      for(std::vector<Machine*>::iterator it = M.begin(); it != M.end(); ++it){
        (*it)->stopall(off);
      };
    };

    uint8_t digitizer(char from){
      return (from>47 and from<58) ? (from-48):255;
    };

    void add_item(COMMAND tmp_command, unsigned int pre_time, unsigned int pre_val){
      if(tmp_command.type == 0)return;
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
            tmp_command.value = pre_val;
      }
      if(bauto){
        auto_queue.push_back(tmp_command);
      }else{
        /*
        Serial.print("MH: type ");
        Serial.println(tmp_command.type);
        Serial.print("MH: machine ");
        Serial.println(tmp_command.machine);
        Serial.print("MH: pin ");
        Serial.println(tmp_command.pin);
        Serial.print("MH: value ");
        Serial.println(tmp_command.value);
        Serial.print("MH: time_start ");
        Serial.println(tmp_command.time_start);
        Serial.print("MH: time ");
        Serial.println(tmp_command.time);
        */
        hand_queue.push_back(tmp_command);
        /*
        Serial.print("MH: New size of hand queue ");
        Serial.println(hand_queue.size());
        */
      }
    };
    
    String item_to_queue(String gcode){
      #ifdef MH_DEBUG
        Serial.print("MH: clear code=\"");
        Serial.print(gcode);
        Serial.println("\"");
      #endif
      COMMAND tmp_command;
      unsigned int pre_time = 0;
      unsigned int pre_val = 0;
      uint8_t selector = 0;
      uint8_t tmp_val = 0;
      bool b_new_machine = false;
      for(uint8_t i_for=0; i_for<gcode.length(); i_for++){
        char c_for = gcode.charAt(i_for);
        tmp_val = digitizer(c_for);
        if(tmp_val==255){
          if(c_for=='T'){
            selector = 8;
          }
          if(c_for=='M'){
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
            add_item(tmp_command, pre_time, pre_val);
            tmp_command.type = 0;
            tmp_command.pin = 0;
            tmp_command.value = 0;
            pre_val = 0;
            selector = 0;
            b_new_machine = true;
          }
        }else{
          switch(selector){
            case 0:
              if(b_new_machine){
                tmp_command.machine = 0;
              }
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
          b_new_machine = false;
        }
      }
      if(tmp_command.type>0){
        add_item(tmp_command, pre_time, pre_val);
      }
      return "";
    };

    bool next_command(){
      if(this->status==MCHN_STATUS::paused)return this->sendAnswer(this->get_status());
      if(this->bauto){
          if(this->file_automatic.available()){
            String agcode = this->file_automatic.readStringUntil('G');
            #ifdef MH_DEBUG
              Serial.println(agcode);
            #endif
            item_to_queue(agcode);
            return true;
          }else{
            this->status = MCHN_STATUS::stoped;
            this->sendAnswer(this->get_status());
            #ifdef MH_DEBUG
              Serial.println("MH: program finished");
            #endif
            this->status=MCHN_STATUS::stoped;
            this->prg_id = "";
            return true;
          }
      }else{
        if(this->gcode.length()>0){
          int end_pos = 0;
          #ifdef MH_DEBUG
            Serial.print("MH: gcode=\"");
            Serial.print(this->gcode);
            Serial.println("\"");
          #endif
          end_pos = this->gcode.indexOf('G', 1);
          if(end_pos>0){
            item_to_queue(this->gcode.substring(1, end_pos));
            this->gcode = this->gcode.substring(end_pos, gcode.length());
          }else{
            item_to_queue(this->gcode.substring(1, gcode.length()));
            this->gcode = "";
          }
          return true;
        }else{
          //this->sendAnswer(this->get_status());
          #ifdef MH_DEBUG
            Serial.println("MH: command finished");
          #endif
          return true;
        }
      }
    };

    bool sendAnswer(String upload){
      return webSocket.broadcastTXT(upload);
    }

    String get_status(){
      String result = "MSW";
      DynamicJsonDocument doc(10000);
      doc["status"] = this->status;
      doc["prg_id"] = this->prg_id;
      doc["default_on"] = this->on;
      for(std::vector<Machine*>::iterator it = M.begin(); it != M.end(); ++it){
        uint8_t mchnum = (*it)->get_machinenum();
        doc["state"][mchnum]["machine"] = mchnum;
        std::vector<Pin *>  P = (*it)->pins();
        for(std::vector<Pin*>::iterator pit = P.begin(); pit != P.end(); ++pit){
          uint8_t tpin = (*pit)->get_type();
          String key = result.substring(tpin, tpin + 1) + String((*pit)->get_pin());
          if(tpin==WEIGHER){
            doc["state"][mchnum]["pins"][key] = ((Weigher *)(*pit))->getW();
          }else{
            doc["state"][mchnum]["pins"][key] = (*pit)->get_state();
          }
        }
      };
      result = "";
      serializeJson(doc, result);
      #ifdef MH_DEBUG 
        Serial.println(result);
      #endif
      return result;
    };

    uint16_t validprg(String s){
      uint16_t result = 0;
      uint8_t tmp_val = 255;
      if(s.indexOf("prg[")!=0)return 0;
      uint8_t pos = 4;
      do{
        char from = s.charAt(pos);
        tmp_val = digitizer(from);
        if(tmp_val<10)result = 10 * result + tmp_val;
        pos++;
      }while(tmp_val!=255);
      return result;
    };
    
    void inprogress(std::vector<COMMAND> &current_queue){
      String result;
      String key;
      bool bSend = false;
      std::vector<COMMAND>::iterator M_it;
      std::vector<COMMAND>::iterator S_it;
      std::vector<COMMAND>::iterator W_it;
      DynamicJsonDocument doc(10000);
      for(std::vector<COMMAND>::iterator it = current_queue.begin(); it != current_queue.end(); it++){
        doc["state"][0]["machine"] = (*it).machine;
        switch((*it).type){
          case 1:
          if(((*it).time_start +  1000 * (*it).time)<millis() and current_queue.size()<2){
            this->M.at((*it).machine)->at((*it).pin)->set_state((*it).value);
            key = "M" + String(this->M.at((*it).machine)->at((*it).pin)->get_pin());
            doc["state"][0]["pins"][key] = this->M.at((*it).machine)->at((*it).pin)->get_state();
            bSend = true;
            current_queue.pop_back();
          }
          break;
          case 2:
            if((bool)((*it).value) == this->M.at((*it).machine)->at((*it).pin)->get_state()){
              key = "S" + String(this->M.at((*it).machine)->at((*it).pin)->get_pin());
              doc["state"][0]["pins"][key] = this->M.at((*it).machine)->at((*it).pin)->get_state();
              bSend = true;
              current_queue.erase(it);
            }
            break;
          case 4:
            int weight = ((Weigher *)(this->M.at((*it).machine)->at((*it).pin)))->getW();
            key = "W" + String(this->M.at((*it).machine)->at((*it).pin)->get_pin());
            doc["state"][0]["pins"][key] = weight;
            bSend = true;
            if(weight>=(*it).value){
              current_queue.erase(it);
            }
            break;
        }
        if(bSend){
          doc["status"] = this->status;
          doc["prg_id"] = this->prg_id;
          serializeJson(doc, result);
          sendAnswer(result);
        }
        bSend = false;
        if(current_queue.size()==0){
          break;
        }
      }
      if(current_queue.size()==0){
          this->next_command();
      }
    };
  
  public:
    boolean bLCD = false;
    WebSocketsServer webSocket = WebSocketsServer(81);
    MHandler(){};

    void resetIni(){
      File fp = SD.open( iniFileName, "w" );
      #ifdef MH_DEBUG
        Serial.println("MH: create new ini");
      #endif
      if(fp){
        #ifdef MH_DEBUG
          Serial.println("MH: file created");
        #endif
        fp.write("[HEADER]\n");
        fp.write("defaultJson=/MACHINE_STRUCT.json\n");
        fp.write("onoffinvert=true\n");
        fp.close();
      }
    };

    void begin(){
      String iniHeader("HEADER");
      iniFile iFile;
      File fp = SD.open( iniFileName, "r" );
      #ifdef MH_DEBUG
        Serial.println(fp);
      #endif
      if(fp){
        #ifdef MH_DEBUG
          Serial.println("MH: ini opened");
        #endif
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
      #ifdef MH_DEBUG
        if(!jsonFile) Serial.println("MH: Don't open file");
      #endif
      DynamicJsonDocument doc(7000);
      DeserializationError error = deserializeJson(doc, jsonFile);
      if (error) {
        #ifdef MH_DEBUG
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        #endif
        return;
      }
      for (uint8_t i = 0; i < 4; i++) {
        M.push_back(new Machine (i));
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

      //load config to PC from ESP "config_download"
      if(strncmp(payload, "config_download", 15)==0){
        if(this->sendAnswer(this->config())) return 0;
        return 2;
      }

      //gcode list
      if(strncmp(payload, "G", 1)==0){
        this->handwork((char*)payload);
        return 0;
      }

      //start saved program
      if(strncmp(payload, "start_prg", 9)==0){
        if(this->status!=MCHN_STATUS::stoped) return 4;
        this->automatic((char*)payload);
        return 0; 
      }

      if(strncmp(payload, "save_prg", 8)==0){ //save program to SD
        if(this->newprg((char*)payload))
          this->sendAnswer(this->listofprg((char*)payload));
      }

      if(strncmp(payload, "load_prg", 8)==0){ //load program and send to client
        this->downloadprg((char*)payload);
        #ifdef MH_DEBUG
          Serial.println("MH: download prg");
        #endif
      }

      if(strncmp(payload, "del_prg", 7)==0) return 6; //delete saved program
      if(strncmp(payload, "restore_to_factory_settings[lDDQD]", 34)==0) return 7; //save blank ini file to SD card

      //get list of saved prog
      if(strncmp(payload, "get_list_prg", 12)==0){
        this->sendAnswer(this->listofprg((char*)payload));
        this->sendAnswer(this->get_status());
        return 8; 
      }

      if(strncmp(payload, "alarm", 5)==0) {
        stopallmotors(); 
        this->status = MCHN_STATUS::stoped;
        bauto = false;
        this->auto_queue.clear();
        this->hand_queue.clear();
        this->sendAnswer(this->get_status());
      } //stop all motors for 5 minutes

      //pause current program
      if(strncmp(payload, "pause_prg", 9)==0){ 
        if(this->status==MCHN_STATUS::inprocess) this->status=MCHN_STATUS::paused;
        this->sendAnswer(this->get_status());
        return 9;
      }

      //stop current program
      if(strncmp(payload, "stop_prg", 8)==0){ 
        if(this->status==MCHN_STATUS::inprocess || this->status==MCHN_STATUS::paused){
          this->file_automatic.close();
          this->status=MCHN_STATUS::stoped;
          this->sendAnswer(this->get_status());
        }
        return 10; 
      }

      if(strncmp(payload, "continue_prg", 12)==0){
        this->status=MCHN_STATUS::inprocess;
        this->next_command();
        return 12;
      }

      //read status
      if(strncmp(payload, "status", 6)==0){
        this->sendAnswer(this->get_status());
        return 13;
      }
      return 0;
    };

    bool handwork(String gcode){
      if(this->status==MCHN_STATUS::inprocess) return false;
      if(this->bauto){
        this->bauto = false;
      }
      printCnt = 0;
      count_of_command = 0;
      this->gcode = gcode;
      return this->next_command();
    };

    bool downloadprg(String gcode){
      DynamicJsonDocument doc(7000);
      String prgnum = gcode.substring(8);
      if(prgnum.toInt()==0) return false;
      String result = "";
      String prgpath = "/prg[" + prgnum +"].json";
      String prgname = "";
      String prglist = "";
      bool bName = true;
      File f = SD.open(prgpath);
      if(!f)return false;
      while(f.available()){
        char current = (char)f.read();
        if(current=='\n'){bName = false;
        }else{
          if(current!='\r'){
            if(bName){
              prgname += current;
            }else{
              prglist += current;
            }
          }
        }
      }
      f.close();
      doc["downloadprg"]["name"] = prgname;
      doc["downloadprg"]["listing"] = prglist;
      serializeJson(doc, result);
      this->sendAnswer(result);
      return true;
    };

    bool firmware(String gcode){
      return true;
    }

    bool newprg(String gcode){
      int endofid = gcode.indexOf('\n');
      int endofname = gcode.indexOf('\n', endofid + 1);
      String prgnum = gcode.substring(8, endofid);
      String prgname = gcode.substring(endofid + 1, endofname);
      String prg_value = gcode.substring(endofname + 1);
      if(prgnum.toInt()==0) return false;
      String prgpath = "/prg[" + prgnum +"].json";
      #ifdef MH_DEBUG
        Serial.println(prgpath);
      #endif
      SD.remove(prgpath);
      File f = SD.open(prgpath, FILE_WRITE);
      f.println(prgname);
      f.println(prg_value);
      f.close();
     return true;
    };

    bool automatic(String gcode){
      String prgnum = gcode.substring(9);
      if(prgnum.toInt()==0) return false;
      String prgpath = "/prg[" + prgnum +"].json";
      #ifdef MH_DEBUG
        Serial.println(prgnum);
      #endif
      this->file_automatic = SD.open(prgpath);
      if(!this->file_automatic.available()) return false;
      this->prg_id = prgnum;
      this->stopallmotors();
      this->file_automatic.readStringUntil('G');
      this->bauto = true;
      this->status = MCHN_STATUS::inprocess;
      this->sendAnswer(this->get_status());
      return this->next_command();
    };

    String config(String gcode=""){
      String result = "";
      if(gcode.isEmpty()){
        File f = SD.open(defaultJson);
        Serial.println(defaultJson);
        if(!f)return "MH: Error no struct json file";
        while(f.available()){
          result += (char)f.read();
        }
        f.close();
      }
      return result;
    };

    String listofprg(String gcode=""){
      String result;
      DynamicJsonDocument doc(4096);
      int16_t numprg = 0;
      int16_t cntprg = 0;
      String json;
      char cOne = '\0';
      File root = SD.open("/");
      File dir = root.openNextFile();
      while(dir){
        String line = "";
        numprg = validprg(dir.name());
        #ifdef MH_DEBUG
          //Serial.println(dir.name());
          //Serial.println(numprg);
        #endif
        if(numprg>0){
          if(!dir.isDirectory()){
            while(dir.available()){
              cOne = (char)dir.read();
              if(cOne=='\r' or cOne=='\n' or cOne=='G') break;
              line += cOne;
            }
          }
          //result += line;
          doc["program"][cntprg]["name"] = line;
          doc["program"][cntprg]["ID"] = numprg;
          cntprg++;
        }
        dir = root.openNextFile();
      }
      dir.close();
      root.close();
      #ifdef MH_DEBUG
        //Serial.print("MH: ");
        //Serial.print(result);
      #endif
      serializeJson(doc, result);
      return result;
    };

    void process(){
      this->webSocket.loop();
      //if(time_alarm>0){
      //  if((millis() - time_alarm)<300000){
      //      return;
      //    }
      //}
      if(this->bauto){
        if(this->auto_queue.size()>0){
          //Serial.println(auto_queue.size());
          this->inprogress(this->auto_queue);
        }
      }else{
        if(this->hand_queue.size()>0){
          this->inprogress(this->hand_queue);
        }
      }
    };
};
MHandler MH;
#endif //MHANDLER_H