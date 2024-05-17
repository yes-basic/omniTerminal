#ifndef serialCommand_h
#define serialCommand_h


#include "Arduino.h"
#ifdef USE_BTclassic
  #include "BluetoothSerial.h"
#endif
#include "SPIFFS.h"
#include <omniCommand.h>


class serialCommand
{
  public:
    serialCommand();
    void tryEspnowSend(char packet[200]);
    bool debug;
    bool check();
    String getCommand();
    void flush(bool flushAll);
    void parseCommandArray(String inputCommandString,bool addons);
    char commandArray[20][20];
    String commandString;
    int wordsInCommand;
    int multiComp(char command[20],char staticArray[50][20]);
    bool isValidLong(char* str);
    bool isValidHex(const char* str);
    omniCommand addons;
    char bufString[400];
    void compileCharArray(char array[][20],int rows,char *string[]);
    #ifdef USE_BTclassic
    BluetoothSerial SerialBT;
    #endif
    void noRec(const char usingIndex[20]);
    void send(char data[200]);
    void clearCMD();
    void reprintCMD();
    //serial replacements
      //printing overloads
        void print();void println();
        void print(int v);void println(int v);
        void print(char v[]);void println(char v[]);
        void print(char v);void println(char v);
        void print(const char v[]);void println(const char v[]);
        void print(long v);void println(long v);
        void print(String v);void println(String v);

        void print(int v,const char color[15]);void println(int v,const char color[15]);
        void print(char v[],const char color[15]);void println(char v[],const char color[15]);
        void print(char v,const char color[15]);void println(char v,const char color[15]);
        void print(const char v[],const char color[15]);void println(const char v[],const char color[15]);
        void print(long v,const char color[15]);void println(long v,const char color[15]);
        void print(String v,const char color[15]);void println(String v,const char color[15]);

        char baseColor[15]="\033[37m";
        char userColor[15]="\033[36m";
        void write(int v);
        typedef void (*sendFunctionPtr)(char command[200],int msgID);
        sendFunctionPtr sendFunction=nullptr;
        void registerSendFunction(sendFunctionPtr function);
        void unregisterSendFunction();
        
      bool available();
  private:
    char inChar;
    
    
};

#endif