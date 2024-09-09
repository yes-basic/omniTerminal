#ifndef serialCommand_h
#define serialCommand_h


#include "Arduino.h"
#ifdef USE_BTclassic
  #include "BluetoothSerial.h"
#endif
#include "SPIFFS.h"
#include <vector>

class serialCommand
{
  public:
    serialCommand();
    int serialState=0; // 0=no printing 1=printing println works as the prompt for the user
    int previousSerialState=0;
    void tryEspnowSend(char packet[200]);
    bool debug;
    bool check();
    String getCommand();
    void flush(bool flushAll);
    void parseCommandArray(String inputCommandString,bool addons);
    char commandArray[20][20];
    String commandString;
    String addonString;
    int wordsInCommand;
    int multiComp(char command[20],char staticArray[50][20]);
    int multiComp(String command,char staticArray[50][20]);
    bool isValidLong(char* str);
    bool isValidHex(const char* str);
    char addonArray[20][20];
    String bufString;
    void compileCharArray(char array[][20],int rows,char *string[]);
    #ifdef USE_BTclassic
    BluetoothSerial SerialBT;
    #endif
    void noRec(const char usingIndex[20]);
    void send(String data);
    void clearCMD();
    void reprintCMD();
    //serial replacements
      //printing overloads
        void print();void println();
        void print(int v);void println(int v);
        void print(char v[]);void println(char v[]);
        void print(char v);void println(char v);
        void print(const char v[]);void println(const char v[]);
        void print(long v);void println(long v);
        void print(String v);void println(String v);

        void print(int v,const char color[15]);void println(int v,const char color[15]);
        void print(char v[],const char color[15]);void println(char v[],const char color[15]);
        void print(char v,const char color[15]);void println(char v,const char color[15]);
        void print(const char v[],const char color[15]);void println(const char v[],const char color[15]);
        void print(long v,const char color[15]);void println(long v,const char color[15]);
        void print(String v,const char color[15]);void println(String v,const char color[15]);

        void printFree();
        char baseColor[15]="\033[37m";
        char userColor[15]="\033[36m";
        char altUserColor[15]="\033[33m";
        void write(int v);
        typedef void (*listenerFunctionPtr)(String data);
        void registerListenerFunction(const String& name, listenerFunctionPtr function);
        String sendString;
        typedef std::function<void(const String&)> listenerFunction;
        struct listenerFunctionRegistryEntry
        {
          String name;
          listenerFunction function;
        };
        std::vector<listenerFunctionRegistryEntry> listenerFunctionRegistry;
      bool available();
  private:
    char inChar;
    
    
};

#endif