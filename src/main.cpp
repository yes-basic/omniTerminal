#include <Arduino.h>
#include <serialCommand.h>

#include <TFT_eSPI.h>

void refreshTFT();
void identifyCommand();
String serialcommand(bool flush);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

serialCommand inCom;
//init misc var
  bool debug=0;
  long millisLastRefresh;
  int wordReturn=1;
  char wordBuffer[30];
  int commandInt;
//init command array
  char commandIndex[50][20]={
    "/help",
    "/command",
    "/123",
    "/debug"
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
      Serial.println(inCom.commandString);
      inCom.parseCommandArray();
      for (int i = 0; i < inCom.wordsInCommand; i++) {
        Serial.print("Word ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(inCom.commandArray[i]);
    }

      identifyCommand();
      inCom.flush();
    }
  refreshTFT();
}
void identifyCommand(){
  if(!strcmp(inCom.commandArray[0],"/test")){
    Serial.println(inCom.commandArray[1]);
  }else{
    Serial.println(inCom.commandArray[0]);
  }
  commandInt=inCom.multiComp(inCom.commandArray[0],commandIndex);
  Serial.println(commandInt);
  switch(commandInt){
    case 0:
    Serial.println("commands:");
    for(int i=0;i<commandIndexWords;i++){
      if(commandIndex[i][0]!='\0'){
      Serial.println(commandIndex[i]);
      }
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