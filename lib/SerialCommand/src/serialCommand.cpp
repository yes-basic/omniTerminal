#include "Arduino.h"
#include "serialCommand.h"
#include "BluetoothSerial.h"
#include "SPIFFS.h"


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
            println("");
            return true;
            commandString.trim();
        }
        //backspace charactor is 127
        if(inChar!=127){
            print(inChar);
            // add it to the commandString:
            commandString += inChar;
        }else{
            write(8);
            print(' ');
            write(8);
            commandString.remove(commandString.length()-1);
        }

        
    }
    #ifdef USE_BTclassic
      while (SerialBT.available()) {
          // get the new byte:
              inChar = (char)SerialBT.read();
    
          if(inChar=='\r'||inChar=='\n'){
              println("");
              return true;
              commandString.trim();
          }
          //backspace charactor is 127
          if(inChar!=127){
              print(inChar);
              // add it to the commandString:
              commandString += inChar;
          }else{
              write(8);
              print(' ');
              write(8);
              commandString.remove(commandString.length()-1);
          }

          
      }
    #endif
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




void serialCommand::parseCommandArray(String inputCommandString){
    int wordIndex = 0;
    int wordLength = 0;
    for (int i = 0; i < inputCommandString.length(); i++) {
        char c = inputCommandString.charAt(i);

        // If current character is not a space, add it to current word
        if (c != ' ' && c != '\0') {
            commandArray[wordIndex][wordLength++] = c;
        } else {
            commandArray[wordIndex++][wordLength] = '\0';  // Null-terminate the word
            wordLength = 0;  // Reset word length for next word
        }

        // Break if we have reached the maximum number of words
        if (wordIndex >= sizeof(commandArray)/sizeof(commandArray[0])) {
            println("command array overflow");
            break;
        }

    }
    wordsInCommand=wordIndex+1;
}
int serialCommand::multiComp(char command[20],char staticArray[50][20]){
    int staticArrayWords=50;
    for(int i=0;i<staticArrayWords;i++){
        if(!strcmp(command,staticArray[i])){return i;}
        if(debug){println(i);}
    }
    if(debug){println("nocmp");}
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
//serial replacements
  #ifdef USE_BTclassic
    bool serialCommand::available(){if(Serial.available()||SerialBT.available()){return true;}else{return false;}}

    void serialCommand::println() {Serial.println();SerialBT.println();}
    void serialCommand::print(char v[]) {Serial.print(v);SerialBT.print(v);}
    void serialCommand::println(char v[]) {Serial.println(v);SerialBT.println(v);}
    void serialCommand::print(char v) {Serial.print(v);SerialBT.print(v);}
    void serialCommand::println(char v) {Serial.println(v);SerialBT.println(v);}
    void serialCommand::print(long v) {Serial.print(v);SerialBT.print(v);}
    void serialCommand::println(long v) {Serial.println(v);SerialBT.println(v);}
    void serialCommand::print(int v) {Serial.print(v);SerialBT.print(v);}
    void serialCommand::println(int v) {Serial.println(v);SerialBT.println(v);}
    void serialCommand::print(const char v[]) {Serial.print(v);SerialBT.print(v);}
    void serialCommand::println(const char v[]) {Serial.println(v);SerialBT.println(v);}
    void serialCommand::print(String v) {Serial.print(v);SerialBT.print(v);}
    void serialCommand::println(String v) {Serial.println(v);SerialBT.println(v);}
    void serialCommand::write(int v){Serial.write(v);SerialBT.write(v);}
  #else
    bool serialCommand::available(){if(Serial.available()){return true;}else{return false;}}

    void serialCommand::println() {Serial.println();}
    void serialCommand::print(char v[]) {Serial.print(v);}
    void serialCommand::println(char v[]) {Serial.println(v);}
    void serialCommand::print(char v) {Serial.print(v);}
    void serialCommand::println(char v) {Serial.println(v);}
    void serialCommand::print(long v) {Serial.print(v);}
    void serialCommand::println(long v) {Serial.println(v);}
    void serialCommand::print(int v) {Serial.print(v);}
    void serialCommand::println(int v) {Serial.println(v);}
    void serialCommand::print(const char v[]) {Serial.print(v);}
    void serialCommand::println(const char v[]) {Serial.println(v);}
    void serialCommand::print(String v) {Serial.print(v);}
    void serialCommand::println(String v) {Serial.println(v);}
    void serialCommand::write(int v){Serial.write(v);}
  #endif

