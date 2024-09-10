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
        if(serialState!=1){
          clearCMD();
          reprintCMD();
        }
      }
        if(inChar=='\r'||inChar=='\n'){
          if(serialState!=1){
            println("");
          }
          return true;
          commandString.trim();
        }
        //backspace charactor is 127
        if(inChar!=127){
          if(serialState!=1){
            serialState=2;
            bufString=userColor;
            bufString.concat(inChar);
            send(bufString);
          }
          // add it to the commandString:
          commandString += inChar;
        }else{
          if(!commandString.isEmpty()){
            if(serialState!=1){
              serialState=2;
              bufString='\b';
              send(bufString);
              serialState=2;
              bufString=' ';
              send(bufString);
              serialState=2;
              bufString='\b';
              send(bufString);
              
            }
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
    flush(false);
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
    if(debug){print("--multiComp (");print(command);println(") -----not found----");}
    return -1;
}

int serialCommand::multiComp(String command,char staticArray[50][20]){
    int staticArrayWords=50;
    for(int i=0;i<staticArrayWords;i++){
        if(!strcmp(command.c_str(),staticArray[i])){if(debug){print("--multiComp (");print(command);print(") ");print(i);println("<found");}return i;}
        if(debug){print("--multiComp (");print(command);print(") ");println(i);}
    }
    if(debug){print("--multiComp (");print(command);println(") -----not found----");}
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
    String addonBuf=userColor+addonString;
    //strcat(addonString,addonArray[0]);
  //for(int i=0;i<19;i++){
  //  if(strcmp(addonArray[i+1],"")){
  //    strcat(addonString," ");
  //    strcat(addonString,addonArray[i+1]);
  //  } 
  //}
  Serial.print(addonBuf);
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

    void serialCommand::println() {serialState=0;bufString="\n";send(bufString);}
    void serialCommand::print(char v[]) {serialState=1;bufString=baseColor;bufString.concat(v);send(bufString);}
    void serialCommand::println(char v[]) {serialState=0;bufString=baseColor;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(char v) {serialState=1;bufString=baseColor;bufString.concat(v);send(bufString);}
    void serialCommand::println(char v) {serialState=0;bufString=baseColor;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(long v) {serialState=1;bufString=baseColor;bufString.concat(v);send(bufString);}
    void serialCommand::println(long v) {serialState=0;bufString=baseColor;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(int v) {serialState=1;bufString=baseColor;bufString.concat(v);send(bufString);}
    void serialCommand::println(int v) {serialState=0;bufString=baseColor;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(const char v[]) {serialState=1;bufString=baseColor;bufString.concat(v);send(bufString);}
    void serialCommand::println(const char v[]) {serialState=0;bufString=baseColor;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(String v) {serialState=1;bufString=baseColor;bufString.concat(v);send(bufString);}
    void serialCommand::println(String v) {serialState=0;bufString=baseColor;bufString.concat(v);bufString.concat("\n");send(bufString);}


    void serialCommand::print(char v[],const char color[15]) {serialState=1;bufString=color;bufString.concat(v);send(bufString);}
    void serialCommand::println(char v[],const char color[15]) {serialState=0;bufString=color;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(char v,const char color[15]) {serialState=1;bufString=color;bufString.concat(v);send(bufString);}
    void serialCommand::println(char v,const char color[15]) {serialState=0;bufString=color;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(long v,const char color[15]) {serialState=1;bufString=color;bufString.concat(v);send(bufString);}
    void serialCommand::println(long v,const char color[15]) {serialState=0;bufString=color;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(int v,const char color[15]) {serialState=1;bufString=color;bufString.concat(v);send(bufString);}
    void serialCommand::println(int v,const char color[15]) {serialState=0;bufString=color;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(const char v[],const char color[15]) {serialState=1;bufString=color;bufString.concat(v);send(bufString);}
    void serialCommand::println(const char v[],const char color[15]) {serialState=0;bufString=color;bufString.concat(v);bufString.concat("\n");send(bufString);}
    void serialCommand::print(String v,const char color[15]) {serialState=1;bufString=color;bufString.concat(v);send(bufString);}
    void serialCommand::println(String v,const char color[15]) {serialState=0;bufString=color;bufString.concat(v);bufString.concat("\n");send(bufString);}

    void serialCommand::printFree(){serialState=0;bufString="";send(bufString);}


  void serialCommand::send(String data){
    if(previousSerialState==0&&serialState!=2){clearCMD();}
    Serial.print(data);
    if(serialState!=2){
      sendString=sendString+data;
      if(serialState==0){
        for (const auto& listener : listenerFunctionRegistry) {
            listener.function(sendString);
        }
        sendString.clear();
      }
    }
    
    if(serialState==0){
      reprintCMD();
    }
    if(serialState==1||serialState==0){
      previousSerialState=serialState;
    }
    
  }

  void serialCommand::registerListenerFunction(const String& name, listenerFunctionPtr function) {
    // Iterate through the vector to find the item with the matching name
    for (auto it = listenerFunctionRegistry.begin(); it != listenerFunctionRegistry.end(); ++it) {
        if (it->name == name) {
            if (function == nullptr) {
                // If newFunc is nullptr, remove the element from the vector
                listenerFunctionRegistry.erase(it);
            } else {
                // If the string exists and newFunc is valid, replace the function
                it->function = function;
            }
            return;
        }
    }

    // If the string doesn't exist and newFunc is not nullptr, add a new entry
    if (function != nullptr) {
        listenerFunctionRegistry.push_back({name, function});
    }
}


