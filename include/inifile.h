#ifndef INIFILE_H
#define INIFILE_H
#include <FS.h>
#include <string.h>

class iniFile{
  private:
    uint32_t srch(File fp, String substr, String restricted = ""){
      uint32_t index = 0;
      uint8_t idx = 0;
      bool b_stop = false;
      while ( fp.available() ){
        char s = fp.read();
        if(s==' ')continue;
        for(uint8_t i_for=0; i_for<restricted.length();i_for++){
          if(s==restricted.charAt(i_for)){
            b_stop = true;
            idx = 0;
            break;
          }
        }
        if(b_stop) break;
        if(s==substr.charAt(idx)){
          idx++;
          if(idx==substr.length()){
            break;
          }
        }else{
          idx = 0;
        }
        index++;
      }
      return (idx>0)? index + 1: 0;
    };

    String value(File fp, int32_t pos, String substr, String val=""){
      String v="";
      if(pos==0) return val;
      fp.seek(pos);
      uint32_t p = this->srch(fp, substr+"=", "[");
      if (p==0) return val;
      pos += p;
      fp.seek(pos);
      while ( fp.available() ){
        char s = fp.read();
        if( s=='\n' || s=='\r' || s=='[' ) break;
        v += s;
      }
      return (v=="")?val: v;
    };

  public:
    iniFile(){}
    String inifileString( File fp, String header, String detail, String defaultData="" ){
      fp.seek(0);
      return this->value(fp, this->srch(fp, "["+header+"]"), detail, defaultData);
    };

    long inifileInteger( File fp, String header, String detail, int defaultData=-1 ){
      String intVal = inifileString(fp, header, detail, "");
      return (intVal=="")? defaultData: intVal.toInt();
    };
    
    bool inifileBool( File fp, String header, String detail, bool defaultData=false ){
      String boolVal = inifileString(fp, header, detail, "");
      if(boolVal=="")return defaultData;
      boolVal.toUpperCase();
      return (boolVal=="TRUE");
    };
};
#endif