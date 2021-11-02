#ifndef WEIGHER_H
#define WEIGHER_H

#include <arduino.h>
//#include <ModbusRTU.h>


class Weigher : public Pin {
  public:
    uint8_t m_adr;
    HardwareSerial *m_serial;

    Weigher(HardwareSerial *serial_, uint8_t adr_) : Pin(0,0){
      this->m_adr = adr_;
      this->m_serial = serial_;
      //ModbusRTU *mb = new ModbusRTU();
      //mb->begin(serial_, adr_);*/
      #ifdef M_DEBUG
        Serial.printf("Create weigher Serial adress %d\n", adr_);
      #endif //DEBUG
    };
    int get_value(){
      return 0;
    };
};

#endif // WEIGHER_H