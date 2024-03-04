#include <Arduino.h>
#include <serialCommand.h>

#include <TFT_eSPI.h>
#define debug inCom.debug
void refreshTFT();
void identifyCommand();
String serialcommand(bool flush);

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
    "/123",
    "/command"
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
          Serial.println(inCom.commandArray[i]);
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