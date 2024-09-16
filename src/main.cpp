#include <Arduino.h>
#include <serialCommand.h>
#include "TFT_eSPI.h"
#include "SPI.h"
#include "SPIFFS.h"
#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ElegantOTA.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include "pin_config.h"
#include "OneButton.h" // https://github.com/mathertel/OneButton
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

String vc(std::vector<String> vector,int index);
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
void printMac(uint8_t mac[6],const char color[15]);
//listener functions
  void espnowSendFunction(String data);
  void websocketListener(String data);
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
long testTime;
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
  TaskHandle_t mainTaskHandle;
  esp_now_send_status_t receiveResult;
  String receivedCommandBuf;
  String commandBuf;
  String fullCommandString;
  char breakChar='a';
  long millisLastRefresh;
  int wordReturn=1;
  char wordBuffer[30];
  int commandInt;
  fs::File file;
  bool usbInit=false;
  std::vector<String> commandQueue;
//init espnow var
  typedef struct struct_message {
    char command[200];
    int msgID;
    int msgOrder;
  } struct_message;
  struct_message espnowMessage;
  struct_message espnowRecieved;
  int espnowmsgOrderLast=-1;
  String espnowCompiledMsg;
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
      "/usb",
      "/wifi",
      "/reset",
      "/ducky",
      "/sducky"
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
      "hex"

    };
    char usbModkeyIndex[50][20]={
      "GUI",
      "ENTER",
      "TAB",
      "ALT",
      "SHIFT",
      "CTRL",
      "COMMAND",
      "ESCAPE",
      "PAUSE",
      "PRINTSCREEN",
      "MENU",
      "BACKSPACE",
      "DELETE",
      "UP",
      "LEFT",
      "DOWN",
      "RIGHT",
      "PAGEUP",
      "PAGEDOWN",
      "HOME",
      "END",
      "F1",
      "F2",
      "F3",
      "F4",
      "F5",
      "F6",
      "F7",
      "F8",
      "F9",
      "F10",
      "F11",
      "F12",
      "SPACE",
      "INSERT",
      "CAPSLOCK"
      



    };
  //wifi
    char wifiIndex [50][20]={
      "ssid",
      "password",
      "begin",
      "end"
    };
//

//wifi startup
  String ssid = "ESP32-AP";
  String password = "";

  // DNS server configuration
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
  //IPAddress apIP(192, 168, 4, 1); // IP address of the ESP32 in AP mode
  const IPAddress localIP(4, 3, 2, 1);		   // the IP address the web server, Samsung requires the IP to be in public space
  const IPAddress gatewayIP(4, 3, 2, 1);		   // IP address of the network should be the same as the local IP for captive portals
  const IPAddress subnetMask(255, 255, 255, 0);
  const String localIPURL = "http://4.3.2.1";
  // Web server
  AsyncWebServer server(80);
  
  AsyncWebSocket ws("/ws"); // WebSocket server on the "/ws" path

  String inputMessage = "No message";

  // WebSocket event handler
  void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t length) {
    if (type == WS_EVT_CONNECT) {
      inCom.clearCMD();
      inCom.print("WebSocket client #");
      inCom.print((int)client->id());
      inCom.print(" connected\n");
      inCom.reprintCMD();
    } else if (type == WS_EVT_DISCONNECT) {
      inCom.clearCMD();
      inCom.print("WebSocket client #");
      inCom.print((int)client->id());
      inCom.print(" disconnected\n");
      inCom.reprintCMD();
    } else if (type == WS_EVT_DATA) {
      inCom.clearCMD();
      String msg = String((char*)data);
      inCom.println("Received message: " + msg);
      client->text("Echo: " + msg); // Echo back the received message
      commandQueue.push_back(msg);
      inCom.reprintCMD();
    }
  }
  

  void printRequestDetails(AsyncWebServerRequest *request) {
    if(debug){
      inCom.clearCMD();
      inCom.println("Request received:");
      inCom.print("  URL: ");
      inCom.println(request->url());
      inCom.print("  Method: ");
      inCom.println(request->methodToString());
      inCom.print("  HTTP Version: ");
      inCom.println(request->version());
      
      // Print the headers
      inCom.println("  Headers:");
      int headersCount = request->headers();
      for (int i = 0; i < headersCount; i++) {
        const AsyncWebHeader* header = request->getHeader(i);
        inCom.print("    ");
        inCom.print(header->name());
        inCom.print(": ");
        inCom.println(header->value());
      }

      // Print the parameters (if any)
      inCom.println("  Parameters:");
      int paramsCount = request->params();
      for (int i = 0; i < paramsCount; i++) {
        const AsyncWebParameter* param = request->getParam(i);
        inCom.print("    ");
        inCom.print(param->name());
        inCom.print(": ");
        inCom.println(param->value());
      }

      inCom.println(); // Just to add a blank line for readability
      inCom.reprintCMD();
    }
  }




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
      //identifyCommand("/run /omniTerminalSYS/SDstartupCommands.txt");
  button.attachClick([] {
    inCom.clearCMD();
    identifyCommand("/run /omniTerminalSYS/onButtonPress.txt");
    inCom.reprintCMD();
    });
  mainTaskHandle=xTaskGetCurrentTaskHandle();
}


void loop() {
  dnsServer.processNextRequest();
  ElegantOTA.loop();
  button.tick();
  //check command
    if(inCom.check()){
      commandBuf=inCom.commandString;
      inCom.flush(true);
      if(debug){inCom.println(commandBuf);}

      if(commandBuf.charAt(0)=='>'){
        commandBuf.remove(0,1);
        inCom.addonString=commandBuf;
        fullCommandString=inCom.addonString+">>"+commandBuf;
        inCom.println(fullCommandString,inCom.userColor);
      }else{
        fullCommandString=inCom.addonString+">>"+commandBuf;
        inCom.println(fullCommandString,inCom.userColor);
        fullCommandString=inCom.addonString+" "+commandBuf;
        fullCommandString.trim();
        identifyCommand(fullCommandString);
      }
      
    }else if(!commandQueue.empty()){
      receivedCommandBuf=inCom.addonString+">>"+vc(commandQueue,0);
      inCom.println(receivedCommandBuf,inCom.altUserColor);
      receivedCommandBuf=inCom.addonString+" "+vc(commandQueue,0);
      commandQueue.erase(commandQueue.begin());
      receivedCommandBuf.trim();
      identifyCommand(receivedCommandBuf);
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
  /*
  void identifyCommand(String command){
    command.trim();
    inCom.parseCommandArray(command,false);
    identifyCommand(inCom.commandArray);
    inCom.flush(false);
  }
  */
//


void identifyCommand(String command){
  //make legacy and extra command variables
    int commandWordCount = 0;
    int wordLength = 0;
    char commandArray[50][20];
    for (int k = 0; k <sizeof(commandArray)/sizeof(commandArray[0]); k++) {
      for (int j = 0; j < sizeof(commandArray[0]); j++) {
        commandArray[k][j] = '\0';  // Set each character to null
      }
    }
    bool legacyPossible=true;
    std::vector<String> commandVector;
    String vectorBuilderWordBuf;
    //create command vector
      for (int i = 0; i < command.length(); i++) {
        char c = command.charAt(i);
        // If current character is not a space, add it to current word
        if (c != ' ' && c != '\0') {
          if(legacyPossible){commandArray[commandWordCount][wordLength] = c;}
          vectorBuilderWordBuf += c;
          wordLength++;
        } else {
          if(legacyPossible){commandArray[commandWordCount][wordLength] = '\0';}
          commandVector.push_back(vectorBuilderWordBuf);
          vectorBuilderWordBuf="";
          commandWordCount++;
          wordLength = 0;  // Reset word length for next word
        }
        

          // Break if we have reached the maximum number of words
            if (commandWordCount >= sizeof(commandArray)/sizeof(commandArray[0])||wordLength>=sizeof(commandArray[0])) {
              if(debug&&legacyPossible){
                inCom.println("command is legacy incompatable!");
              }
              for (int k = 0; k <sizeof(commandArray)/sizeof(commandArray[0]); k++) {
                for (int j = 0; j < sizeof(commandArray[0]); j++) {
                  commandArray[k][j] = '\0';  // Set each character to null
                }
              }
              legacyPossible=false;
            }

      }
      commandWordCount++;
      if(!vectorBuilderWordBuf.isEmpty()){commandVector.push_back(vectorBuilderWordBuf);}

  
  //find the command
    commandInt=inCom.multiComp(vc(commandVector,0),commandIndex);
    
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
          switch (inCom.multiComp(vc(commandVector,1),espnowIndex))
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
                  
                  espnowMessage.msgID=1;
                  espnowMessage.msgOrder=0;
                  esp_err_t result;
                  if(command.length()-13<=200){
                    strcpy(espnowMessage.command,command.substring(13).c_str());
                        
                    espnowMessage.msgOrder=-2;
                    result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                    inCom.print("<single> ", green);
                    inCom.println(espnowMessage.command);
                    inCom.println();
                  }else{
                    for(int i=command.length()-13;i>=0;){
                      espnowMessage.msgID=1;
                      if(i<200){
                        strcpy(espnowMessage.command,command.substring(command.length()-i).c_str());
                        
                        espnowMessage.msgOrder=-1;
                        result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                        if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                        inCom.print("<closer> ", green);
                        inCom.println(espnowMessage.command);
                        inCom.println();
                        i=-1;
                      }else{
                        strcpy(espnowMessage.command,command.substring(command.length()-i,command.length()-i+200).c_str());
                        i=i-200;
                        
                        result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                        if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                        inCom.println("< ", green);
                        inCom.print(espnowMessage.msgOrder, green);
                        inCom.print("> ", green);
                        inCom.println(espnowMessage.command,yellow);
                        espnowMessage.msgOrder=espnowMessage.msgOrder+1;
                      }
                    }
                  }

                  if(result==ESP_OK){
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                  }

                  if(receiveResult == ESP_NOW_SEND_SUCCESS&&result==ESP_OK){
                    inCom.print(esp_err_to_name(result));
                    inCom.print(">");
                    inCom.print(esp_err_to_name(receiveResult));
                    inCom.print(": (");
                    inCom.print(espnowTGT);
                    inCom.print(")  ");
                    printMac(peerInfoArray[espnowTGT].peer_addr);
                    inCom.println();
                  }else{
                    inCom.print(esp_err_to_name(result),red);
                    inCom.print(">",red);
                    inCom.print(esp_err_to_name(receiveResult),red);
                    inCom.print(": (",red);
                    inCom.print(espnowTGT,red);
                    inCom.print(")  ",red);
                    printMac(peerInfoArray[espnowTGT].peer_addr,red);
                    inCom.println();
                  }

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
                  espnowMessage.msgID=2;
                  espnowMessage.msgOrder=0;
                  esp_err_t result;
                  
                  if(command.length()-12<=200){
                    strcpy(espnowMessage.command,command.substring(12).c_str());
                        
                    espnowMessage.msgOrder=-2;
                    esp_err_t result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                    if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                    inCom.print("<single> ", green);
                    inCom.println(espnowMessage.command);
                    inCom.println();
                  }else{
                    for(int i=command.length()-12;i>=0;){
                      espnowMessage.msgID=2;
                      if(i<200){
                        strcpy(espnowMessage.command,command.substring(command.length()-i).c_str());
                        
                        espnowMessage.msgOrder=-1;
                        esp_err_t result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                        if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                        inCom.print("<closer> ", green);
                        inCom.println(espnowMessage.command);
                        inCom.println();
                        i=-1;
                      }else{
                        strcpy(espnowMessage.command,command.substring(command.length()-i,command.length()-i+200).c_str());
                        i=i-200;
                        
                        esp_err_t result=esp_now_send( peerInfoArray[espnowTGT].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
                        if(debug){inCom.print("--send result:  "); inCom.println(esp_err_to_name(result));}
                        inCom.println("< ", green);
                        inCom.print(espnowMessage.msgOrder, green);
                        inCom.print("> ", green);
                        inCom.println(espnowMessage.command,yellow);
                        espnowMessage.msgOrder=espnowMessage.msgOrder+1;
                      }
                    }
                  }
                  
                  if(result==ESP_OK){
                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                  }
                  
                  if(receiveResult == ESP_NOW_SEND_SUCCESS&&result==ESP_OK){
                    inCom.print(esp_err_to_name(result));
                    inCom.print(">");
                    inCom.print(esp_err_to_name(receiveResult));
                    inCom.print(": (");
                    inCom.print(espnowTGT);
                    inCom.print(")  ");
                    printMac(peerInfoArray[espnowTGT].peer_addr);
                    inCom.println();
                  }else{
                    inCom.print(esp_err_to_name(result),red);
                    inCom.print(">",red);
                    inCom.print(esp_err_to_name(receiveResult),red);
                    inCom.print(": (",red);
                    inCom.print(espnowTGT,red);
                    inCom.print(")  ",red);
                    printMac(peerInfoArray[espnowTGT].peer_addr,red);
                    inCom.println();
                  }

                }
              break;}
            //pair
              case 8:{
                if(!strcmp(commandArray[2],"")){
                  if(espnowtryinit()){
                    inCom.registerListenerFunction("espnow",espnowSendFunction);
                    // Populate the peerInfo structure
                      memcpy(peerInfoArray[9].peer_addr, lastReceivedMac, 6); // Copy MAC address
                      peerInfoArray[9].channel = 0;  // Set the channel
                      peerInfoArray[9].encrypt = false;  // Set encryption (true or false)

                    esp_now_add_peer(&peerInfoArray[9]);
                  inCom.println("paired");
                  
                      
                  }
                }else if(!strcmp(commandArray[2],"off")){
                  inCom.registerListenerFunction("espnow",nullptr);
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
              inCom.println("An Error has occurred while mounting SPIFFS");
              return;
            }
            
            if(SPIFFS.exists(vc(commandVector,1).c_str())){
              fs::File runFile=SPIFFS.open(vc(commandVector,1).c_str());
              while(runFile.available()){
                String startCommand = runFile.readStringUntil('\n');
                startCommand.trim();
                inCom.println(startCommand,green);
                identifyCommand(startCommand);
                inCom.flush(false);
              }
              runFile.close();
            }else{
              String fileLocation="/sdcard"+vc(commandVector,1);        
              FILE *SDrunFile = fopen(fileLocation.c_str(), "r");
              if (SDrunFile != NULL) {
                String fileContent;
                char ch;
                
                // Read the entire file into a String
                while ((ch = fgetc(SDrunFile)) != char(-1)) {
                  if(ch=='\n'){
                    fileContent.trim();  // Remove any leading or trailing whitespace
                    inCom.println(fileContent, green);
                    identifyCommand(fileContent);
                    fileContent.clear();
                    inCom.flush(false);
                  }else{
                    fileContent += ch;
                  }
                }
                if(!fileContent.isEmpty()){
                  fileContent.trim();  // Remove any leading or trailing whitespace
                  inCom.println(fileContent, green);
                  identifyCommand(fileContent);
                  fileContent.clear();
                  inCom.flush(false);
                }
                fclose(SDrunFile);
              }else{
                inCom.print("file not found at: ",red);
                inCom.println(vc(commandVector,1),red);
              }
            }
              
        break;}
      //test
        case 8:{
          inCom.println(char(-1));
        break;}
      //usb
        case 9:{
          if(vc(commandVector,1).equals("deinit")){
            Keyboard.end();
            usbInit=false;
          }else{
            if(usbTryInit()){
              switch (inCom.multiComp(vc(commandVector,1),usbIndex))
              {
                //other
                  case -1:{
                    
                    for(int i=1;strcmp(vc(commandVector,i).c_str(),"");i++){
                      switch (inCom.multiComp(vc(commandVector,i),usbModkeyIndex)){
                        //single char/ norec
                          case -1:{
                            if(vc(commandVector,i).length()==1){
                              Keyboard.press(vc(commandVector,i).charAt(0));
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
                        //CTRL
                          case 5:{
                            Keyboard.press(KEY_LEFT_CTRL);
                          break;}
                        //COMMAND
                          case 6:{
                            Keyboard.press(KEY_LEFT_GUI);
                          break;}
                        //ESCAPE
                          case 7:{
                            Keyboard.press(KEY_ESC);
                          break;}
                        //PAUSE
                          case 8:{
                            Keyboard.press(0xD0);
                          break;}
                        //PRINTSCREEN
                          case 9:{
                            Keyboard.press(0xCE);
                          break;}
                        //MENU
                          case 10:{
                            Keyboard.press(0x65);
                          break;}
                        //BACKSPACE
                          case 11:{
                            Keyboard.press(KEY_BACKSPACE);
                          break;}
                        //DELETE
                          case 12:{
                            Keyboard.press(KEY_DELETE);
                          break;}
                        //UP
                          case 13:{
                            Keyboard.press(KEY_UP_ARROW);
                          break;}
                        //LEFT
                          case 14:{
                            Keyboard.press(KEY_LEFT_ARROW);
                          break;}
                        //DOWN
                          case 15:{
                            Keyboard.press(KEY_DOWN_ARROW);
                          break;}
                        //RIGHT
                          case 16:{
                            Keyboard.press(KEY_RIGHT_ARROW);
                          break;}
                        //PAGEUP
                          case 17:{
                            Keyboard.press(KEY_PAGE_UP);
                          break;}
                        //PAGEDOWN
                          case 18:{
                            Keyboard.press(KEY_PAGE_DOWN);
                          break;}
                        //HOME
                          case 19:{
                            Keyboard.press(KEY_HOME);
                          break;}
                        //END
                          case 20:{
                            Keyboard.press(KEY_END);
                          break;}
                        //F1
                          case 21:{
                            Keyboard.press(KEY_F1);
                          break;}
                        //F2
                          case 22:{
                            Keyboard.press(KEY_F2);
                          break;}
                        //F3
                          case 23:{
                            Keyboard.press(KEY_F3);
                          break;}
                        //F4
                          case 24:{
                            Keyboard.press(KEY_F4);
                          break;}
                        //F5
                          case 25:{
                            Keyboard.press(KEY_F5);
                          break;}
                        //F6
                          case 26:{
                            Keyboard.press(KEY_F6);
                          break;}
                        //F7
                          case 27:{
                            Keyboard.press(KEY_F7);
                          break;}
                        //F8
                          case 28:{
                            Keyboard.press(KEY_F8);
                          break;}
                        //F9
                          case 29:{
                            Keyboard.press(KEY_F9);
                          break;}
                        //F10
                          case 30:{
                            Keyboard.press(KEY_F10);
                          break;}
                        //F11
                          case 31:{
                            Keyboard.press(KEY_F11);
                          break;}
                        //F12
                          case 32:{
                            Keyboard.press(KEY_F12);
                          break;}
                        //SPACE
                          case 33:{
                            Keyboard.press(' ');
                          break;}
                        //INSERT
                          case 34:{
                            Keyboard.press(KEY_INSERT);
                          break;}
                        //CAPSLOCK
                          case 35:{
                            Keyboard.press(KEY_CAPS_LOCK);
                          break;}
                        
                      }
                    }
                    Keyboard.releaseAll();
                  break;}
                //STRING
                  case 0:{
                    Keyboard.print(command.substring(12));
                  break;}
                //REM
                  case 1:{
                    inCom.println(command.substring(9));
                    
                  break;}
                //DELAY
                  case 2:{
                    vTaskDelay(vc(commandVector,2).toInt());
                  break;}
                //MSC
                  case 3:{
                    #ifdef USE_SD_MMC
                      if(strcmp(commandArray[2],"on")==0){
                        MSC.mediaPresent(true);
                      }else if(strcmp(commandArray[2],"off")==0){
                        MSC.mediaPresent(false);
                      }else{
                        inCom.noRec("MSC");
                      }
                    #else
                      inCom.println("SD_MMC is disabled for this device!",red);
                    #endif
                  break;}
                //hex
                  case 4:{
                    Keyboard.press(strtol(vc(commandVector,2).c_str(),NULL,16));
                    Keyboard.release(strtol(vc(commandVector,2).c_str(),NULL,16));
                  break;}
                
              }
            }
          }

        break;}
      //wifi
        case 10:{
          switch(inCom.multiComp(vc(commandVector,1),wifiIndex)){
            //ssid
              case 0:{
                ssid=vc(commandVector,2);
                inCom.print("ssid changed to: ");
                inCom.println(vc(commandVector,2));
              break;}
            //password
              case 1:{
                password=vc(commandVector,2);
                inCom.print("password changed to: ");
                inCom.println(vc(commandVector,2));
              break;}
            //begin
              case 2:{
                #define MAX_CLIENTS 4
                // Define the WiFi channel to be used (channel 6 in this case)
                #define WIFI_CHANNEL 6

                // Set the WiFi mode to access point and station
                WiFi.mode(WIFI_MODE_AP);
                const IPAddress subnetMask(255, 255, 255, 0);
                WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
                WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

                inCom.println("Access Point Started");
                inCom.print("IP Address: ");
                inCom.println(WiFi.softAPIP().toString());
                // Start DNS server to redirect all queries to the ESP32's IP
                dnsServer.start(DNS_PORT, "*", localIP);  // '*' matches all domains

                

                if(!SPIFFS.begin(true)){
                  inCom.println("An Error has occurred while mounting SPIFFS");
                  return;
                }
                // Set up the web server
                server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                  //if (!request->authenticate("hello", "world")) {
                  //  return request->requestAuthentication();
                  //}
                  printRequestDetails(request);
                  request->send(SPIFFS, "/terminal.html", "text/html");
                });
                
                server.serveStatic("/ansi_up.min.js",SPIFFS,"/ansi_up.min.js");


                server.on("/generate_204", [](AsyncWebServerRequest *request) {printRequestDetails(request); request->redirect(localIPURL); });		   // android captive portal redirect
                server.on("/redirect", [](AsyncWebServerRequest *request) {printRequestDetails(request); request->redirect(localIPURL); });			   // microsoft redirect
                server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) {printRequestDetails(request); request->redirect(localIPURL); });  // apple call home
                server.on("/canonical.html", [](AsyncWebServerRequest *request) {printRequestDetails(request); request->redirect(localIPURL); });	   // firefox captive portal call home
                server.on("/success.txt", [](AsyncWebServerRequest *request) {printRequestDetails(request); request->send(200); });					   // firefox captive portal call home
                server.on("/ncsi.txt", [](AsyncWebServerRequest *request) {printRequestDetails(request); request->redirect(localIPURL); });


                server.onNotFound([](AsyncWebServerRequest *request){
                  //request->send(200, "text/html", "<h1>Page Not Found, but here is the captive portal!</h1>");
                  printRequestDetails(request);
                  request->redirect("/");
                });
                ElegantOTA.begin(&server,"admin","apocalypse");
                ws.onEvent(webSocketEvent);
                server.addHandler(&ws);
                // Start the web server
                server.begin();
                
                inCom.registerListenerFunction("websocket",websocketListener);
                  
              break;}
            //end
              case 3:{
                WiFi.softAPdisconnect(true);
              break;}

          }
        break;}
      //reset
        case 11:{
          esp_restart();
        break;}
      //ducky
        case 12:{
          //file
            if(!SPIFFS.begin(true)){
              inCom.println("An Error has occurred while mounting SPIFFS");
              return;
            }
            
            if(SPIFFS.exists(vc(commandVector,1).c_str())){
              fs::File runFile=SPIFFS.open(vc(commandVector,1).c_str());
              while(runFile.available()){
                String startCommand = runFile.readStringUntil('\n');
                startCommand.trim();
                inCom.println(startCommand,green);
                startCommand="/usb "+startCommand;
                identifyCommand(startCommand);
                inCom.flush(false);
              }
              runFile.close();
            }else{
              String fileLocation="/sdcard"+vc(commandVector,1);        
              FILE *SDrunFile = fopen(fileLocation.c_str(), "r");
              if (SDrunFile != NULL) {
                String fileContent;
                char ch;
                
                // Read the entire file into a String
                while ((ch = fgetc(SDrunFile)) != char(-1)) {
                  if(ch=='\n'){
                    fileContent.trim();  // Remove any leading or trailing whitespace
                    inCom.println(fileContent, green);
                    fileContent="/usb "+fileContent;
                    identifyCommand(fileContent);
                    fileContent.clear();
                    inCom.flush(false);
                  }else{
                    fileContent += ch;
                  }
                }
                if(!fileContent.isEmpty()){
                  fileContent.trim();  // Remove any leading or trailing whitespace
                  inCom.println(fileContent, green);
                  fileContent="/usb "+fileContent;
                  identifyCommand(fileContent);
                  fileContent.clear();
                  inCom.flush(false);
                }
                fclose(SDrunFile);
              }else{
                inCom.print("file not found at: ",red);
                inCom.println(vc(commandVector,1),red);
              }
            }
              
       break;}
       //sducky
        case 13:{
          //file
            if(!SPIFFS.begin(true)){
              inCom.println("An Error has occurred while mounting SPIFFS");
              return;
            }
            
            if(SPIFFS.exists(vc(commandVector,1).c_str())){
              fs::File runFile=SPIFFS.open(vc(commandVector,1).c_str());
              while(runFile.available()){
                String startCommand = runFile.readStringUntil('\n');
                startCommand.trim();
                inCom.println(startCommand,green);
                startCommand="/espnow cmd /usb "+startCommand;
                identifyCommand(startCommand);
                inCom.flush(false);
              }
              runFile.close();
            }else{
              String fileLocation="/sdcard"+vc(commandVector,1);        
              FILE *SDrunFile = fopen(fileLocation.c_str(), "r");
              if (SDrunFile != NULL) {
                String fileContent;
                char ch;
                
                // Read the entire file into a String
                while ((ch = fgetc(SDrunFile)) != char(-1)) {
                  if(ch=='\n'){
                    fileContent.trim();  // Remove any leading or trailing whitespace
                    inCom.println(fileContent, green);
                    fileContent="/espnow cmd /usb "+fileContent;
                    identifyCommand(fileContent);
                    fileContent.clear();
                    inCom.flush(false);
                  }else{
                    fileContent += ch;
                  }
                }
                if(!fileContent.isEmpty()){
                  fileContent.trim();  // Remove any leading or trailing whitespace
                  inCom.println(fileContent, green);
                  fileContent="/espnow cmd /usb "+fileContent;
                  identifyCommand(fileContent);
                  fileContent.clear();
                  inCom.flush(false);
                }
                fclose(SDrunFile);
              }else{
                inCom.print("file not found at: ",red);
                inCom.println(vc(commandVector,1),red);
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
  receiveResult=status;
  xTaskNotifyGive(mainTaskHandle);

}
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&espnowRecieved, incomingData, sizeof(espnowRecieved));
  memcpy(lastReceivedMac,mac,6);
  if(debug){
    inCom.print(espnowRecieved.msgOrder);
    inCom.print("  ");
    inCom.println(espnowRecieved.msgID,cyan);
  }
  if(espnowRecieved.msgOrder==-2){
    if(espnowReceiveSet==2){
      if(espnowRecieved.msgID==0){
        char recBufChar[230];
        sprintf(recBufChar,"~espnow {%02X:%02X:%02X:%02X:%02X:%02X ID:%d}---- %s"
        ,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],espnowRecieved.msgID,espnowRecieved.command);
        inCom.print(recBufChar);
        inCom.printFree();
      }else{
        char recBufChar[230];
        sprintf(recBufChar,"~espnow {%02X:%02X:%02X:%02X:%02X:%02X ID:%d}---- %s"
        ,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],espnowRecieved.msgID,espnowRecieved.command);
        inCom.println(recBufChar);
      }
    }
    if (espnowRecieved.msgID==2){
     commandQueue.push_back(espnowRecieved.command);
    }
  }else{

    if (espnowRecieved.msgOrder==-1){
      if(espnowmsgOrderLast!=-1){
        espnowCompiledMsg=espnowCompiledMsg+espnowRecieved.command;
        espnowmsgOrderLast=-1;
        if(espnowReceiveSet==2){
          
          
          if(espnowRecieved.msgID==0){
            char recBufChar[230];
            sprintf(recBufChar,"~espnow {%02X:%02X:%02X:%02X:%02X:%02X ID:%d}---- "
            ,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],espnowRecieved.msgID);

            inCom.print(recBufChar);
            inCom.print(espnowCompiledMsg);
            inCom.printFree();
          }else{
            char recBufChar[230];
            sprintf(recBufChar,"~espnow {%02X:%02X:%02X:%02X:%02X:%02X ID:%d}---- "
            ,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],espnowRecieved.msgID);

            inCom.print(recBufChar);
            inCom.println(espnowCompiledMsg);
          }
        }
        if (espnowRecieved.msgID==2){
          commandQueue.push_back(espnowCompiledMsg);
        }
      }else{
        inCom.println("message closer without body recieved",red);
      }
    }else{
      if(espnowRecieved.msgOrder-1==espnowmsgOrderLast){
        espnowCompiledMsg=espnowCompiledMsg+espnowRecieved.command;
        espnowmsgOrderLast=espnowmsgOrderLast+1;
      }else{
        
        inCom.println("non-consecutive message cell received",red);
      }
    }

    
  }
  
}
void printMac(uint8_t mac[6]){
  char macString[20];
  for (int i = 0; i < 6; i++) {
    sprintf(macString + 3 * i, "%02X:", mac[i]);
  }
  inCom.print(macString);
}
void printMac(uint8_t mac[6],const char color[15]){
  char macString[20];
  for (int i = 0; i < 6; i++) {
    sprintf(macString + 3 * i, "%02X:", mac[i]);
  }
  inCom.print(macString,color);
}
void espnowSendFunction(String data){
  //--------YOU CANT SEND MORE THAN 3 PACKETS FROM DATARECV---------
  espnowMessage.msgID=0;
  espnowMessage.msgOrder=0;
  if(data.length()<=200){
    strcpy(espnowMessage.command,data.c_str());
        
    espnowMessage.msgOrder=-2;
    esp_err_t result=esp_now_send( peerInfoArray[9].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
  }else{
    for(int i=data.length();i>=0;){
      espnowMessage.msgID=0;
      if(i<200){
        strcpy(espnowMessage.command,data.substring(data.length()-i).c_str());
        
        espnowMessage.msgOrder=-1;
        esp_err_t result=esp_now_send( peerInfoArray[9].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
        
        i=-1;
      }else{
        strcpy(espnowMessage.command,data.substring(data.length()-i,data.length()-i+200).c_str());
        i=i-200;
        
        esp_err_t result=esp_now_send( peerInfoArray[9].peer_addr, (uint8_t *) &espnowMessage, sizeof(espnowMessage));
        espnowMessage.msgOrder=espnowMessage.msgOrder+1;
        
      }
    }
  }
}
void websocketListener(String data){
  ws.textAll(data);
}
String vc(std::vector<String> vector,int index){
  if(vector.size()>index){
    return vector.at(index);
  }else{
    if(debug){inCom.println("vector check failed!",red);}
    return"";
  }
}

