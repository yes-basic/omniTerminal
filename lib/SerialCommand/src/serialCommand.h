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
    String commandArray(int index);
    String commandString;
  private:
    char inChar;
    
    
};

#endif