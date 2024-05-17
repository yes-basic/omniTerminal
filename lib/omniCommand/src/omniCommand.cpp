#include "omniCommand.h"
#include <Arduino.h>

omniCommand::omniCommand(){


}
omniCommand::omniCommand(String inputString){
set(inputString);
}
omniCommand::omniCommand(char inputCharArray[200]){
set(inputCharArray);
}
omniCommand::omniCommand(char inputWordArray[20][50]){
set(inputWordArray);
}

void omniCommand::set(String inputString){
  string=inputString;
  strcpy(charArray,string.c_str());

  int wordIndex = 0;
  int wordLength = 0;
  

  for (int i = 0; i < string.length(); i++) {
      char c = string.charAt(i);

      // If current character is not a space, add it to current word
      if (c != ' ' && c != '\0') {
          wordArray[wordIndex][wordLength++] = c;
      } else {
          wordArray[wordIndex++][wordLength] = '\0';  // Null-terminate the word
          wordLength = 0;  // Reset word length for next word
      }

      // Break if we have reached the maximum number of words
      if (wordIndex >= sizeof(wordArray)/sizeof(wordArray[0])) {
          //println("command array overflow");
          //break;
      }

  }
  wordArray[wordIndex][wordLength] = '\0';  // Null-terminate the word
  wordLength = 0;  // Reset word length for next word

  wordCount=wordIndex+1;
  letterCount=string.length();
}
void omniCommand::set(char inputCharArray[200]){
  string=inputCharArray;
  strcpy(charArray,inputCharArray);

  int wordIndex = 0;
  int wordLength = 0;
  

  for (int i = 0; i < string.length(); i++) {
      char c = string.charAt(i);

      // If current character is not a space, add it to current word
      if (c != ' ' && c != '\0') {
          wordArray[wordIndex][wordLength++] = c;
      } else {
          wordArray[wordIndex++][wordLength] = '\0';  // Null-terminate the word
          wordLength = 0;  // Reset word length for next word
      }

      // Break if we have reached the maximum number of words
      if (wordIndex >= sizeof(wordArray)/sizeof(wordArray[0])) {
          //println("command array overflow");
          //break;
      }

  }

  wordArray[wordIndex][wordLength] = '\0';  // Null-terminate the word
  wordLength = 0;  // Reset word length for next word

  wordCount=wordIndex+1;
  letterCount=string.length();
}

void omniCommand::set(char inputWordArray[20][50]){
  for(int i=0;i<50;i++){
    strcpy(wordArray[i],inputWordArray[i]);
  }
  strcpy(charArray,wordArray[0]);
  for(int i=1;i<20;i++){
    if(strcmp(wordArray[i],"")){
      strcat(charArray," ");
      strcat(charArray,wordArray[i]);
    }
  }
  wordCount=0;
  for(int i=0;i<50;i++){
    if(strcmp(wordArray[i],"")==0){
      wordCount++;
    }
  }
  string=charArray;
  letterCount=string.length();
}