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

void omniCommand::clear(){
    string="";
    strcpy(charArray,"");
    for(int i=0;i<50;i++){
      strcpy(wordArray[i],"");
    }
    wordVector.clear();
    wordCount=0;
    letterCount=0;
}

void omniCommand::set(String inputString){
  clear();
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
          break;
      }

  }
  wordArray[wordIndex][wordLength] = '\0';  // Null-terminate the word
  wordLength = 0;  // Reset word length for next word

 
  letterCount=string.length();
  makeVector();
}
void omniCommand::set(char inputCharArray[200]){
  clear();
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
          break;
      }

  }

  wordArray[wordIndex][wordLength] = '\0';  // Null-terminate the word
  wordLength = 0;  // Reset word length for next word

  
  letterCount=string.length();
  makeVector();
}

void omniCommand::set(char inputWordArray[20][50]){
  clear();
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
  makeVector();
}

void omniCommand::makeVector(){

  int wordIndex = 0;
  
  wordVector.push_back("");
  for (int i = 0; i < string.length(); i++) {
      char c = string.charAt(i);

      // If current character is not a space, add it to current word
      if (c != ' ' && c != '\0') {
          wordVector[wordIndex] += c;
      } else {
          wordIndex++;
          wordVector.push_back("");
      }


  }
  //wordArray[wordIndex][wordLength] = '\0';  // Null-terminate the word
  //wordLength = 0;  // Reset word length for next word
 wordCount=wordVector.size();
}