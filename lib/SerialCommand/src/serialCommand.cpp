#include "Arduino.h"
#include "serialCommand.h"
#include <vector>
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

String serialCommand::commandArray(int index){
    int startIndex = 0;
    int spaceIndex = 0;
    int wordCount = 0;

    // Find spaces in the string and extract words
    while (spaceIndex >= 0) {
        spaceIndex = commandString.indexOf(' ', startIndex);
        if (spaceIndex >= 0) {
            wordCount++;
            if (wordCount == index) {
                return commandString.substring(startIndex, spaceIndex);
            }
            startIndex = spaceIndex + 1;  // Move to the next character after space
        }
    }
        // If n is greater than the number of words, return the last word
        if (index == wordCount + 1) {
            return commandString.substring(startIndex);
        }
    // If n is out of range, return an empty string or handle as needed
    return "";

}
