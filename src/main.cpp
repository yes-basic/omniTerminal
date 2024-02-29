#include <Arduino.h>
#include <serialCommand.h>

#include <TFT_eSPI.h>

String serialcommand(bool flush);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

serialCommand inCom;
//init misc var
  bool debug=0;
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
    if(Serial.available())
    if(inCom.check()){
      Serial.println(inCom.commandString);
      inCom.flush();
    }

    delay(50);
    img.fillSprite(TFT_BLACK);
    img.setTextSize(2);
    img.setTextColor(TFT_WHITE);
    img.setCursor(0, 0, 2);
    img.println(inCom.commandString); 

    img.pushSprite(0, 0);
      

}
