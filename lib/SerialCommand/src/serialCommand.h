#ifndef serialCommand_h
#define serialCommand_h


#include "Arduino.h"
#include "BluetoothSerial.h"
#include <vector>
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
  private:
    char inChar;
    
    
};

#endif