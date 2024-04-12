#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

USBHIDKeyboard Keyboard;

void setup() {
  Serial.begin(115200);
  pinMode(21,OUTPUT);
  Keyboard.begin();
  USB.begin();
//look at how fast this types, it just instantly prints whatever!look at how fast this types, it just instantly prints whatever!
  delay(500);
  Keyboard.print("look at how fast this types, it just instantly prints whatever!");
  Serial.println("sent");
  delay(5000);
  Keyboard.end();
  Serial.begin(115200);
}

void loop() {
digitalWrite(21,HIGH);
delay(500);
digitalWrite(21,LOW);
delay(500);
}