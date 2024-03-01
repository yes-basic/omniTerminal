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
    for (int i = 0; i <sizeof(commandArray)/sizeof(commandArray[0]); i++) {
        for (int j = 0; j < sizeof(commandArray[0]); j++) {
            commandArray[i][j] = '\0';  // Set each character to null
        }
    }
}

void serialCommand::parseCommandArray(){
    int wordIndex = 0;
    int wordLength = 0;
    for (int i = 0; i < commandString.length(); i++) {
        char c = commandString.charAt(i);

        // If current character is not a space, add it to current word
        if (c != ' ' && c != '\0') {
            commandArray[wordIndex][wordLength++] = c;
        } else {
            commandArray[wordIndex++][wordLength] = '\0';  // Null-terminate the word
            wordLength = 0;  // Reset word length for next word
        }

        // Break if we have reached the maximum number of words
        if (wordIndex >= sizeof(commandArray)/sizeof(commandArray[0])) {
            Serial.println("command array overflow");
            break;
        }

    }
    wordsInCommand=wordIndex+1;
}
