#include <Arduino.h>
#include <serialCommand.h>
#include <TFT_eSPI.h>
#include "SPI.h"
#include <BleKeyboard.h>
#include <BleMouse.h>
#include "nvs_flash.h"


#define debug inCom.debug
void refreshTFT();
void identifyCommand();
String serialcommand(bool flush);
bool isValidHex(const char* str);

/*
 * This include defines the actual pin number for pins like IR_RECEIVE_PIN, IR_SEND_PIN for many different boards and architectures
 */
#include "PinDefinitionsAndMore.h"
#if !defined(RAW_BUFFER_LENGTH)
#  if RAMEND <= 0x4FF || RAMSIZE < 0x4FF
#define RAW_BUFFER_LENGTH  180  // 750 (600 if we have only 2k RAM) is the value for air condition remotes. Default is 112 if DECODE_MAGIQUEST is enabled, otherwise 100.
#  elif RAMEND <= 0x8FF || RAMSIZE < 0x8FF
#define RAW_BUFFER_LENGTH  600  // 750 (600 if we have only 2k RAM) is the value for air condition remotes. Default is 112 if DECODE_MAGIQUEST is enabled, otherwise 100.
#  else
#define RAW_BUFFER_LENGTH  750  // 750 (600 if we have only 2k RAM) is the value for air condition remotes. Default is 112 if DECODE_MAGIQUEST is enabled, otherwise 100.
#  endif
#endif
#define MARK_EXCESS_MICROS    20
#include <IRremote.hpp> // include the library
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);


serialCommand inCom;
//init misc var
  char breakChar='a';
  long millisLastRefresh;
  int wordReturn=1;
  char wordBuffer[30];
  int commandInt;
//init command array
  //main array  
    char commandIndex[50][20]={
      "/help",
      "/debug",
      "/add",
      "/ir",
      "/ble"

    };
    int commandIndexWords=sizeof(commandIndex)/sizeof(commandIndex[0]);
  //IR protocols
    char IRprotocolIndex[50][20]={
      "NEC",
      "nec",
      "SONY",
      "sony",
      "RC5",
      "rc5",
      "RC6",
      "rc6",
      "SHARP",
      "sharp",
      "JVC",
      "jvc",
      "SAMSUNG",
      "samsung",
      "LG",
      "lg",
      "WHYNTER",
      "whynter",
      "PANASONIC",
      "panasonic",
      "DENON",
      "denon", 
      "receive",
      "rec"

    };
void setup() {
  //init serial
    Serial.begin(115200);
  //init TFT
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    img.createSprite(240, 135);
    img.fillSprite(TFT_BLACK);
  //init IR
    IrReceiver.begin(IR_RECEIVE_PIN,ENABLE_LED_FEEDBACK);
    IrSender.begin(); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin
  //init BLE
    Keyboard.begin();
    Mouse.begin();
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
          {
          int index=inCom.multiComp(inCom.commandArray[1],IRprotocolIndex)+1;
          if(index%2==0){index--;}
          if(debug){Serial.print("index:");Serial.println(index);}
          switch (index){
            //not listed
                case-1:
                Serial.println("invalid protocol");
                break;
            //main protocols
                case 1:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendNEC(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 3:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendSony(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 5:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendRC5(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 7:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendRC6(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 9:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendSharp(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 11:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendJVC(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 13:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendSamsung(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 15:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendLG(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 17:
                Serial.println("protocol is WIP");
                break;
                case 19:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendPanasonic(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
                case 21:
                  if (inCom.isValidHex(inCom.commandArray[2])&&inCom.isValidHex(inCom.commandArray[3])){
                    IrSender.sendDenon(strtol(inCom.commandArray[2],NULL,16),strtol(inCom.commandArray[3],NULL,16),0);
                  }
                break;
            //enable reciever
                case 23:
                  while(!Serial.available()){
                    delay(250);
                    if (IrReceiver.decode()) {
                      if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
                        if(!strcmp(inCom.commandArray[2],"all")){
                          Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
                          // We have an unknown protocol here, print extended info
                          IrReceiver.printIRResultRawFormatted(&Serial, true);
                          Serial.println();
                        }
                        IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
                      } else {
                        IrReceiver.resume(); // Early enable receiving of the next IR frame
                        IrReceiver.printIRResultShort(&Serial);
                        IrReceiver.printIRSendUsage(&Serial);
                        Serial.println();
                      }
                    }
                  }
                break;
            //listed in index not but not switch
                default:
                Serial.println("protocol listed, not supported");
                break;
          }

          if(debug){
              Serial.println(strtol(inCom.commandArray[2],NULL,16),HEX);
              Serial.println(strtol(inCom.commandArray[3],NULL,16),HEX);

          }
          }
        break;
        
      //ble
        case 4:
          if(bleDevice.isConnected()){
            Keyboard.print(inCom.commandArray[1]);//Serial.println(inCom.commandArray[1]);
                /*Keyboard.press(KEY_LEFT_CTRL);
                Keyboard.press(KEY_LEFT_ALT);
                Keyboard.press(KEY_DELETE);
                delay(100);
                Keyboard.releaseAll();
                */
          }else{
            Serial.println("ble not connected");
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



