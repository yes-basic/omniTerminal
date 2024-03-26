#ifndef serialCommand_h
#define serialCommand_h


#include "Arduino.h"
#include "BluetoothSerial.h"

class serialCommand
{
  public:
    serialCommand();
    bool debug;
    bool check();
    String getCommand();
    void flush();
    void parseCommandArray();
    char commandArray[20][20];
    String commandString;
    int wordsInCommand;
    int multiComp(char command[20],char staticArray[50][20]);
    bool isValidLong(char* str);
    bool isValidHex(const char* str);
    BluetoothSerial SerialBT;
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