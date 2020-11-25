#include <Arduino.h>
#include "FastLED.h"
#include <EEPROM.h>
#include <math.h>
#include "main.h"

#define BUTTON_PIN D0
#define TRIG_PIN D2
#define ECHO_PIN D1
#define LED_RED D6
#define LED_GREEN D7
#define LED_BLUE D5
#define TEST_LED D2

setup_t settings;

//#define RESET_PARAMS


//---------------------------------
//--  Settings Stuff  -------------
//---------------------------------
void print_settings()
{     
    Serial.println("Settings");
    Serial.println("--------");
    Serial.print("Mode: ");
    Serial.println(settings.mode);
  
}

void init_settings()
{
    settings.mode = MODE_DISTANCE_WARNING;
}

void save_settings()
{
    uint16_t ii;
    uint8_t* pData;
    pData = (uint8_t*)&settings;

    for (ii=0;ii<sizeof(setup_t);ii++)
    {
        EEPROM.write(ii,*pData);
        pData++;
    }
    EEPROM.commit();
    print_settings();
}

void load_settings()
{
    uint16_t ii;
    uint8_t* pData;
    pData = (uint8_t*)&settings;

    for (ii=0;ii<sizeof(setup_t);ii++)
    {
        *pData = EEPROM.read(ii);
        pData++;
    }
    if (settings.mode > MODE_LAST)
    {
        init_settings();
        save_settings();
    }
                  
#ifndef USE_I2C    
    print_settings();    
#endif
}


mode_button_e check_button(void)
{
    static bool button_pressed = false;
    static unsigned long button_time = 0;

    //New Button Press
    if ((LOW == digitalRead(BUTTON_PIN)) && (button_pressed == false))
    {
        button_pressed = true;
        button_time = millis();     
        return DOWN; 
    }
    else if ((HIGH == digitalRead(BUTTON_PIN)) && (button_pressed == true))
    {
        button_pressed = false;

        if ((millis() - button_time) > VERY_VERY_LONG_PRESS_MS)
        {        
            return VERY_VERY_LONG_PRESS;
        }        
        if ((millis() - button_time) > VERY_LONG_PRESS_MS)
        {        
            return VERY_LONG_PRESS;
        }
        if ((millis() - button_time) > LONG_PRESS_MS)
        {         
            return LONG_PRESS;
        }
        else
        {            
            return SHORT_PRESS;
        }
    }
    return NO_PRESS;
}




void setup() 
{ 
  Serial.begin(115200);
  Serial.println("");
  Serial.println("STARTUP");
  Serial.println("");

  randomSeed(42);

  // put your setup code here, to run once:
  Serial.println("SETUP START");
  pinMode(BUTTON_PIN,INPUT);
  pinMode(LED_RED,OUTPUT);
  pinMode(LED_GREEN,OUTPUT);
  pinMode(LED_BLUE,OUTPUT);
  digitalWrite(LED_RED,HIGH);
  digitalWrite(LED_GREEN,HIGH);
  digitalWrite(LED_BLUE,HIGH);

  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the trigPin as an Output
  Serial.println("SETUP DONE");

  //LED Check
  set_rgb_led(255,0,0);
  delay(300);
  set_rgb_led(0,255,0);
  delay(300);
  set_rgb_led(0,0,255);
  delay(300);
  set_rgb_led(0,0,0);   
  
  init_settings();

    //Setup EEPROM
    EEPROM.begin(512);
    //Overwrite Settings on Reset
#ifdef RESET_PARAMS
    save_settings();
#else
    load_settings();
#endif  
}


//Color Stuff
//-----------

void blink_leds(CRGB col,uint8_t num)
{
    set_rgb_led(0,0,0);
    for (int ii=0;ii<num;ii++)
    {
      set_rgb_led(col.r,col.g,col.b);
      delay(400);
      set_rgb_led(0,0,0);
      delay(300);
    }     
}

void set_rgb_led(uint8_t R, uint8_t G, uint8_t B)
{
    uint16_t r;
    uint16_t g;
    uint16_t b;

    r = 1023 - (R<<2);
    g = 1023 - (G<<2);
    b = 1023 - (B<<2);    
    analogWrite(LED_RED,r);
    analogWrite(LED_GREEN,g);
    analogWrite(LED_BLUE,b); 
}

int measure_distance()
{
  float duration, distance;

    // Clears the trigPin
  digitalWrite(ECHO_PIN, LOW);   // set the echo pin LOW  
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(8);
  digitalWrite(TRIG_PIN, LOW);

  // Calculating the distance
  duration = pulseIn(ECHO_PIN, HIGH);  // measure the echo time (μs)
  distance= duration*0.034/2;
  return (int)distance;
}

int runningfilter(int input, float* pBuffer, float weight)
{
  *pBuffer = (*pBuffer * (1.0-weight)) + ((float)input * weight);
  return(int) *pBuffer;
}

int proc_distance(int dist)
{
    int fdist;
    int ndist = dist;
    static float filterbuf;
    if (ndist>400)
       ndist = 400;
    if (ndist < 5)
       ndist = 5;

    fdist = runningfilter(ndist,&filterbuf,0.1);
    return fdist;
}

#define MAX_DIST 250
#define MIN_DIST 40
#define COUNT_RED 2000
#define COUNT_BLACK 2000
void set_distance_color(int dist,uint8_t min_hue, uint8_t max_hue)
{
    CHSV hsv;
    int h;
    int ndist = dist;
    static unsigned int cnt_red = COUNT_RED;
    static unsigned int cnt_blk = COUNT_BLACK;

    if (ndist<50)
    {
      if (cnt_red>0)
      {
        cnt_red--;
        set_rgb_led(255, 0, 0);
        return;
      }
      if (cnt_blk>0)
      {
        cnt_blk--;
        set_rgb_led(0, 0, 0);
        return;
      }
      cnt_red = COUNT_RED;
      cnt_blk = COUNT_BLACK;
      return;
    }

    if (ndist>MAX_DIST)
       ndist = MAX_DIST;
    if (ndist < MIN_DIST)
       ndist = MIN_DIST;

    h = map(ndist,MIN_DIST,MAX_DIST,min_hue,max_hue);

    hsv.h = h;
    hsv.s = 255;
    hsv.v = 255;

    //Apply offsets
    CRGB finalcol; 
    hsv2rgb_rainbow(hsv,finalcol);
    set_rgb_led(finalcol.r, finalcol.g, finalcol.b);
}

void set_distance_binary(int distance)
{
    int ndist = distance;
    static unsigned int cnt_white = 400;
    static unsigned int cnt_blk = 50;
    static CRGB col = 0;
    const uint32_t collist[4] = {0x000000FF,0x00FF0000,0x00FF8A00,0x00FFFFFF}; //Red, Blue, Orange, White

    if (ndist>400)
       ndist = 400;
    if (ndist < 50)
       ndist = 50;

    if (cnt_white>0)
    {
      cnt_white--;
      set_rgb_led(col.r, col.g, col.b);
      return;
    }
    if (cnt_blk>0)
    {
      cnt_blk--;
      set_rgb_led(0, 0, 0);
      return;
    }
    cnt_white = random(ndist*10, ndist*24);
    cnt_blk = random(ndist*10, ndist*24);
    col = collist[(int)random(4)];
    return;
}

void set_pulse_color(int distance)
{
  const CRGB collist[] = {0x0000FF,0x00FF00,0xFF0000,0x00FFFF,0xFFFF00,0xFF00FF,0xFFFFFF};
  static float cnt = 0.0;
  static int colval = 0;
  static CRGB curcol1 = 0;
  static CRGB curcol2 = 0;
  float val1 = sin(cnt);
  float val2 = sin(cnt+3.0);

  val1 = val1 + 1;
  val1= val1 /2;
  if (val1 < 0.005)
  {
    colval = random(6);
    curcol1 = collist[colval];
  }

  val2 = val2 + 1;
  val2= val2 /2;
  if (val2 < 0.005)
  {
    colval = random(6);
    curcol2 = collist[colval]; 
  }  

  uint8_t r = (uint8_t)(((val1*(float)curcol1.r) + (val2*(float)curcol2.r))/2.0);
  uint8_t g = (uint8_t)(((val1*(float)curcol1.g) + (val2*(float)curcol2.g))/2.0);
  uint8_t b = (uint8_t)(((val1*(float)curcol1.b) + (val2*(float)curcol2.b))/2.0);

  set_rgb_led(r,g,b);
  cnt = cnt + 0.0005;
  if (cnt > (2* PI))
    cnt = cnt - (2*PI);

}

void set_pulse_red(int distance)
{
  static float cnt = 0.0;
  float val = sin(cnt);
  val = val + 1;
  val = val /2;
  val = val * 255;
  set_rgb_led((uint8_t)val, 0,0);

  cnt = cnt + 0.001;
  if (cnt > (2* PI))
    cnt = cnt - (2*PI);
}

void loop() 
{
    static long lastms = millis();
    mode_button_e button_res;
    static int distance;

    if ((lastms + 20) < millis())
    {
      distance = measure_distance();
      distance = proc_distance(distance);
      Serial.print("Distance: ");
      Serial.println(distance);
      lastms = millis();
    }
    

    button_res = check_button();
    if (SHORT_PRESS == button_res)
    {
      settings.mode++;
      if (settings.mode == MODE_LAST)
        settings.mode = MODE_DISTANCE_WARNING;

      blink_leds(CRGB::Green,(int)settings.mode + 1);
      save_settings();
    }

    switch (settings.mode)
    {
      case MODE_DISTANCE_WARNING:
        set_distance_color(distance,0,100);
        break;
      case MODE_DISTANCE_RAINBOW:
        set_distance_color(distance,0,255);
        break;
      case MODE_DISTANCE_BINARY:
        set_distance_binary(distance);
        break;
      case MODE_PULSE_COLOR:
        set_pulse_color(distance);
        break;
      case MODE_PULSE_RED:
        set_pulse_red(distance);
        break;        
      case MODE_BEAT:
        set_rgb_led(255,255,255);
        break;
    }
}