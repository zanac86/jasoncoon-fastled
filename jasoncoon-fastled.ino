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

//#include <avr/wdt.h>

//#define FASTLED_ALLOW_INTERRUPTS 1
//#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <FastLED.h>
FASTLED_USING_NAMESPACE

#include "GradientPalettes.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// esp8266
#define DATA_PIN      2

// micro
//#define DATA_PIN      10

// это в маленькой лампе и полосках по 30 штук
// у в круглых платах так же
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB



// маленькая лампа с одним кусокм на 60 штук
//#define LED_TYPE      WS2813
//#define COLOR_ORDER   BRG

//#define NUM_LEDS      12
//#define NUM_LEDS      20
#define NUM_LEDS      30
//#define NUM_LEDS      60
//#define NUM_LEDS      64
//#define NUM_LEDS      256

#define MILLI_AMPS         2500 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  60  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.

String nameString;

CRGB leds[NUM_LEDS];

const uint8_t brightnessCount = 3;
uint8_t brightnessMap[brightnessCount] = { 10, 100, 250 };
uint8_t brightnessIndex = 0;

uint8_t brightness = 150;

// кнопка для micro
//#define BUTTON_PIN 2
// так на nodemcu
#define BUTTON_PIN 4
#include "GyverButton.h"
GButton touch(BUTTON_PIN, LOW_PULL, NORM_OPEN);


// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
uint8_t secondsPerPalette = 30;

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
uint8_t cooling = 49;

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
uint8_t sparking = 60;

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

uint8_t currentPatternIndex = 27; // Index number of which pattern is current

unsigned long autoplayDuration = 100;
unsigned long autoPlayTimeout = 0;

uint8_t currentPaletteIndex = 0;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

CRGB solidColor = CRGB::Blue;

// scale the brightness of all pixels down
void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i].nscale8(value);
  }
}

typedef void (*Pattern)();
typedef Pattern PatternList[];

#include "Twinkles.h"
#include "TwinkleFOX.h"
#include "PridePlayground.h"
#include "ColorWavesPlayground.h"

// List of patterns to cycle through.  Each is defined as a separate function below.

PatternList patterns =
{
  pride, // 0
  colorWaves, // 1
  pridePlayground, // 2
  colorWavesPlayground, // 3
  rainbowTwinkles, // 4
  snowTwinkles, // 5
  cloudTwinkles, // 6
  incandescentTwinkles, // 7
  retroC9Twinkles, // 8
  redWhiteTwinkles, // 9
  blueWhiteTwinkles, // 10
  redGreenWhiteTwinkles, // 11
  fairyLightTwinkles, // 12
  snow2Twinkles, // 13
  hollyTwinkles, // 14
  iceTwinkles, // 15
  partyTwinkles, // 16
  forestTwinkles, // 17
  lavaTwinkles, // 18
  fireTwinkles, // 19
  cloud2Twinkles, // 20
  oceanTwinkles, // 21
  rainbow, // 22
  rainbowWithGlitter, // 23
  rainbowSolid, // 24
  confetti, // 25
  sinelon, // 26
  bpm, // 27
  juggle, //28
  fire, // 29
  water, // 30
  showSolidColor, // 31
};

const uint8_t patternCount = ARRAY_SIZE(patterns);

uint8_t patternUsed[patternCount];

const CRGBPalette16 palettes[] =
{
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

void setup()
{
  wdt_reset();
  wdt_disable();

  Serial.begin(115200);


  randomSeed(analogRead(0));

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);         // for WS2812 (Neopixel)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  FastLED.setBrightness(brightness);

  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(500);

  for (uint8_t i = 0; i < patternCount; i++)
  {
    patternUsed[i] = i;
  }

  adjustBrightness();
  nextPattern();
  autoPlayTimeout = millis() + (autoplayDuration * 1000);

  FastLED.show();

  wdt_enable(WDTO_8S);

}

void testTouchClicks()
{
  if (touch.hasClicks())
  {
    byte clicks = touch.getClicks();
    switch (clicks)
    {
      case 1:
        adjustBrightness();
        break;
      case 2:
        nextPattern();
        break;
      case 3:
        nextPatternIndex(22);
        break;
    }
  }
}

void loop()
{
  wdt_reset();
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(65535));

  //  change to a new cpt - city gradient palette
  EVERY_N_SECONDS(secondsPerPalette)
  {
    gCurrentPaletteNumber = addmod8(gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  EVERY_N_SECONDS(secondsPerPalette)
  {
    // currentPaletteIndex = (currentPaletteIndex + 1) % paletteCount;
    currentPaletteIndex = random(0, paletteCount);
  }

  EVERY_N_MILLISECONDS(40)
  {
    // slowly blend the current palette to the next
    nblendPaletteTowardPalette(gCurrentPalette, gTargetPalette, 8);
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  if (millis() > autoPlayTimeout)
  {
    nextPattern();
  }

  touch.tick();
  testTouchClicks();

  // Call the current pattern function once, updating the 'leds' array
  patterns[currentPatternIndex]();

  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void setSolidColor(CRGB color)
{
  setSolidColor(color.r, color.g, color.b);
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b)
{
  solidColor = CRGB(r, g, b);
  setPattern(patternCount - 1);
}

// increase or decrease the current pattern number, and wrap around at the ends
void nextPattern()
{
  /*
    currentPatternIndex++;
    if (currentPatternIndex >= patternCount)
    currentPatternIndex = 0;
  */

  currentPatternIndex = random(0, patternCount);
  patternUsed[currentPatternIndex] = true;
  autoPlayTimeout = millis() + (autoplayDuration * 1000);
}

void nextPatternIndex(uint8_t i)
{
  currentPatternIndex = i;
  patternUsed[currentPatternIndex] = true;
  autoPlayTimeout = millis() + (autoplayDuration * 1000);
}

void setPattern(uint8_t value)
{
  if (value >= patternCount)
  {
    value = patternCount - 1;
  }

  currentPatternIndex = value;
}

void setPalette(uint8_t value)
{
  if (value >= paletteCount)
  {
    value = paletteCount - 1;
  }

  currentPaletteIndex = value;
}

void adjustBrightness()
{
  brightnessIndex = (brightnessIndex + 1) % brightnessCount;
  brightness = brightnessMap[brightnessIndex];
  FastLED.setBrightness(brightness);
}

void strandTest()
{
  static uint8_t i = 0;

  EVERY_N_SECONDS(1)
  {
    i++;
    if (i >= NUM_LEDS)
    {
      i = 0;
    }
  }

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  leds[i] = solidColor;
}

void showSolidColor()
{
  fill_solid(leds, NUM_LEDS, solidColor);
}

// Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void rainbowSolid()
{
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  // leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(speed, 0, NUM_LEDS);
  static int prevpos = 0;
  CRGB color = ColorFromPalette(palettes[currentPaletteIndex], gHue, 255);
  if (pos < prevpos)
  {
    fill_solid(leds + pos, (prevpos - pos) + 1, color);
  }
  else
  {
    fill_solid(leds + prevpos, (pos - prevpos) + 1, color);
  }
  prevpos = pos;
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t beat = beatsin8(speed, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle()
{
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand)   // Debounce to make sure we're not repeating an assignment.
  {
    lastSecond = secondHand;
    switch (secondHand)
    {
      case  0:
        numdots = 1;
        basebeat = 20;
        hueinc = 16;
        faderate = 2;
        thishue = 0;
        break; // You can change values here, one at a time , or altogether.
      case 10:
        numdots = 4;
        basebeat = 10;
        hueinc = 16;
        faderate = 8;
        thishue = 128;
        break;
      case 20:
        numdots = 8;
        basebeat =  3;
        hueinc =  0;
        faderate = 8;
        thishue = random8();
        break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30:
        break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for (int i = 0; i < numdots; i++)
  {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

void fire()
{
  heatMap(HeatColors_p, true);
}

void water()
{
  heatMap(IceColors_p, false);
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0 ; i < NUM_LEDS; i++)
  {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
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

void radialPaletteShift()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    // leds[i] = ColorFromPalette( gCurrentPalette, gHue + sin8(i*16), brightness);
    leds[i] = ColorFromPalette(gCurrentPalette, i + gHue, 255, LINEARBLEND);
  }
}

// based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino
void heatMap(CRGBPalette16 palette, bool up)
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    heat[i] = qsub8(heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (uint16_t k = NUM_LEDS - 1; k >= 2; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < sparking)
  {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (uint16_t j = 0; j < NUM_LEDS; j++)
  {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    colorindex = scale8(heat[j], 190);

    CRGB color = ColorFromPalette(palette, colorindex);

    if (up)
    {
      leds[j] = color;
    }
    else
    {
      leds[(NUM_LEDS - 1) - j] = color;
    }
  }
}

void addGlitter(uint8_t chanceOfGlitter)
{
  if (random8() < chanceOfGlitter)
  {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

uint8_t beatsaw8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255,
                 uint32_t timebase = 0, uint8_t phase_offset = 0)
{
  uint8_t beat = beat8(beats_per_minute, timebase);
  uint8_t beatsaw = beat + phase_offset;
  uint8_t rangewidth = highest - lowest;
  uint8_t scaledbeat = scale8(beatsaw, rangewidth);
  uint8_t result = lowest + scaledbeat;
  return result;
}

void colorWaves()
{
  colorwaves(leds, NUM_LEDS, gCurrentPalette);
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves(CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0 ; i < numleds; i++)
  {
    hue16 += hueinc16;
    uint16_t h16_128 = hue16 >> 7;
    uint8_t hue8;
    if (h16_128 & 0x100)
    {
      hue8 = 255 - (h16_128 >> 1);
    }
    else
    {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
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
