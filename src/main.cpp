#include <Arduino.h>
#include <serialCommand.h>
#include "TFT_eSPI.h"
#include "SPI.h"
#include "SPIFFS.h"
#include <esp_now.h>
#include <WiFi.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include "pin_config.h"
#include "OneButton.h" // https://github.com/mathertel/OneButton

OneButton button(BTN_PIN, true);


serialCommand inCom;

#ifdef USE_SD_MMC
  #include "USBMSC.h"
  #include "driver/sdmmc_host.h"
  #include "driver/sdspi_host.h"
  #include "esp_vfs_fat.h"
  #include "sdmmc_cmd.h"
  #include <stdio.h>
  #include <dirent.h>

  USBMSC MSC;

  #define MOUNT_POINT "/sdcard"
  sdmmc_card_t *card;


  void sd_init(void) {
    esp_err_t ret;
    const char mount_point[] = MOUNT_POINT;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {.format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    sdmmc_host_t host = {
        .flags = SDMMC_HOST_FLAG_4BIT | SDMMC_HOST_FLAG_DDR,
        .slot = SDMMC_HOST_SLOT_1,
        .max_freq_khz = SDMMC_FREQ_DEFAULT,
        .io_voltage = 3.3f,
        .init = &sdmmc_host_init,
        .set_bus_width = &sdmmc_host_set_bus_width,
        .get_bus_width = &sdmmc_host_get_slot_width,
        .set_bus_ddr_mode = &sdmmc_host_set_bus_ddr_mode,
        .set_card_clk = &sdmmc_host_set_card_clk,
        .do_transaction = &sdmmc_host_do_transaction,
        .deinit = &sdmmc_host_deinit,
        .io_int_enable = sdmmc_host_io_int_enable,
        .io_int_wait = sdmmc_host_io_int_wait,
        .command_timeout_ms = 0,
    };
    sdmmc_slot_config_t slot_config = {
        .clk = (gpio_num_t)SD_MMC_CLK_PIN,
        .cmd = (gpio_num_t)SD_MMC_CMD_PIN,
        .d0 = (gpio_num_t)SD_MMC_D0_PIN,
        .d1 = (gpio_num_t)SD_MMC_D1_PIN,
        .d2 = (gpio_num_t)SD_MMC_D2_PIN,
        .d3 = (gpio_num_t)SD_MMC_D3_PIN,
        .cd = SDMMC_SLOT_NO_CD,
        .wp = SDMMC_SLOT_NO_WP,
        .width = 4, // SDMMC_SLOT_WIDTH_DEFAULT,
        .flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP,
    };

    gpio_set_pull_mode((gpio_num_t)SD_MMC_CMD_PIN, GPIO_PULLUP_ONLY); // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode((gpio_num_t)SD_MMC_D0_PIN, GPIO_PULLUP_ONLY);  // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode((gpio_num_t)SD_MMC_D1_PIN, GPIO_PULLUP_ONLY);  // D1, needed in 4-line mode only
    gpio_set_pull_mode((gpio_num_t)SD_MMC_D2_PIN, GPIO_PULLUP_ONLY);  // D2, needed in 4-line mode only
    gpio_set_pull_mode((gpio_num_t)SD_MMC_D3_PIN, GPIO_PULLUP_ONLY);  // D3, needed in 4- and 1-line modes

    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
        inCom.println("Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
      } else {
        inCom.println("Failed to initialize the card .");
        inCom.println(esp_err_to_name(ret));
        }
      return;
    }
  }

  static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    // inCom.printf("MSC WRITE: lba: %u, offset: %u, bufsize: %u\n", lba, offset, bufsize);
    uint32_t count = (bufsize / card->csd.sector_size);
    sdmmc_write_sectors(card, buffer + offset, lba, count);
    return bufsize;
  }

  static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    // inCom.printf("MSC READ: lba: %u, offset: %u, bufsize: %u\n", lba, offset, bufsize);
    uint32_t count = (bufsize / card->csd.sector_size);
    sdmmc_read_sectors(card, buffer + offset, lba, count);
    return bufsize;
  }

  static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    //inCom.printf("MSC START/STOP: power: %u, start: %u, eject: %u\n", power_condition, start, load_eject);
    return true;
  }

  static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == ARDUINO_USB_EVENTS) {
      arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
      switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT:
        //inCom.println("USB PLUGGED");
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        //inCom.println("USB UNPLUGGED");
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        //inCom.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en);
        break;
      case ARDUINO_USB_RESUME_EVENT:
        //inCom.println("USB RESUMED");
        break;

      default:
        break;
      }
    }
  }
#endif

#define debug inCom.debug
#define IR_SEND_PIN IR_SEND_PIN_MOD

void refreshTFT(); 

bool espnowtryinit();
bool usbTryInit();
void identifyCommand(char commandArray[50][20]);
void identifyCommand(String command);
void identifyCommand(char command[200]);
void identifyCommand(const char command[200]);
String serialcommand(bool flush);
bool isValidHex(const char* str);
void strToMac(const char* str, uint8_t* mac);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void printMac(uint8_t mac[6]);
void mainSendFunction(char command[200],int msgID);
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

USBHIDKeyboard Keyboard;

//ANSI format
  const char black[15]="\033[30m";
  const char red[15]="\033[31m";
  const char green[15]="\033[32m";
  const char yellow[15]="\033[33m";
  const char blue[15]="\033[34m";
  const char magenta[15]="\033[35m";
  const char cyan[15]="\033[36m";
  const char white[15]="\033[37m";
  
//init misc var
  char breakChar='a';
  long millisLastRefresh;
  int wordReturn=1;
  char wordBuffer[30];
  int commandInt;
  fs::File file;
  bool usbInit=false;
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
  uint8_t lastReceivedMac[6];
  uint8_t sendBackMac;
  String receivedCommand="";
//init command array
  //main array  
    char commandIndex[50][20]={
      "/help",
      "/debug",
      "/add",
      "/ir",
      "/bt",
      "/file",
      "/espnow",
      "/run",
      "/test",
      "/usb"
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
  //file
    char fileIndex[50][20]={
      "open",
      "read"
    };
  //espnow
    char espnowIndex[50][20]={
      "register",
      "tgt",
      "listreg",
      "send",
      "rec",
      "init",
      "mac",
      "cmd",
      "pair"
    };
  //usb
    char usbIndex[50][20]={
      "STRING",
      "REM",
      "DELAY",
      "MSC",


    };
    char usbModkeyIndex[50][20]={
      "GUI",
      "ENTER",
      "TAB",
      "ALT",
      "SHIFT"



    };
//


void setup() {
  //init serial
    Serial.begin(115200);
  #ifdef USE_TFT_ESPI 
  //init TFT
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    img.createSprite(240, 135);
    img.fillSprite(TFT_BLACK);
  #endif
  
  //init ir
    #ifdef IR_RECEIVE_PIN_MOD
      IrReceiver.begin(IR_RECEIVE_PIN_MOD,false);
    #endif
    #ifdef IR_SEND_PIN_MOD
      IrSender.begin(); // Start with IR_SEND_PIN as send pin and disable feedback LED at default feedback LED pin
    #endif

  //init MSC
    #ifdef USE_SD_MMC
      sd_init();
      USB.onEvent(usbEventCallback);
      MSC.vendorID("LILYGO");       // max 8 chars
      MSC.productID("T-Dongle-S3"); // max 16 chars
      MSC.productRevision("1.0");   // max 4 chars
      MSC.onStartStop(onStartStop);
      MSC.onRead(onRead);
      MSC.onWrite(onWrite);
      MSC.begin(card->csd.capacity, card->csd.sector_size);
    #endif


  //file
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    //startup commands
      identifyCommand("/run /startupCommands.txt");
      identifyCommand("/run /omniTerminalSYS/SDstartupCommands.txt");
  button.attachClick([] {
    inCom.clearCMD();
    identifyCommand("/run /omniTerminalSYS/onButtonPress.txt");
    inCom.reprintCMD();
    });
}


void loop() {
  button.tick();
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
      inCom.flush(true);
    }else if(strcmp(receivedCommand.c_str(),"")!=0){
      inCom.parseCommandArray(receivedCommand,true);
      receivedCommand="";
      identifyCommand(inCom.commandArray);
      inCom.flush(false);
    }
  refreshTFT();
}
//identifyCommand processers
  void identifyCommand(char command[200]){
    String bufferString=command;
    identifyCommand(bufferString);
  }
  void identifyCommand(const char command[200]){
    String bufferString=command;
    identifyCommand(bufferString);
  }
  void identifyCommand(String command){
    command.trim();
    inCom.parseCommandArray(command,false);
    identifyCommand(inCom.commandArray);
    inCom.flush(false);
  }
//


void identifyCommand(char commandArray[50][20]){
  //find the command
    commandInt=inCom.multiComp(commandArray[0],commandIndex);
    
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
      //file
        case 5:{
          switch (inCom.multiComp(commandArray[1],fileIndex))
          {
          //norec
            case -1:{
              inCom.noRec("file");
            break;}
          //open
            case 0:{
            file = SPIFFS.open(commandArray[2]);
            if(!file){
              inCom.println("Failed to open file for reading");
              return;
            }
            break;}
          //read
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
                  }else{inCom.noRec("receive");}
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
            //cmd
              case 7:{
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

                  espnowMessage.msgID=2;
                  esp_err_t result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                  if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                }
              break;}
            //pair
              case 8:{
                if(!strcmp(commandArray[2],"")){
                  if(espnowtryinit()){
                    inCom.registerSendFunction(mainSendFunction);
                    // Populate the peerInfo structure
                      memcpy(peerInfoArray[9].peer_addr, lastReceivedMac, 6); // Copy MAC address
                      peerInfoArray[9].channel = 0;  // Set the channel
                      peerInfoArray[9].encrypt = false;  // Set encryption (true or false)

                    esp_now_add_peer(&peerInfoArray[9]);
                  inCom.println("paired");
                  
                      
                  }
                }else if(!strcmp(commandArray[2],"off")){
                  inCom.unregisterSendFunction();
                }else {
                  inCom.noRec("pair");
                }
              break;}
          }
        break;}

      //run
        case 7:{
            //file
              if(!SPIFFS.begin(true)){
                Serial.println("An Error has occurred while mounting SPIFFS");
                return;
              }
              
              if(SPIFFS.exists(commandArray[1])){
                fs::File runFile=SPIFFS.open(commandArray[1]);
                while(runFile.available()){
                  String startCommand = runFile.readStringUntil('\n');
                  startCommand.trim();
                  inCom.println(startCommand);
                  inCom.parseCommandArray(startCommand,false);
                  identifyCommand(inCom.commandArray);
                  inCom.flush(false);
                }
                runFile.close();
              }else{
                char fileLocation[30]="/sdcard";
                strcat(fileLocation,commandArray[1]);
                FILE *SDrunFile = fopen(fileLocation,"r");
                if(SDrunFile!=NULL){
                  char buffer[256];
                  // Read and print contents line by line
                  while (fgets(buffer, sizeof(buffer), SDrunFile) != NULL) {
                      String bufferString=buffer;
                      bufferString.trim();
                      inCom.println(bufferString);
                      inCom.parseCommandArray(bufferString,false);
                      identifyCommand(inCom.commandArray);
                      inCom.flush(false);
                  }
                  fclose(SDrunFile);
                }else{
                  inCom.println("file not found",red);
                }
              }
                
        break;}
      //test
        case 8:{
          FILE *thing = fopen("/sdcard/hello.txt","r");
          if(thing!=NULL){
            char buffer[256];
            // Read and print contents line by line
            while (fgets(buffer, sizeof(buffer), thing) != NULL) {
                inCom.println(buffer);
            }
            fclose(thing);
          }else{
            inCom.println("file not found");
          }
          /*
            DIR *dir = opendir("/sdcard");
            
            if (dir == NULL) {
                inCom.println("Failed to open directory");
                return;
            }

            struct dirent *entry;

            
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_REG) {
                    inCom.print("File: ");
                    inCom.println(entry->d_name);
                } else if (entry->d_type == DT_DIR) {
                    inCom.print("Directory: ");
                    inCom.println(entry->d_name);
                }
            }
            closedir(dir);
            */
        break;}
      //usb
        case 9:{
          if(strcmp(commandArray[1],"deinit")==0){
            Keyboard.end();
            usbInit=false;
          }else{
            if(usbTryInit()){
              switch (inCom.multiComp(commandArray[1],usbIndex))
              {
                //norec
                  case -1:{
                    
                    for(int i=1;strcmp(commandArray[i],"");i++){
                      switch (inCom.multiComp(commandArray[i],usbModkeyIndex)){
                        //single char/ norec
                          case -1:{
                            if(strlen(commandArray[i])==1){
                              Keyboard.press(commandArray[i][0]);
                            }else{
                              inCom.noRec("usb");
                            }
                          break;}
                      
                        //GUI
                          case 0:{
                            Keyboard.press(KEY_LEFT_GUI);
                          break;}
                        //ENTER
                          case 1:{
                            Keyboard.press(KEY_RETURN);
                          break;}
                        //TAB
                          case 2:{
                            Keyboard.press(KEY_TAB);
                          break;}
                        //ALT
                          case 3:{
                            Keyboard.press(KEY_LEFT_ALT);
                          break;}
                        //SHIFT
                          case 4:{
                            Keyboard.press(KEY_LEFT_SHIFT);
                          break;}
                      }
                    }
                    Keyboard.releaseAll();
                  break;}
                //STRING
                  case 0:{
                    char USBsendString[200];
                    strcpy(USBsendString,commandArray[2]);
                    for(int i=0;i<20;i++){
                      if(strcmp(commandArray[i+3],"")){
                        strcat(USBsendString," ");
                        strcat(USBsendString,commandArray[i+3]);
                      }
                    }
                    Keyboard.print(USBsendString);
                  break;}
                //REM
                  case 1:{
                    /*
                    char USBsendString[200];
                    strcpy(USBsendString,commandArray[2]);
                    for(int i=0;i<20;i++){
                      if(strcmp(commandArray[i+3],"")){
                        strcat(USBsendString," ");
                        strcat(USBsendString,commandArray[i+3]);
                      }
                    }
                    inCom.println(USBsendString);
                    */
                  break;}
                //DELAY
                  case 2:{
                    vTaskDelay(atoi(commandArray[2]));
                  break;}
                //MSC
                  case 3:{
                    if(strcmp(commandArray[2],"on")==0){
                      MSC.mediaPresent(true);
                    }else if(strcmp(commandArray[2],"off")==0){
                      MSC.mediaPresent(false);
                    }else{
                      inCom.noRec("MSC");
                    }
                  break;}
                
              }
            }
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

bool usbTryInit(){
  if(!usbInit){
    Keyboard.begin();
    if (!USB.begin()) {
      inCom.println("Error initializing USB");
      return false;
    }else{
      usbInit=true;
      inCom.println("--initialized USB--");
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
  if(espnowMessage.msgID!=0){
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
}
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&espnowRecieved, incomingData, sizeof(espnowRecieved));
  memcpy(lastReceivedMac,mac,6);
  if(espnowReceiveSet==2){
    if(espnowRecieved.msgID==0){
      inCom.print(espnowRecieved.command);
    }else{
      char recBufChar[230];
      sprintf(recBufChar,"~espnow {%02X:%02X:%02X:%02X:%02X:%02X ID:%d}---- %s"
      ,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],espnowRecieved.msgID,espnowRecieved.command);
      inCom.println(recBufChar);
    }
  }
  if (espnowRecieved.msgID==2){
    receivedCommand = espnowRecieved.command;
  
  }
}

void printMac(uint8_t mac[6]){
  char macString[20];
  for (int i = 0; i < 6; i++) {
    sprintf(macString + 3 * i, "%02X:", mac[i]);
  }
  inCom.print(macString);
}
void mainSendFunction(char command[200],int msgID){
  //--------YOU CANT SEND MORE THAN 3 PACKETS FROM DATARECV---------
  strcpy(espnowMessage.command,command);
  espnowMessage.msgID=msgID;
  esp_err_t result;
  result= esp_now_send( peerInfoArray[9].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
  if(debug){Serial.println(esp_err_to_name(result));}
}

