#include <Arduino.h>

#include "SPI.h"
#include <TFT_eSPI.h>

#define IR_RECEIVE_PIN          15  // D15
#define IR_SEND_PIN              21  // D4
#define TONE_PIN                27  // D27 25 & 26 are DAC0 and 1
#define APPLICATION_PIN         16  // RX2 pin

#include <IRremote.hpp>
void setup() {
Serial.begin(115200);
IrSender.begin(DISABLE_LED_FEEDBACK);
}

void loop() {
IrSender.sendNEC(0x1, 0x40, 0);
delay(500);
}

