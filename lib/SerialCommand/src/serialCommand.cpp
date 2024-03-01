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
  
        if(inChar=='\r'||inChar=='\n'){
            Serial.println("");
            return true;
        }
        //backspace charactor is 127
        if(inChar!=127){
            Serial.print(inChar);
            // add it to the commandString:
            commandString += inChar;
        }else{
            Serial.write(8);
            Serial.print(' ');
            Serial.write(8);
            commandString.remove(commandString.length()-1);
        }

    //commandString.trim();
    }
return false;
}

void serialCommand::flush(){
inChar=0;
commandString="";
}



int serialCommand::thing(int num){

}
