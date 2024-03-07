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
        commandString.trim();
    }

    while (Serial.available()) {
        // get the new byte:
            inChar = (char)Serial.read();
  
        if(inChar=='\r'||inChar=='\n'){
            Serial.println("");
            return true;
            commandString.trim();
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
int serialCommand::multiComp(char command[20],char staticArray[50][20]){
    int staticArrayWords=50;
    for(int i=0;i<staticArrayWords;i++){
        if(!strcmp(command,staticArray[i])){return i;}
        if(debug){Serial.println(i);}
    }
    if(debug){Serial.println("nocmp");}
    return -1;
}

bool serialCommand::isValidLong(char* str) {
  // Check if the string is empty
  if (strlen(str) == 0) {
    return false;
  }

  // Check each character to ensure it's a digit or a valid sign
  for (int i = 0; i < strlen(str); i++) {
    if (!isdigit(str[i]) && str[i] != '-' && str[i] != '+') {
      return false;
    }
  }

  // Check if the string is only a sign without any digits
  if (strlen(str) == 1 && (str[0] == '-' || str[0] == '+')) {
    return false;
  }

  // Check if the string exceeds the limits of a long
  if (strlen(str) > 11) {  // 11 because of sign and 10 digits for long
    return false;
  }

  // Use strtol to further validate
  char* endPtr;
  long value = strtol(str, &endPtr, 10);

  // Check if strtol successfully converted the string
  // Also check if there are any trailing characters that strtol ignored
  for (int i = 0; i < strlen(endPtr); i++) {
    if (!isspace(endPtr[i]) && endPtr[i] != '\0') {
      return false;
    }
  }
return true;
}

bool serialCommand::isValidHex(const char* str) {
  // Check if the string is empty
  if (strlen(str) == 0) {
    return false;
  }

  // Check each character to ensure it's a valid hexadecimal character
  for (int i = 0; i < strlen(str); i++) {
    if (!isxdigit(str[i])) {
      return false;
    }
  }

  return true;
}
