#ifndef serialCommand_h
#define serialCommand_h

#include "Arduino.h"
#include <vector>
class serialCommand
{
  public:
    serialCommand();
    bool check();
    String getCommand();
    void flush();
    void parseCommandArray();
    char commandArray[20][20];
    String commandString;
    int wordsInCommand;
  private:
    char inChar;
    
    
};

#endif