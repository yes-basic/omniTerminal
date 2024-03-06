#include <Arduino.h>
#include <IRremote.hpp>
#include "SPI.h"
#include <TFT_eSPI.h>

#define IR_RECEIVE_PIN          15  // D15
#define IR_SEND_PIN              21  // D4
#define TONE_PIN                27  // D27 25 & 26 are DAC0 and 1
#define APPLICATION_PIN         16  // RX2 pin
#include <IRremote.h>

const int RECV_PIN = 7;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup(){
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);
}

void loop(){
  if (irrecv.decode(&results)){
        Serial.println(results.value, HEX);
        switch (results.decode_type){
            case NEC: Serial.println("NEC"); break ;
            case SONY: Serial.println("SONY"); break ;
            case RC5: Serial.println("RC5"); break ;
            case RC6: Serial.println("RC6"); break ;
            //case DISH: Serial.println("DISH"); break ;
            case SHARP: Serial.println("SHARP"); break ;
            case JVC: Serial.println("JVC"); break ;
            //case SANYO: Serial.println("SANYO"); break ;
            //case MITSUBISHI: Serial.println("MITSUBISHI"); break ;
            case SAMSUNG: Serial.println("SAMSUNG"); break ;
            case LG: Serial.println("LG"); break ;
            case WHYNTER: Serial.println("WHYNTER"); break ;
            //case AIWA_RC_T501: Serial.println("AIWA_RC_T501"); break ;
            case PANASONIC: Serial.println("PANASONIC"); break ;
            case DENON: Serial.println("DENON"); break ;
          default:
            case UNKNOWN: Serial.println("UNKNOWN"); break ;
          }
        irrecv.resume();
 }
}
