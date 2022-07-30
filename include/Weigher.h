#ifndef WEIGHER_H
#define WEIGHER_H

#include <arduino.h>
#include "Pin.h"

struct request{
  uint8_t MbAddr;
  uint8_t command;
  uint8_t AddrH;
  uint8_t AddrL;
  uint8_t QntH;
  uint8_t QntL;
  uint16_t CRC;
};

struct response{
  uint8_t MbAddr;
  uint8_t command;
  uint8_t ByteCnt;
  uint8_t ValueH;
  uint8_t ValueL;
  uint16_t CRC;
};

class Weigher : public Pin {
  private:
   int V0;
  public:
    uint8_t m_adr;
    HardwareSerial *m_serial;

    Weigher(HardwareSerial *serial_, uint8_t adr_) : Pin(0, adr_, WEIGHER){
      this->m_adr = adr_;
      this->m_serial = serial_;
      this->V0 = 0;
      for(int i = 0; i<10; i++){
        int v = this->getV();
        if(this->V0<v && v>0) this->V0 = v;
      }

      #ifdef WEI_DEBUG
        Serial.printf("Weigher: Create weigher Serial adress %d\n", adr_);
      #endif //DEBUG
    };
    
    int getV(){
      request buf;
      response res;
      buf.MbAddr = m_adr;
      buf.command = 3;
      buf.AddrH = 0;
      buf.AddrL = 0;
      buf.QntH = 0;
      buf.QntL = 1;
      buf.CRC = CRC16_2((byte*)&buf, sizeof(buf) - sizeof(uint16_t));

      m_serial->write((byte*)&buf, sizeof(buf));
      

      if(m_serial->readBytes((byte*)&res, sizeof(res))){
        // if(m_serial->available() > 0){
        //   m_serial->flush();
        // }
        //uint16_t crc = CRC16_2((byte*)&res, sizeof(res));
        #ifdef WEI_DEBUG
          // Serial.printf("Adr %d\n", res.MbAddr);
          // Serial.printf("Comand %d\n", res.command);
          // Serial.printf("ByteCnt %d\n", res.ByteCnt);
          // Serial.printf("ValueH %d\n", res.ValueH);
          // Serial.printf("ValueL %d\n", res.ValueL);
          // Serial.printf("CRC recived %d\n", res.CRC);
          // Serial.printf("CRC calculated %d\n", crc);
        #endif

        //if (crc == 0){
          return (res.ValueH << 8) + res.ValueL;
        //}
      }
      return -1;
    };

    int getW(){
      int v = this->getV();
      return (v-this->V0)*4;
    };

    uint16_t CRC16_2(unsigned char *buf, int len){  
      uint16_t crc = 0xFFFF;
      for (int pos = 0; pos < len; pos++)
      {
      crc ^= (uint16_t)buf[pos];    // XOR byte into least sig. byte of crc

      for (int8_t i = 8; i != 0; i--) {    // Loop over each bit
        if ((crc & 0x0001) != 0) {      // If the LSB is set
          crc >>= 1;                    // Shift right and XOR 0xA001
          crc ^= 0xA001;
        }
        else                            // Else LSB is not set
          crc >>= 1;                    // Just shift right
        }
      }

      return crc;
    };
   
};

#endif // WEIGHER_H