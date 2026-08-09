/*
 WallEDisplay v0.2 skeleton
 NOTE: Generated from design discussion.
*/

#include <FastLED.h>
#include <SoftwareSerial.h>
#include <DYPlayerArduino.h>

#define LED_PIN 2
#define RESTART_PIN 3
#define ENABLE_PIN 4
#define MOTION_PIN 5
#define DY_RX 8
#define DY_TX 9

#define NUM_LEDS 51
CRGB leds[NUM_LEDS];

SoftwareSerial audioSerial(DY_RX,DY_TX);
DY::Player player(&audioSerial);

const CRGB CHARGE=CRGB(128,128,0);

enum State {STARTUP,IDLE,LOCKOUT};
State state=STARTUP;

bool motionEnabled=true;
bool motionLatch=false;

unsigned long tmr=0,lockoutStart=0;
byte stage=0;
byte currentBar=0;

const unsigned long STEP_MS=500;
const unsigned long LOCKOUT_MS=60000UL;

void clearDisplay(){FastLED.clear();FastLED.show();}
void lightBar(byte b){for(byte i=0;i<4;i++) leds[b*4+i]=CHARGE; FastLED.show();}
void lightLevel(){for(byte i=40;i<=46;i++) leds[i]=CHARGE; FastLED.show();}
void lightSun(){for(byte i=47;i<=50;i++) leds[i]=CHARGE; FastLED.show();}

void beginStartup(){
  player.stop();
  clearDisplay();
  currentBar=0;
  stage=0;
  state=STARTUP;
  player.playSpecified(1);
  tmr=millis();
}

void finishStartup(){
  player.stop();
  player.playSpecified(2);
  state=IDLE;
}

void setup(){
  pinMode(RESTART_PIN,INPUT_PULLUP);
  pinMode(ENABLE_PIN,INPUT_PULLUP);
  pinMode(MOTION_PIN,INPUT);

  FastLED.addLeds<WS2812B,LED_PIN,GRB>(leds,NUM_LEDS);
  clearDisplay();

  audioSerial.begin(9600);
  delay(500);
  player.begin();

  motionEnabled = digitalRead(ENABLE_PIN); // LOW=switch fitted=>disable
  beginStartup();
}

void loop(){

  if(digitalRead(RESTART_PIN)==LOW){
    while(digitalRead(RESTART_PIN)==LOW);
    beginStartup();
  }

  switch(state){

    case STARTUP:
      if(millis()-tmr>=STEP_MS){
        tmr=millis();
        if(currentBar<10){
          lightBar(currentBar++);
        }else if(stage==0){
          lightLevel();
          stage=1;
        }else if(stage==1){
          lightSun();
          stage=2;
          finishStartup();
        }
      }
    break;

    case IDLE:

      if(!motionEnabled) break;

      if(digitalRead(MOTION_PIN) && !motionLatch){
        motionLatch=true;
        player.playSpecified(random(3,18));
        lockoutStart=millis();
        state=LOCKOUT;
      }
      if(!digitalRead(MOTION_PIN)) motionLatch=false;
    break;

    case LOCKOUT:
      if(millis()-lockoutStart>=LOCKOUT_MS){
        motionLatch=false;
        state=IDLE;
      }
    break;
  }
}
