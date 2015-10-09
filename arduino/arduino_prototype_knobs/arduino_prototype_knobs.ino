#include <avr/pgmspace.h>

// Pin definitions
#define RED_PIN    11
#define GREEN_PIN  9
#define BLUE_PIN   10
#define INPUT_PIN  2
#define KNOB_1     A2
#define KNOB_2     A1
#define KNOB_3     A0

// state definitions
#define OFF        0
#define RGB_MODE   1
#define HSL_MODE   2
#define FADE_HUE   3
#define FADE_VALUE 4
#define NUM_STATES 5

// Max duty cycles
#define MAX_RED    255
#define MAX_GREEN  255
#define MAX_BLUE   255
#define MAX_HUE    255
#define MAX_SAT    255
#define MAX_VAL    255

// Global variables
int counter = 0;
volatile uint8_t state = 3;
uint16_t hue = 0;
uint16_t val = 0;
extern const uint8_t PROGMEM gamma[];
long lastDebounceTime;
long debounceThreshold = 100;

void setup()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  
  // set up interrupt routine for mode button
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN), changeState, RISING);
  
  Serial.begin(9600);
  
  turnOff();
}

void loop()
{
  bool fadeUpVal = true;
  
  switch (state)
  {
    case RGB_MODE:
      analogWrite(RED_PIN, readKnob(KNOB_1));
      analogWrite(GREEN_PIN, readKnob(KNOB_2));
      analogWrite(BLUE_PIN, readKnob(KNOB_3));
      break;
      
    case HSL_MODE:
      uint8_t r, g, b;
      hsvtorgb(&r, &g, &b, readKnob(KNOB_1), readKnob(KNOB_2), readKnob(KNOB_3));
      displayRGBcorrected(r, g, b);
      break;
      
    case FADE_HUE:
      // enter loop until mode button is pressed
      while (state == FADE_HUE)
      {
        uint16_t hueRate = 260 - readKnob(KNOB_1);

        ++hue;
        if (hue >= MAX_HUE) hue = 0;
        
        hsvtorgb(&r, &g, &b, hue, 255, 255);
        displayRGBcorrected(r, g, b);
        delay(hueRate);
      }
      break;

    case FADE_VALUE:
      // enter loop until mode button is pressed
      while (state == FADE_VALUE)
      {
        uint16_t valRate = readKnob(KNOB_3) >> 5;

        if(fadeUpVal)
        {
          if (valRate > (MAX_VAL - val))
          {
            val = MAX_VAL;
            // change direction
            fadeUpVal = false;
          }
          else val += valRate;
        }
        else
        {
          if (val < valRate)
          {
            val = 1;
            // change direction
            fadeUpVal = true;
          }
          else val -= valRate;
        }
        
        hsvtorgb(&r, &g, &b, readKnob(KNOB_1), 255, val);
        displayRGBcorrected(r, g, b);
        delay(10);
      }
      break;
      
    case OFF:
      turnOff();
      break;
  }
}

uint8_t readKnob(uint8_t pin)
{
  /* analogRead returns value in (0,1023), shift right 2 bits to
     divide by 4 to get value in (0,255)
     The loop is used to poll multiple times to reduce flickering.
  */
  uint8_t readings[16];
  uint8_t counter = 0;
  uint8_t attempts = 0;
  
  while (counter < 7 && attempts < 4)
  {
    for (uint8_t i = 0; i < 16; i++)
    {
      readings[i] = analogRead(pin) >> 2;
      if (readings[i] == readings[0]) ++counter;
    }
    ++attempts;
  }
  return readings[0];
}

void displayRGBcorrected(uint8_t r, uint8_t g, uint8_t b)
{
  analogWrite(RED_PIN, pgm_read_byte(&gamma[r]));
  analogWrite(GREEN_PIN, pgm_read_byte(&gamma[g]));
  analogWrite(BLUE_PIN, pgm_read_byte(&gamma[b]));
}

void displayRGB(uint8_t r, uint8_t g, uint8_t b)
{
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

/* HSV to RGB conversion function with only integer
 * math */
 // TODO: saturation behaves strnagely...find out why and fix it
void hsvtorgb(uint8_t* r, uint8_t* g, uint8_t* b, uint8_t h, uint8_t s, uint8_t v)
{
    int region, fpart, p, q, t;
    
    if(s == 0) {
        /* color is grayscale */
        *r = *g = *b = v;
        return;
    }
    
    /* make hue 0-5 */
    region = h / 43;
    /* find remainder part, make it from 0-255 */
    fpart = (h - (region * 43)) * 6;
    
    /* calculate temp vars, doing integer multiplication */
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * fpart) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8;
        
    /* assign temp vars based on color cone region */
    switch(region) {
        case 0:
            *r = v; *g = t; *b = p; break;
        case 1:
            *r = q; *g = v; *b = p; break;
        case 2:
            *r = p; *g = v; *b = t; break;
        case 3:
            *r = p; *g = q; *b = v; break;
        case 4:
            *r = t; *g = p; *b = v; break;
        default:
            *r = v; *g = p; *b = q; break;
    }

    /*Hack to fix bad behavior. For some reason all non-255 values are 1 lower 
      than when native C program is compiled with gcc.  This leads to ugly 
      transition points because for an unsigned int, 0 - 1 = 255*/
    if (h == 0) *g = 0;
    else if (h == 86) *b = 0;
    else if (h == 172) *r = 0;
    
    return;
}

void turnOff()
{
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
}

//ISR for button
void changeState()
{
  if (millis() - lastDebounceTime > debounceThreshold)
  {
    state = (++state) % NUM_STATES;
    lastDebounceTime = millis();
  }
}

// lookup table for gamma correction
const uint8_t PROGMEM gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


