#include <Arduino.h>
#include <serialCommand.h>
#include <TFT_eSPI.h>
#include "SPI.h"
#include "SPIFFS.h"
#include <esp_now.h>
#include <WiFi.h>
#define debug inCom.debug
#define IR_SEND_PIN IR_SEND_PIN_MOD

void refreshTFT(); 

bool espnowtryinit();
void identifyCommand(char commandArray[50][20]);
String serialcommand(bool flush);
bool isValidHex(const char* str);
void strToMac(const char* str, uint8_t* mac);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void printMac(uint8_t mac[6]);
//include ir dependancies
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
#ifdef USE_TFT_ESPI 
  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite img = TFT_eSprite(&tft);
#endif
serialCommand inCom;
//init misc var
  char breakChar='a';
  long millisLastRefresh;
  int wordReturn=1;
  char wordBuffer[30];
  int commandInt;
  fs::File file;
//init espnow var
  typedef struct struct_message {
    char command[200];
    int msgID;
  } struct_message;
  struct_message espnowMessage;
  struct_message espnowRecieved;
  bool espnowInit=false;
  const int peerNumber=10;
  uint8_t peerAddress[peerNumber][6];
  esp_now_peer_info_t peerInfoArray[peerNumber];
  int espnowTGT=0;
  bool espnowAutorun=false;
  int espnowReceiveSet=0;
//init command array
  //main array  
    char commandIndex[50][20]={
      "/help",
      "/debug",
      "/add",
      "/ir",
      "/bt",
      "/spiffs",
      "/espnow"
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
  //bluetooth
    char bluetoothIndex[50][20]={
      "begin",
      "end"
    };
  //spiffs
    char spiffsIndex[50][20]={
      "open",
      "read"
    };
  //espnow
    char espnowIndex[50][20]={
      "register",
      "tgt",
      "listreg",
      "send",
      "receive",
      "init",
      "mac"
    };
void setup() {
  //init serial
    Serial.begin(115200);
  #ifdef USE_TFT_ESPI 
  //init TFT
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    img.createSprite(240, 135);
    img.fillSprite(TFT_BLACK);
  #endif
  //init ir
    IrReceiver.begin(IR_RECEIVE_PIN_MOD,false);
    IrSender.begin(); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin
  //spiffs
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    //startup commands
      if(SPIFFS.exists("/startupCommands")){
        fs::File startupCommands=SPIFFS.open("/startupCommands");
        while(startupCommands.available()){
          String startCommand = startupCommands.readStringUntil('\n');
          startCommand.trim();
          inCom.println(startCommand);
          inCom.parseCommandArray(startCommand,true);
          identifyCommand(inCom.commandArray);
          inCom.flush();
        }
        startupCommands.close();
      }
}


void loop() {
  //check command
    if(inCom.check()){
      if(debug){inCom.println(inCom.commandString);}

      if(debug){
        for (int i = 0; i < inCom.wordsInCommand; i++) {
          inCom.print("Word ");
          inCom.print(i);
          inCom.print(": ");
          inCom.print("\"");
          inCom.print(inCom.commandArray[i]);
          inCom.println("\"");
        }
      }
      if(inCom.commandString.charAt(0)=='>'){
        inCom.commandString.remove(0,1);
        inCom.parseCommandArray(inCom.commandString,false);
        for(int i=0;i<=20;i++){
          strcpy(inCom.addonArray[i],inCom.commandArray[i]);
        }
      }else{
        inCom.parseCommandArray(inCom.commandString,true);
        identifyCommand(inCom.commandArray);
      }
      inCom.flush();
    }
  refreshTFT();
}

void identifyCommand(char commandArray[50][20]){
  //find the command
    commandInt=inCom.multiComp(commandArray[0],commandIndex);
    if(debug){inCom.println(commandInt);}
  //do the command
    switch(commandInt){
      //not recognized
        case -1:{
          inCom.noRec("commands");
        break;}
      //help
        case 0:{
        inCom.println("commands:");
        for(int i=0;i<commandIndexWords;i++){
          if(commandIndex[i][0]!='\0'){
          inCom.println(commandIndex[i]);
          }
        }
        inCom.println("----");
        break;}
      //debug 
        case 1:{
        debug=!debug;
        if(debug){
          inCom.println("debug on");
        }else{
          inCom.println("debug off");
        }
        break;}
      //add
        case 2:{
        if (inCom.isValidLong(commandArray[1])&&inCom.isValidLong(commandArray[2])){
          inCom.print(atol(commandArray[1]));
          inCom.print("+");
          inCom.print(atol(commandArray[2]));
          inCom.print("=");
          inCom.println(atol(commandArray[1])+atol(commandArray[2]));
        }else{
          inCom.println("not valid numbers");
          if(debug){
            inCom.print(inCom.isValidLong(commandArray[1]));
            inCom.println(inCom.isValidLong(commandArray[2]));
          }
        }
        break;}
      //IR
        case 3:{  
          int index=inCom.multiComp(commandArray[1],IRprotocolIndex)+1;
          if(index%2==0){index--;}
          if(debug){inCom.print("index:");inCom.println(index);}
          switch (index){
            //not listed
                case-1:
                inCom.noRec("IRprotocols");
                break;
            //main protocols
                case 1:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendNEC(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 3:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendSony(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 5:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendRC5(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 7:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendRC6(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 9:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendSharp(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 11:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendJVC(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 13:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendSamsung(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 15:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendLG(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 17:
                inCom.println("protocol is WIP");
                break;
                case 19:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendPanasonic(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
                case 21:
                  if (inCom.isValidHex(commandArray[2])&&inCom.isValidHex(commandArray[3])){
                    IrSender.sendDenon(strtol(commandArray[2],NULL,16),strtol(commandArray[3],NULL,16),0);
                  }
                break;
            //enable reciever
                case 23:
                  while(!inCom.available()){
                    delay(250);
                    if (IrReceiver.decode()) {
                      if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
                        if(!strcmp(commandArray[2],"all")){
                          inCom.println(F("Received noise or an unknown (or not yet enabled) protocol"));
                          // We have an unknown protocol here, print extended info
                          IrReceiver.printIRResultRawFormatted(&Serial, true);
                          inCom.println();
                        }
                        IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
                      } else {
                        IrReceiver.resume(); // Early enable receiving of the next IR frame
                        IrReceiver.printIRResultShort(&Serial);
                        IrReceiver.printIRSendUsage(&Serial);
                        inCom.println();
                      }
                    }
                  }
                break;
            //listed in index not but not switch
                default:
                inCom.println("protocol listed, not supported");
                break;
          }

          if(debug){
              Serial.println(strtol(commandArray[2],NULL,16),HEX);
              Serial.println(strtol(commandArray[3],NULL,16),HEX);

          }

        break;}
        

      //bluetooth
        case 4:{
          #ifdef USE_BTclassic
            switch (inCom.multiComp(commandArray[1],bluetoothIndex)){
              //command not recognized
                case -1:{
                  inCom.noRec("BTclassic");
                break;}
              //begin
                case 0:{
                  if(!strcmp(commandArray[2],"")){
                    inCom.SerialBT.begin("ESP32");
                    inCom.println("started bluetooth as: ESP32");
                  }else{
                    inCom.SerialBT.begin(commandArray[2]);
                    inCom.print("started bluetooth as: ");
                    inCom.println(commandArray[2]);
                  }
                break;}
              //end
                case 1:{
                  inCom.SerialBT.end();
                break;}
            }
          #else
          inCom.println("BTclassic disabled in environment");
          #endif
        break;}
      //spiffs
        case 5:{
          switch (inCom.multiComp(commandArray[1],spiffsIndex))
          {
          case -1:{
            inCom.noRec("spiffs");
          break;}

          case 0:{
          file = SPIFFS.open(commandArray[2]);
          if(!file){
            inCom.println("Failed to open file for reading");
            return;
          }
          break;}

          case 1:{
            if(file){
              while(file.available()){
                inCom.println(file.readStringUntil('\n'));
              }
              file.seek(0);
            }else{
              inCom.println("file not found");
            }
          break;}

          default:
            break;
          }
          
        break;}
      //espnow
        case 6:{
          switch (inCom.multiComp(commandArray[1],espnowIndex))
          {
            //not recognized 
              case -1:{
                inCom.noRec("espnow");
              break;}
            
            //register
              case 0:{
                // test address:    12:34:56:78:9A:BC
                if(espnowtryinit()){
                  if(inCom.isValidLong(commandArray[2])&&(atol(commandArray[2])%1)==0&& !(atol(commandArray[2])>peerNumber)){
                    
                    int peerArrayTGT=atol(commandArray[2]);
                    uint8_t address[6];
                    strToMac(commandArray[3],address);
                    //uint8_t mac[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

                    // Populate the peerInfo structure
                      memcpy(peerInfoArray[peerArrayTGT].peer_addr, address, 6); // Copy MAC address
                      peerInfoArray[peerArrayTGT].channel = 0;  // Set the channel
                      peerInfoArray[peerArrayTGT].encrypt = false;  // Set encryption (true or false)

                    if (esp_now_add_peer(&peerInfoArray[peerArrayTGT]) != ESP_OK) {
                      inCom.println("Failed to add peer");
                      
                    } else {
                      inCom.println("Peer added successfully");
                    }
                    printMac(peerInfoArray[espnowTGT].peer_addr);
                    inCom.println();

                  }else{
                    inCom.noRec("register");
                  }
                }
              break;}
            //tgt
              case 1:{
                if(espnowtryinit()){
                  if(inCom.isValidLong(commandArray[2])&&(atol(commandArray[2])%1)==0&& !(atol(commandArray[2])>peerNumber)){
                  espnowTGT=atoi(commandArray[2]);
                  inCom.print("Target set to:  (");
                  inCom.print(espnowTGT);
                  inCom.print(")  ");
                  printMac(peerInfoArray[espnowTGT].peer_addr);
                  inCom.println();
                  }else{
                    inCom.noRec("TGT");
                  }
                }
              break;}
            //listreg
              case 2:{
                if(espnowtryinit()){
                  for(int i=0;i<peerNumber;i++){
                    inCom.print("(");
                    inCom.print(i);
                    inCom.print(")  ");
                    printMac(peerInfoArray[i].peer_addr); 
                    inCom.println();
                  }
                }
              break;}

            //send
              case 3:{
                if(espnowtryinit()){
                  strcpy(espnowMessage.command,commandArray[2]);
                  for(int i=0;i<9;i++){
                    if(strcmp(commandArray[i+3],"")){
                      strcat(espnowMessage.command," ");
                      strcat(espnowMessage.command,commandArray[i+3]);
                    }

                    if(debug){inCom.print("--command");inCom.print(i);inCom.print("  ");inCom.println(commandArray[i+2]);}
                  }
                  if(debug){inCom.print("command compiled: "); inCom.println(espnowMessage.command);}

                  
                  
                
                  espnowMessage.msgID=1;
                  esp_err_t result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                  if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                }
              break;}

            //receive
              case 4:{
                if(espnowtryinit()){
                  if(strcmp(commandArray[2],"none")==0){
                    espnowReceiveSet=0;
                    esp_now_unregister_recv_cb();
                    inCom.println("receive set to none");
                    
                  }
                  if(strcmp(commandArray[2],"silent")==0){
                    espnowReceiveSet=1;
                    esp_now_register_recv_cb(OnDataRecv);
                    inCom.println("receive set to silent");
                  }
                  if(strcmp(commandArray[2],"full")==0){
                    espnowReceiveSet=2;
                    esp_now_register_recv_cb(OnDataRecv);
                    inCom.println("receive set to full");
                  }
                }
              break;}
            //init
              case 5:{
                espnowtryinit();
              break;}
            
            //mac
              case 6:{
                if(espnowtryinit()){
                  inCom.println(WiFi.macAddress());
                }
              break;}
          }
        break;}

    }
  
}


void refreshTFT(){
  #ifdef USE_TFT_ESPI 
  //refresh TFT
    if(millis()-millisLastRefresh>50){
      img.fillSprite(TFT_BLACK);
      img.setTextSize(2);
      img.setTextColor(TFT_WHITE);
      img.setCursor(0, 0, 2);
      img.println(inCom.commandString); 

      img.pushSprite(0, 0);
    }  
  #endif
}

bool espnowtryinit(){
  if(!espnowInit){
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
      inCom.println("Error initializing ESP-NOW");
      return false;
    }else{
      espnowInit=true;
      inCom.println("--initialized ESP-NOW--");
      esp_now_register_send_cb(OnDataSent);
      

      return true;
    }
  }else{
    return true;
  }
}

void strToMac(const char* str, uint8_t* mac) {
    sscanf(str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS){
    inCom.print("Packet successfully sent to: (");
    inCom.print(espnowTGT);
    inCom.print(")  ");
    printMac(peerInfoArray[espnowTGT].peer_addr);
    inCom.println();
  }else{
    inCom.print("Packet Failed to send to: (");
    inCom.print(espnowTGT);
    inCom.print(")  ");
    printMac(peerInfoArray[espnowTGT].peer_addr);
    inCom.println();
  }


}
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&espnowRecieved, incomingData, sizeof(espnowRecieved));
  if(espnowReceiveSet==2){
    inCom.print("~espnow {");
    printMac((uint8_t *) mac);
    if(debug){
      inCom.print(" Bytes:");
      inCom.print(len);
    }
    inCom.print(" ID:");
    inCom.print(espnowRecieved.msgID);

    inCom.print("}");
    inCom.print("---- ");
    inCom.println(espnowRecieved.command);
    inCom.print(inCom.commandString);
  }
}

void printMac(uint8_t mac[6]){
  char macString[20];
  for (int i = 0; i < 6; i++) {
    sprintf(macString + 3 * i, "%02X:", mac[i]);
  }
  inCom.print(macString);
}





