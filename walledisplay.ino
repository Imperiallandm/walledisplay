/*
  ============================================================
  WallEDisplay
  Version : 0.1
  Board   : Arduino Nano
  ============================================================

  PIN ASSIGNMENTS

  D2  -> WS2812B data
  D3  -> Restart button (to GND)
  D5  -> RCWL0516 OUT

  D8  -> DYPlayer RX (Arduino RX - unused)
  D9  -> DYPlayer TX (Arduino TX -> DYPlayer RX)

  LED LAYOUT

  LED 0-39   = 10 x bars (4 LEDs each)
  LED 40-46  = Level (7 LEDs)
  LED 47-50  = Sun (4 LEDs)

  STARTUP

  - All LEDs OFF
  - Play track 1
  - Every 500ms illuminate another bar
  - Level illuminates
  - Sun illuminates
  - Stop track 1
  - Play track 2
  - Enter IDLE

  MOTION

  - RCWL ignored during startup
  - Motion after startup plays random track 3-17
  - 60 second lockout after trigger

  RESTART

  - D3 LOW restarts entire sequence

  No delay() is used during normal operation.
  ============================================================
*/

#include <FastLED.h>
#include <SoftwareSerial.h>
#include <DYPlayerArduino.h>


// ============================================================
// PIN DEFINITIONS
// ============================================================

#define LED_PIN       2
#define RESTART_PIN   3
#define MOTION_PIN    5

#define DY_RX         8
#define DY_TX         9


// ============================================================
// LED SETTINGS
// ============================================================

#define NUM_LEDS 51

CRGB leds[NUM_LEDS];


// ============================================================
// LED LAYOUT
// ============================================================

const byte BAR_COUNT = 10;
const byte LEDS_PER_BAR = 4;

const byte LEVEL_START_LED = 40;
const byte LEVEL_LED_COUNT = 7;

const byte SUN_START_LED = 47;
const byte SUN_LED_COUNT = 4;


// ============================================================
// LED COLOUR
// ============================================================

// Half brightness yellow

const CRGB CHARGE_COLOUR = CRGB(128, 128, 0);


// ============================================================
// TIMING
// ============================================================

const unsigned long BAR_INTERVAL = 500;

const unsigned long LEVEL_INTERVAL = 500;

const unsigned long SUN_INTERVAL = 500;

const unsigned long MOTION_TIMEOUT = 60000UL;


// ============================================================
// AUDIO TRACKS
// ============================================================

const byte TRACK_STARTUP = 1;

const byte TRACK_COMPLETE = 2;

const byte RANDOM_TRACK_FIRST = 3;

const byte RANDOM_TRACK_LAST = 17;


// ============================================================
// DYPLAYER
// ============================================================

SoftwareSerial audioSerial(DY_RX, DY_TX);

DY::Player player(&audioSerial);


// ============================================================
// SYSTEM STATES
// ============================================================

enum SystemState
{
  STARTUP,
  IDLE,
  MOTION_LOCKOUT
};

SystemState currentState = STARTUP;


// ============================================================
// STARTUP VARIABLES
// ============================================================

byte currentBar = 0;

byte startupStage = 0;

unsigned long startupTimer = 0;


// ============================================================
// MOTION VARIABLES
// ============================================================

unsigned long motionTimer = 0;

bool motionLatch = false;


// ============================================================
// BUTTON VARIABLES
// ============================================================

bool lastRestartState = HIGH;


// ============================================================
// SETUP
// ============================================================

void setup()
{
  pinMode(RESTART_PIN, INPUT_PULLUP);

  pinMode(MOTION_PIN, INPUT);


  // LEDs

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  FastLED.clear();

  FastLED.show();


  // Random seed

  randomSeed(analogRead(A0));


  // DYPlayer

  audioSerial.begin(9600);

  delay(500);

  player.begin();


  // Start sequence

  beginStartup();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  checkRestart();


  switch (currentState)
  {
    case STARTUP:

      updateStartup();

      break;


    case IDLE:

      updateIdle();

      break;


    case MOTION_LOCKOUT:

      updateMotionLockout();

      break;
  }
}


// ============================================================
// START STARTUP SEQUENCE
// ============================================================

void beginStartup()
{
  player.stop();

  clearDisplay();


  currentBar = 0;

  startupStage = 0;

  motionLatch = false;


  currentState = STARTUP;


  // Play startup sound

  player.playSpecified(TRACK_STARTUP);


  startupTimer = millis();
}


// ============================================================
// UPDATE STARTUP
// ============================================================

void updateStartup()
{
  unsigned long now = millis();


  // ----------------------------------------------------------
  // BATTERY BARS
  // ----------------------------------------------------------

  if (currentBar < BAR_COUNT)
  {
    if (now - startupTimer >= BAR_INTERVAL)
    {
      lightBar(currentBar);

      currentBar++;

      startupTimer = now;
    }

    return;
  }


  // ----------------------------------------------------------
  // LEVEL
  // ----------------------------------------------------------

  if (startupStage == 0)
  {
    if (now - startupTimer >= LEVEL_INTERVAL)
    {
      lightLevel();

      startupStage = 1;

      startupTimer = now;
    }

    return;
  }


  // ----------------------------------------------------------
  // SUN
  // ----------------------------------------------------------

  if (startupStage == 1)
  {
    if (now - startupTimer >= SUN_INTERVAL)
    {
      lightSun();

      startupStage = 2;

      finishStartup();
    }
  }
}


// ============================================================
// FINISH STARTUP
// ============================================================

void finishStartup()
{
  player.stop();

  player.playSpecified(TRACK_COMPLETE);

  currentState = IDLE;

  motionLatch = false;
}


// ============================================================
// IDLE / MOTION DETECTION
// ============================================================

void updateIdle()
{
  bool motion = digitalRead(MOTION_PIN);


  if (motion && !motionLatch)
  {
    motionLatch = true;

    playRandomMotionSound();

    motionTimer = millis();

    currentState = MOTION_LOCKOUT;
  }


  if (!motion)
  {
    motionLatch = false;
  }
}


// ============================================================
// MOTION LOCKOUT
// ============================================================

void updateMotionLockout()
{
  unsigned long now = millis();


  if (now - motionTimer >= MOTION_TIMEOUT)
  {
    motionLatch = false;

    currentState = IDLE;
  }
}


// ============================================================
// RANDOM MOTION SOUND
// ============================================================

void playRandomMotionSound()
{
  byte track = random(
    RANDOM_TRACK_FIRST,
    RANDOM_TRACK_LAST + 1
  );

  player.playSpecified(track);
}


// ============================================================
// LIGHT ONE BAR
// ============================================================

void lightBar(byte barNumber)
{
  byte firstLED = barNumber * LEDS_PER_BAR;


  for (byte i = 0; i < LEDS_PER_BAR; i++)
  {
    leds[firstLED + i] = CHARGE_COLOUR;
  }


  FastLED.show();
}


// ============================================================
// LIGHT LEVEL
// ============================================================

void lightLevel()
{
  for (byte i = 0; i < LEVEL_LED_COUNT; i++)
  {
    leds[LEVEL_START_LED + i] = CHARGE_COLOUR;
  }


  FastLED.show();
}


// ============================================================
// LIGHT SUN
// ============================================================

void lightSun()
{
  for (byte i = 0; i < SUN_LED_COUNT; i++)
  {
    leds[SUN_START_LED + i] = CHARGE_COLOUR;
  }


  FastLED.show();
}


// ============================================================
// CLEAR DISPLAY
// ============================================================

void clearDisplay()
{
  FastLED.clear();

  FastLED.show();
}


// ============================================================
// RESTART BUTTON
// ============================================================

void checkRestart()
{
  bool currentRestartState =
    digitalRead(RESTART_PIN);


  if (
    currentRestartState == LOW &&
    lastRestartState == HIGH
  )
  {
    beginStartup();
  }


  lastRestartState = currentRestartState;
}
