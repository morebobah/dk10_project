#ifndef INIFILE_H
#define INIFILE_H
#include <FS.h>
#include <string.h>

class iniFile{
    public:
    iniFile(){}

  char *inifileString( File fp, char *header, char *detail, char *defaultData=NULL ){
      int  length, index = 0;
      unsigned int position = 0;
      char str[256], *output = NULL;
      bool hit, headerHit = false;
      if( !fp ){
          return defaultData;
      }
      fp.seek( 0 );
      #ifdef DEBUG
          Serial.print( header );  Serial.print( ":" );  Serial.print( detail );  Serial.print( ":" );
      #endif
      while ( fp.available() ){
          str[index] = fp.read();
          if( !(str[index]==0x0d || str[index]==0x0a ) ){
              if( index!=255 )index++;
              continue;
          }
          str[index] = 0x00;
          if( str[0]=='[' ){
            if( strchr( str, ']' )!=NULL ){
              headerHit = false;
              for( int i=0 ; ; i++ ){
                if( header[i]==0 ){
                  if( str[1+i]==']' )  headerHit = true;
                  break;
                  }
                  if( toupper(str[1+i])!=toupper(header[i]) )  break;
                }
              }
            }
            if( headerHit==true ){
              char *strpos = strchr( str, '=' );
              if( strpos!=NULL ){
                position = (strpos - str);
                if( position==strlen( detail ) ){
                  hit = true;
                  for( unsigned int i=0 ; i<position ; i++ ){
                    if( toupper(str[i])!=toupper(detail[i]) ){
                      hit=false;
                      break;
                    }
                  }
                  if( hit==true ){
                    length = strlen( &str[position+1] );
                    if( output!=NULL )  free( output );
                    output = (char *)malloc( length+1 );
                    memcpy( output, &str[position+1], length );
                    output[length] = 0;
                    break;
                  }
                }
              }
            }
            index = 0;
      }
      if( output==NULL ){
          output = (char*)malloc( strlen(defaultData)+1 );
          strcpy( output, defaultData );
      }
      return output;
    };
  //  return output;
  //};

  int inifileInteger( File fp, char *header, char *detail, int defaultData=-1 ){
      char *str = inifileString( fp, header, detail, NULL );
      if( str==NULL ){
          return defaultData;
      }
      int num = atoi( str );
      free( str );
      return num;
  };

  bool inifileBool( File fp, char *header, char *detail, bool defaultData=false ){
      char *str = inifileString( fp, header, detail, NULL );
      if( str==NULL ){
          return defaultData;
      }
      int num = strcasecmp_P( str, "true" );
      free( str );
      return ( (num==0)?true:false );
  };
};
#endif