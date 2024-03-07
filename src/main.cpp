#include <Arduino.h>
#include <serialCommand.h>

#include <TFT_eSPI.h>
#define debug inCom.debug
void refreshTFT();
void identifyCommand();
String serialcommand(bool flush);
bool isValidHex(const char* str);

#define DISABLE_CODE_FOR_RECEIVER // Disables restarting receiver after each send. Saves 450 bytes program memory and 269 bytes RAM if receiving functions are not used.
//#define SEND_PWM_BY_TIMER         // Disable carrier PWM generation in software and use (restricted) hardware PWM.
//#define USE_NO_SEND_PWM           // Use no carrier PWM, just simulate an active low receiver signal. Overrides SEND_PWM_BY_TIMER definition

/*
 * This include defines the actual pin number for pins like IR_RECEIVE_PIN, IR_SEND_PIN for many different boards and architectures
 */
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp> // include the library

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

serialCommand inCom;
//init misc var
  
  long millisLastRefresh;
  int wordReturn=1;
  char wordBuffer[30];
  int commandInt;
//init command array
  char commandIndex[50][20]={
    "/help",
    "/debug",
    "/add",
    "/IR"

  };
  int commandIndexWords=sizeof(commandIndex)/sizeof(commandIndex[0]);
void setup() {
  //init serial
    Serial.begin(115200);
  //init TFT
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    img.createSprite(240, 135);
    img.fillSprite(TFT_BLACK);

    IrSender.begin(DISABLE_LED_FEEDBACK); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin
}


void loop() {
  //check command
    if(inCom.check()){
      if(debug){Serial.println(inCom.commandString);}
      inCom.parseCommandArray();
      if(debug){
        for (int i = 0; i < inCom.wordsInCommand; i++) {
          Serial.print("Word ");
          Serial.print(i);
          Serial.print(": ");
          Serial.print("\"");
          Serial.print(inCom.commandArray[i]);
          Serial.println("\"");
        }
      }
      identifyCommand();
      inCom.flush();
    }
  refreshTFT();
}

void identifyCommand(){
  //find the command
    commandInt=inCom.multiComp(inCom.commandArray[0],commandIndex);
    if(debug){Serial.println(commandInt);}
  //do the command
    switch(commandInt){
      //not recognized
        case -1:
          Serial.println("not recognized command");
        break;
      //help
        case 0:
        Serial.println("commands:");
        for(int i=0;i<commandIndexWords;i++){
          if(commandIndex[i][0]!='\0'){
          Serial.println(commandIndex[i]);
          }
        }
        Serial.println("----");
        break;
      //debug 
        case 1:
        debug=!debug;
        if(debug){
          Serial.println("debug on");
        }else{
          Serial.println("debug off");
        }
        break;
      //add
        case 2:
        if (inCom.isValidLong(inCom.commandArray[1])&&inCom.isValidLong(inCom.commandArray[2])){
          Serial.print(atol(inCom.commandArray[1]));
          Serial.print("+");
          Serial.print(atol(inCom.commandArray[2]));
          Serial.print("=");
          Serial.println(atol(inCom.commandArray[1])+atol(inCom.commandArray[2]));
        }else{
          Serial.println("not valid numbers");
          if(debug){
            Serial.print(inCom.isValidLong(inCom.commandArray[1]));
            Serial.println(inCom.isValidLong(inCom.commandArray[2]));
          }
        }
        break;
      //IR
        case 3:
          if (inCom.isValidHex(inCom.commandArray[1])&&inCom.isValidHex(inCom.commandArray[2])){
            IrSender.sendNEC(strtol(inCom.commandArray[1],NULL,16),strtol(inCom.commandArray[2],NULL,16),0);
          }else{
            Serial.println("invalid input");
          }
          if(debug){
              Serial.println(strtol(inCom.commandArray[1],NULL,16),HEX);
              Serial.println(strtol(inCom.commandArray[2],NULL,16),HEX);

          }
        break;
        


    }
  
}


void refreshTFT(){
  //refresh TFT
    if(millis()-millisLastRefresh>50){
      img.fillSprite(TFT_BLACK);
      img.setTextSize(2);
      img.setTextColor(TFT_WHITE);
      img.setCursor(0, 0, 2);
      img.println(inCom.commandString); 

      img.pushSprite(0, 0);
    }  

}



