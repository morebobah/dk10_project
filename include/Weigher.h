#ifndef WEIGHER_H
#define WEIGHER_H

#include <arduino.h>
#include <ModbusRTU.h>


class Weigher {
  public:
    uint8_t m_adr;
    HardwareSerial *m_serial;

    Weigher(HardwareSerial *serial_, uint8_t adr_){
      this->m_adr = adr_;
      this->m_serial = serial_;
      ModbusRTU *mb = new ModbusRTU();
      mb->begin(serial_, adr_);
    }

};


#endif // WEIGHER_H