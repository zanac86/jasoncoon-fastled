/*
   ESP8266 FastLED WebServer: https://github.com/jasoncoon/esp8266-fastled-webserver
   Copyright (C) 2015-2018 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef __AVR__
#include <avr/wdt.h>
#endif

//#define FASTLED_ALLOW_INTERRUPTS 1
//#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <FastLED.h>
FASTLED_USING_NAMESPACE

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


#ifdef __AVR__
// micro
#define DATA_PIN 10
#define BUTTON_PIN 2
#else
// esp8266
// с лентой ws2813 заработал только выход 0
// если 2 - то зажигается только первый диод и все
#define DATA_PIN D4
#define BUTTON_PIN D2
#endif

// comment if no button
#define USE_BUTTON

// это в маленькой лампе и полосках по 30 штук
// у в круглых платах так же
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// маленькая лампа с одним кусокм на 60 штук
// #define LED_TYPE      WS2813
// #define COLOR_ORDER   BRG

// #define NUM_LEDS      12
// #define NUM_LEDS 30
//#define NUM_LEDS      144
// #define NUM_LEDS      30
 #define NUM_LEDS      300
//#define NUM_LEDS      64
//#define NUM_LEDS      256

#define MILLI_AMPS 3500       // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND 50  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.

String nameString;

CRGB leds[NUM_LEDS];

const uint8_t brightnessCount = 3;
uint8_t brightnessMap[brightnessCount] = { 20, 120, 240 };
uint8_t brightnessIndex = 0;

uint8_t brightness = 240;

#include "GyverButton.h"
GButton touch(BUTTON_PIN, LOW_PULL, NORM_OPEN);

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
const uint8_t secondsPerPalette = 25;

uint8_t speed = 30;

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];

uint8_t gCurrentPaletteNumber = 0;
CRGBPalette16 gCurrentPalette(CRGB::Black);
CRGBPalette16 gTargetPalette(gGradientPalettes[0]);
CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

uint8_t currentPatternIndex = 27;  // Index number of which pattern is current
uint8_t lastPatternIndex = currentPatternIndex;

// период смены эффектов
uint32_t autoplayDuration = 100*1000;
// текущее время запомнить для проверки
uint32_t autoPlayTime = 0;

uint8_t currentPaletteIndex = 0;
uint8_t gHue = 0;  // rotating "base color" used by many of the patterns
CRGB solidColor = CRGB::Blue;
CRGB solidColors[] = { CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Purple, CRGB::Yellow, CRGB::Cyan, CRGB::Magenta };
const uint8_t solidColorsCount = ARRAY_SIZE(solidColors);

void addGlitter(uint8_t chanceOfGlitter);
void colorwaves(CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette);
void colorWaves();
void rainbow();
void rainbowWithGlitter();
void rainbowSolid();
void confetti();
void sinelon();
void bpm();
void juggle();
void pride();
void showSolidColor();
void adjustBrightness();
void nextPattern();
void nextPatternIndex(uint8_t i);
void showNightLamp();

// scale the brightness of all pixels down
void dimAll(byte value) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value);
  }
}

typedef void (*Pattern)();
typedef Pattern PatternList[];

#include "Twinkles.h"
#include "TwinkleFOX.h"
#include "PridePlayground.h"
#include "ColorWavesPlayground.h"
#include "GradientPalettes.h"

// List of patterns to cycle through.  Each is defined as a separate function below.

PatternList patterns = {
  pride,                  // 0
  colorWaves,             // 1
  pridePlayground,        // 2
  colorWavesPlayground,   // 3
  rainbowTwinkles,        // 4
  snowTwinkles,           // 5
  cloudTwinkles,          // 6
  incandescentTwinkles,   // 7
  retroC9Twinkles,        // 8
  redWhiteTwinkles,       // 9
  blueWhiteTwinkles,      // 10
  redGreenWhiteTwinkles,  // 11
  fairyLightTwinkles,     // 12
  snow2Twinkles,          // 13
  hollyTwinkles,          // 14
  iceTwinkles,            // 15
  partyTwinkles,          // 16
  forestTwinkles,         // 17
  lavaTwinkles,           // 18
  fireTwinkles,           // 19
  cloud2Twinkles,         // 20
  oceanTwinkles,          // 21
  rainbow,                // 22
  rainbowWithGlitter,     // 23
  rainbowSolid,           // 24
  confetti,               // 25
  sinelon,                // 26
  bpm,                    // 27
  juggle,                 // 28
  showSolidColor,         // 29
  showNightLamp
};

const uint8_t patternCount = ARRAY_SIZE(patterns);

const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};

const uint8_t paletteCount = ARRAY_SIZE(palettes);

#ifndef __AVR__
#define RANDOM_REG32 ESP8266_DREG(0x20E44)
uint8_t random_esp8266(uint8_t upper) {
  return (RANDOM_REG32) % upper;
}
#endif

void setup() {
  wdt_reset();
  wdt_disable();

  Serial.begin(115200);  // Initialize serial port for debugging.
  delay(200);            // Soft startup to ease the flow of electrons.

  randomSeed(millis());

  Serial.println();

#ifndef __AVR__
  for (int i = 0; i < 10; i++) {
    uint32_t r = RANDOM_REG32;
    Serial.print(r);
    Serial.print(" ");
  }
  Serial.println();
#endif

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);  // for WS2812 (Neopixel)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  FastLED.clear();
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  Serial.println("\n\nTest RGB\n");
  // тест подключения и порядка цветов
  leds[0] = CRGB::Red;
  leds[1] = CRGB::Red;
  leds[2] = CRGB::Green;
  leds[3] = CRGB::Green;
  leds[4] = CRGB::Blue;
  leds[5] = CRGB::Blue;
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  delay(200);

  FastLED.clear();
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  randomSeed(random(1000, 5000));

  // можно и убрать
  adjustBrightness();


  // начать со случайного эффекта
  // там внутри тоже обновится autoPlayTime на текущее время
  nextPattern();
  autoPlayTime = millis();

  FastLED.clear();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  wdt_enable(WDTO_8S);
}

void testTouchClicks() {
  if (touch.hasClicks()) {
    byte clicks = touch.getClicks();
    switch (clicks) {
      case 1:
        nextPattern();
        break;
      case 2:
        adjustBrightness();
        break;
      case 3:
        nextPatternIndex(30);
        break;
      case 4:
        nextPatternIndex(22);
        break;
    }
  }
}

void loop() {
  wdt_reset();

  EVERY_N_SECONDS(5) {
    random16_add_entropy(random(65535));
  }

  EVERY_N_SECONDS(secondsPerPalette) {
    gCurrentPaletteNumber = addmod8(gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[gCurrentPaletteNumber];
  }

  EVERY_N_SECONDS(secondsPerPalette) {
    // currentPaletteIndex = (currentPaletteIndex + 1) % paletteCount;
    currentPaletteIndex = random(0, paletteCount);
  }

  EVERY_N_MILLISECONDS(40) {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette(gCurrentPalette, gTargetPalette, 8);
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if (((uint32_t)millis()) - autoPlayTime > autoplayDuration) {
    // там внутри обновится autoPlayTime на текущее время
    nextPattern();
  }

#ifdef USE_BUTTON
  touch.tick();
  testTouchClicks();
#endif

  // Call the current pattern function once, updating the 'leds' array
  patterns[currentPatternIndex]();

  // FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void nextPattern() {
  uint8_t newPatternIndex = random(patternCount - 2);  // без двух последних - сплошной цвет и ночник
  if (newPatternIndex == lastPatternIndex) {
    newPatternIndex = random(patternCount);
  }
  nextPatternIndex(newPatternIndex);
}

void nextPatternIndex(uint8_t i) {
  currentPatternIndex = i;
  lastPatternIndex = currentPatternIndex;
  Serial.print("New pattern index ");
  Serial.println(currentPatternIndex);
  autoPlayTime = millis();
  solidColor = solidColors[random(solidColorsCount)];
}

void setPalette(uint8_t value) {
  if (value >= paletteCount) {
    value = paletteCount - 1;
  }

  currentPaletteIndex = value;
}

void adjustBrightness() {
  brightnessIndex = (brightnessIndex + 1) % brightnessCount;
  brightness = brightnessMap[brightnessIndex];
  FastLED.setBrightness(brightness);
}

void strandTest() {
  static uint8_t i = 0;

  EVERY_N_SECONDS(1) {
    i++;
    if (i >= NUM_LEDS) {
      i = 0;
    }
  }

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  leds[i] = solidColor;
}

void showSolidColor() {
  fill_solid(leds, NUM_LEDS, solidColor);
}

void showNightLamp() {
  FastLED.clear();
  uint16_t n = NUM_LEDS / 5;
  uint16_t i = 0;
  while (i < NUM_LEDS) {
    leds[i] = CHSV(128, 50, 150);
    i = i + n;
  }
  FastLED.show();
}

// Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
}

void rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void rainbowSolid() {
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  // leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(speed, 0, NUM_LEDS);
  static int prevpos = 0;
  CRGB color = ColorFromPalette(palettes[currentPaletteIndex], gHue, 255);
  if (pos < prevpos) {
    fill_solid(leds + pos, (prevpos - pos) + 1, color);
  } else {
    fill_solid(leds + prevpos, (pos - prevpos) + 1, color);
  }
  prevpos = pos;
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t beat = beatsin8(speed, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle() {
  static uint8_t numdots = 4;                 // Number of dots in use.
  static uint8_t faderate = 2;                // How long should the trails be. Very low value = longer trails.
  static uint8_t hueinc = 255 / numdots - 1;  // Incremental change in hue between each dot.
  static uint8_t thishue = 0;                 // Starting hue.
  static uint8_t curhue = 0;                  // The current hue
  static uint8_t thissat = 255;               // Saturation of the colour.
  static uint8_t thisbright = 255;            // How bright should the LED/display be.
  static uint8_t basebeat = 5;                // Higher = faster movement.

  static uint8_t lastSecond = 99;               // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30;  // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand)  // Debounce to make sure we're not repeating an assignment.
  {
    lastSecond = secondHand;
    switch (secondHand) {
      case 0:
        numdots = 1;
        basebeat = 20;
        hueinc = 16;
        faderate = 2;
        thishue = 0;
        break;  // You can change values here, one at a time , or altogether.
      case 10:
        numdots = 4;
        basebeat = 10;
        hueinc = 16;
        faderate = 8;
        thishue = 128;
        break;
      case 20:
        numdots = 8;
        basebeat = 3;
        hueinc = 0;
        faderate = 8;
        thishue = random8();
        break;  // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30:
        break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue;  // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for (int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;  //gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV(hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 64);
  }
}

void radialPaletteShift() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( gCurrentPalette, gHue + sin8(i*16), brightness);
    leds[i] = ColorFromPalette(gCurrentPalette, i + gHue, 255, LINEARBLEND);
  }
}

void addGlitter(uint8_t chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

uint8_t beatsaw8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255,
                 uint32_t timebase = 0, uint8_t phase_offset = 0) {
  uint8_t beat = beat8(beats_per_minute, timebase);
  uint8_t beatsaw = beat + phase_offset;
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8(beatsaw, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

void colorWaves() {
  colorwaves(leds, NUM_LEDS, gCurrentPalette);
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves(CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;  //gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0; i < numleds; i++) {
    hue16 += hueinc16;
    uint16_t h16_128 = hue16 >> 7;
    uint8_t hue8;
    if (h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8(index, 240);

    CRGB newcolor = ColorFromPalette(palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds - 1) - pixelnumber;

    nblend(ledarray[pixelnumber], newcolor, 128);
  }
}
