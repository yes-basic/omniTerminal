#ifndef omniCommand_h
#define omniCommand_h
#include <Arduino.h>
#include <vector>

class omniCommand
{
  public:
    omniCommand();
    omniCommand(String inputString);
    omniCommand(char inputCharArray[200]);
    omniCommand(char inputWordArray[20][50]);

    void clear();

    void set(String inputString);
    void set(char inputCharArray[200]);
    void set(char inputWordArray[20][50]);


    String string;
    char charArray[200];
    char wordArray[20][50];
    std::vector<String> wordVector;
    int wordCount;
    int letterCount;

  private:
  void makeVector();


};




#endif