#include "Arduino.h"
#include "serialCommand.h"
#include "BluetoothSerial.h"
#include "SPIFFS.h"
#include <esp_now.h>
#include <WiFi.h>

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
      if(commandString.isEmpty()){
        clearCMD();
        reprintCMD();
      }
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
          if(!commandString.isEmpty()){
            print('\b');
            print(' ');
            print('\b');
            commandString.remove(commandString.length()-1);
          }
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
              print('\b');
              print(' ');
              print('\b');
              commandString.remove(commandString.length()-1);
          }

          
      }
    #endif
    return false;
}

void serialCommand::flush(bool flushAll){
    if(flushAll){
      inChar=0;
      commandString="";
    } 
    for (int i = 0; i <sizeof(commandArray)/sizeof(commandArray[0]); i++) {
        for (int j = 0; j < sizeof(commandArray[0]); j++) {
            commandArray[i][j] = '\0';  // Set each character to null
        }
    }
}


void serialCommand::parseCommandArray(String inputCommandString,bool addons){
    int wordIndex = 0;
    int wordLength = 0;
    if(addons){
      while(strcmp(addonArray[wordIndex],"")){
        strcpy(commandArray[wordIndex],addonArray[wordIndex]);
        wordIndex++;
      }
    }

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
        if(!strcmp(command,staticArray[i])){if(debug){print("--multiComp (");print(command);print(") ");print(i);println("<found");}return i;}
        if(debug){print("--multiComp (");print(command);print(") ");println(i);}
    }
    if(debug){print("--multiComp (");print(command);print(") -----not found----");}
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

void serialCommand::noRec(const char usingIndex[20]){
  print("no such command in index: ");
  println(usingIndex);
}

void serialCommand::clearCMD(){
  Serial.print("\033[2K"); // Clear the line
  Serial.print("\r"); 
  #ifdef USE_BTclassic
  SerialBT.print("\033[2K"); // Clear the line
  SerialBT.print("\r"); 
  #endif
}
void serialCommand::reprintCMD(){
    strcpy(addonString,addonArray[0]);
  for(int i=0;i<19;i++){
    if(strcmp(addonArray[i+1],"")){
      strcat(addonString," ");
      strcat(addonString,addonArray[i+1]);
    } 
  }
  Serial.print(addonString);
  Serial.print(">>");
  Serial.print(commandString);
  #ifdef USE_BTclassic
  SerialBT.print(addonString);
  SerialBT.print(">>");
  SerialBT.print(commandString);
  #endif
}


//serial replacements
    bool serialCommand::available(){if(Serial.available()){return true;}else{return false;}}

    void serialCommand::println() {snprintf(bufString,198,"\n");send(bufString);}
    void serialCommand::print(char v[]) {snprintf(bufString,198,"%s",v);send(bufString);}
    void serialCommand::println(char v[]) {snprintf(bufString,198,"%s\n",v);send(bufString);}
    void serialCommand::print(char v) {snprintf(bufString,198,"%c",v);send(bufString);}
    void serialCommand::println(char v) {snprintf(bufString,198,"%c\n",v);send(bufString);}
    void serialCommand::print(long v) {snprintf(bufString,198,"%d",v);send(bufString);}
    void serialCommand::println(long v) {snprintf(bufString,198,"%d\n",v);send(bufString);}
    void serialCommand::print(int v) {snprintf(bufString,198,"%d",v);send(bufString);}
    void serialCommand::println(int v) {snprintf(bufString,198,"%d\n",v);send(bufString);}
    void serialCommand::print(const char v[]) {snprintf(bufString,198,"%s",v);send(bufString);}
    void serialCommand::println(const char v[]) {snprintf(bufString,198,"%s\n",v);send(bufString);}
    void serialCommand::print(String v) {v.toCharArray(bufString,198);send(bufString);}
    void serialCommand::println(String v) {v.toCharArray(bufString,198);snprintf(bufString,198,"%s\n",bufString);send(bufString);}


  void serialCommand::tryEspnowSend(char packet[200]){
    
  }
  void serialCommand::send(char data[200]){
    Serial.print(data);
    if(sendFunction!= nullptr){
      sendFunction(data,0);
    }
  }

  void serialCommand::registerSendFunction(sendFunctionPtr function){
    sendFunction=function;
  }
  void serialCommand::unregisterSendFunction(){
    sendFunction=nullptr;
  }

