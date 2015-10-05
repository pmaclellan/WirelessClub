// Pin definitions
#define RED_PIN    11
#define GREEN_PIN  10
#define BLUE_PIN   9
#define INPUT_PIN  2
#define KNOB_1     A2
#define KNOB_2     A1
#define KNOB_3     A0

// state definitions
#define OFF        0
#define RGB_MODE   1
#define HSL_MODE   2
#define FADE_MODE  3
#define NUM_STATES 4

// Max duty cycles
#define MAX_RED    255
#define MAX_GREEN  255
#define MAX_BLUE   255
#define MAX_HUE    1023
#define MAX_SAT    1023
#define MAX_LUM    1023

// Function declarations
void displayRGB(int, int, int);
void turnOff();
void HSLtoRGB(float, float, float, int*, int*, int*);

// Global variables
int counter = 0;
volatile int state = 0;
int hue = 0;

void setup()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(INPUT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN), changeState, RISING);
  Serial.begin(9600);
  
  turnOff();
}

void loop()
{
  bool fadeUpHue = true;
  
  switch (state)
  {
    case RGB_MODE:
      analogWrite(RED_PIN, floor(readKnob(KNOB_1) * 255));
      analogWrite(GREEN_PIN, floor(readKnob(KNOB_2) * 255));
      analogWrite(BLUE_PIN, floor(readKnob(KNOB_3) * 255));
      break;
    case HSL_MODE:
      int r, g, b;
    
      HSLtoRGB(readKnob(KNOB_1), readKnob(KNOB_2), readKnob(KNOB_3), &r, &g, &b);
      Serial.println(r);
      Serial.println(g);
      Serial.println(b);
      Serial.println("---");
      displayRGB(r, g, b);
      break;
    case FADE_MODE:
      // enter loop until mode button is pressed
      while (state == FADE_MODE)
      {
        int hueRate = floor(readKnob(KNOB_1) * 10);
      
        if(fadeUpHue)
        {
          hue += hueRate;
          if (hue >= MAX_HUE)
          {
            hue = MAX_HUE;
            // change direction
            fadeUpHue = false;
          }
        }
        else
        {
          hue -= hueRate;
          if (hue <= 0)
          {
            hue = 0;
            // change direction
            fadeUpHue = true;
          }
        }
        HSLtoRGB((float)hue/MAX_HUE, 1.0f, 1.0f, &r, &g, &b);
        displayRGB(r, g, b);
        delay(10);
      }
      break;
    case OFF:
      turnOff();
      break;
  }
}

float readKnob(int pin)
{
  return (float) analogRead(pin) / 1023;
}

void displayRGB(int r, int g, int b)
{
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// H,S,L in [0,1] 
// R,G,B in [0,255]
void HSLtoRGB(float hue, float sat, float lum, int* r, int* g, int* b)
{
    float red = 0, green = 0, blue = 0;
    
    float f_sextant = hue / 0.1666f;
    int i_sextant = floor(f_sextant);
    float prim = (1 - abs(2*lum - 1)) * sat;
    float sec = (1 - abs(i_sextant % 2 - 1)) * prim;
    float m = lum - (0.5 * prim);
 
    
    switch (i_sextant) {
      
            case 0: 
              red = prim + m; 
              green = sec + m; 
              blue = m; 
              break;
            case 1: 
              red = sec + m; 
              green = prim + m; 
              blue = m; 
              break;
            case 2: 
              red = m; 
              green = prim + m; 
              blue = sec + m; 
              break;
            case 3: 
              red = m; 
              green = sec + m; 
              blue = prim + m;
             break;
            case 4: 
              red = sec + m; 
              green = m; 
              blue = prim + m; 
              break;
            case 5: 
              red = prim + m; 
              green = m; 
              blue = sec + m; 
              break;
        }
                
    *r = floor(red * 255), 
    *g = floor(green * 255), 
    *b = floor(blue * 255);

    /*Serial.println(hue,4);
    Serial.println(sat,4);
    Serial.println(lum,4);
    delay(200);*/
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
  state = (++state) % NUM_STATES;
}



