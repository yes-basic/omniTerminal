#include "Arduino.h"
#include "serialCommand.h"

serialCommand::serialCommand()
{
commandString.reserve(200);
}
bool serialCommand::check() {
if(inChar=='\r'||inChar=='\n'){
    return true;
}
 while (Serial.available()) {
    // get the new byte:
    inChar = (char)Serial.read();
    Serial.print(inChar);
    if(inChar=='\r'||inChar=='\n'){
        Serial.println("");
        return true;
    }
    // add it to the commandString:
    commandString += inChar;
    commandString.trim();
}
return false;
}

void serialCommand::flush(){
inChar=0;
commandString="";
}



int serialCommand::thing(int num){

}
