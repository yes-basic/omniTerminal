#ifndef serialCommand_h
#define serialCommand_h


#include "Arduino.h"
#ifdef USE_BTclassic
  #include "BluetoothSerial.h"
#endif
#include "SPIFFS.h"


class serialCommand
{
  public:
    serialCommand();
    bool debug;
    bool check();
    String getCommand();
    void flush();
    void parseCommandArray(String inputCommandString);
    char commandArray[20][20];
    String commandString;
    int wordsInCommand;
    int multiComp(char command[20],char staticArray[50][20]);
    bool isValidLong(char* str);
    bool isValidHex(const char* str);
    char addonArray[20][20]={"/add"};
    char addonString[400];
    char bufString[400];
    void compileCharArray(char array[][20],int rows,char string[]);
    #ifdef USE_BTclassic
    BluetoothSerial SerialBT;
    #endif
    void noRec(const char usingIndex[20]);
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
        void write(int v);
        
      bool available();
  private:
    char inChar;
    
    
};

#endif