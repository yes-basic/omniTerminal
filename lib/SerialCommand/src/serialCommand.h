#ifndef serialCommand_h
#define serialCommand_h

#include "Arduino.h"

class serialCommand
{
  public:
    serialCommand();
    bool check();
    String getCommand();
    int thing(int num);
    void flush();
    String commandString;
  private:
    char inChar;
    
};

#endif