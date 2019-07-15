//Code is mostly duplicated since it's controlling two Neopixels. _SV denotes Saturn V Neopixels and _LM denotes Lunar Module Neopixels.
//There are duplicates for the display; 7-segment has been commented out (and un-tested) and the Alphanumeric one has been enabled/tested.

#include <Adafruit_LEDBackpack_RK.h>
#include <neopixel.h>
#include <Particle.h>
#include <math.h>
#include "Particle.h"
#include "Adafruit_GFX_RK.h"
#include "Adafruit_LEDBackpack_RK.h"

//Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

//Saturn V pins/parameters
#define PIN_SV D2
#define PIXEL_COUNT_SV 60
#define arrayLength_SV PIXEL_COUNT_SV/3
#define SCALE_AMOUNT_SV 0.5
#define FLICKER_SPEED_SV 5
#define FLICKER_AMOUNT_SV 50

//Lunar Module pins/parameters
#define PIN_LM D3
#define PIXEL_COUNT_LM 6
#define arrayLength_LM PIXEL_COUNT_LM/3
#define SCALE_AMOUNT_LM 0.5
#define FLICKER_SPEED_LM 5
#define FLICKER_AMOUNT_LM 50

//Pins to control the lights
#define Fuel_Red A0
#define Fuel_Green A1
#define Fuel_Blue A2
#define Alarm_1201 A3
#define Alarm_1202 A4

//Pins to turn on the audio clips
#define SV_COUNTDOWN D4
#define SV_SHUTDOWN D5
#define LM_LANDING D6

String SV_Status = "Off";
String LM_Status = "Off";

//Values and flag variables for the Saturn V
int SV_Brightness = 0;              //Value to set the brightness level of the SV lights
bool Launch_Flag = false;           //Flag to ramp up the SV lights from off to full-on
bool Engine_Sequence_Flag = false;  //Flag to ramp up the SV lights from off to low
bool Engine_Launch_Flag = false;    //Flag to ramp up the SV lights from low to full-on
bool Audio_Launch_Flag = false;     //Flag to start the SV launch audio sequence

//Values and flag variables for the Lunar Module
int LM_Brightness = 0;              //Value to set the brightness level of the LM lights
bool Landing_Flag = false;          //Flag to ramp up the LM lights from off to full-on
bool PDI_Flag = false;
bool Throttle_Down_Flag = false;    //Flag to ramp down the LM lights from full-on to medium
bool LM_Shutdown_Flag = false;
bool Audio_Landing_Flag = false;    //Flag to start up the LM landing audio sequence
int Alarm_Number;
int Alarm_Loops = 0;
int Alarm_Counter = 0;

//Values and flag variables For the timer/stopwatch/clock display
int Clock_Type = 0; //0 is for a timer (count down), 1 is for a stopwatch (count up)
int Minute_1 = 0;
int Minute_2 = 0;
int Second_1 = 0;
int Second_2 = 0;
bool Clock_Display_Flag = false;
bool Time_Display_Flag = true; //Dictate whether you want the time to start displaying immediately or not
int hour = 00;
int minute = 00;

//Variables used to control the flicker delay for the Saturn V and Lunar Module. Original source didn't need this but the Photon runs faster.
int Flicker_Delay = 50; //Delay the looping lights code this many ms so the flicker looks right
unsigned long lastTime_SV = 0;
unsigned long lastTime_LM = 0;


//Timers to start engine sequence, launch and shutdown for Saturn V (SV) and Lunar Module (LM)
Timer timer_SVES(24000, SV_Engine_Sequence, true);
Timer timer_SVL(32000, SV_Launch, true);
Timer timer_SVSD(20700, SV_Shutdown, true);
Timer timer_LMSU(33000, LM_Startup, true);
Timer timer_LMCD(35000, LM_Countdown_Start, true);
Timer timer_CD(1000, Clock_Display, false);
Timer timer_F1202(125750, LM_First_1202, true);
Timer timer_S1202(156500, LM_Second_1202, true);
Timer timer_LMTD(169000, LM_Throttle_Down, true);
Timer timer_1201(186800, LM_1201, true);
Timer timer_LM60(215000, LM_60_Seconds, true);
Timer timer_LM30(245000,LM_30_Seconds, true);
Timer timer_LMSD(261500, LM_Shutdown, true);
Timer timer_LMCE(310000, LM_Countdown_End, true);
Timer timer_LM_Alarm(412, LM_Alarm, true);

//All of the following are setting up the parameters for both of the neopixels.
struct RGBW_SV {
  byte r_SV;
  byte g_SV;
  byte b_SV;
  byte w_SV;
};
struct RGBW_LM {
  byte r_LM;
  byte g_LM;
  byte b_LM;
  byte w_LM;
};

RGBW_SV colors_SV[] = {
  { 250, 150, 0, 250},    // yellow + white //Removed the 250 value for white to simulate my other non-white neopixel strip
  { 250, 120, 0, 0},      // yellow + white
  { 250, 90, 0, 0},       // orange
  { 250, 30, 0, 0},       // orangie-red
  { 250, 0, 0, 0},        // red
  { 250, 0, 0, 0}         // extra red
};
RGBW_LM colors_LM[] = {
  { 250, 150, 0, 250},    // yellow + white //Removed the 250 value for white to simulate my other non-white neopixel strip
  { 250, 120, 0, 0},      // yellow + white
  { 250, 90, 0, 0},       // orange
  { 250, 30, 0, 0},       // orangie-red
  { 250, 0, 0, 0},        // red
  { 250, 0, 0, 0}         // extra red
};

int NUMBER_OF_COLORS_SV = sizeof(colors_SV) / sizeof(RGBW_SV);
int NUMBER_OF_COLORS_LM = sizeof(colors_LM) / sizeof(RGBW_LM);

int percentBetween_SV(int a, int b, float percent) {
  return (int)(((b - a) * percent) + a);
}
int percentBetween_LM(int a, int b, float percent) {
  return (int)(((b - a) * percent) + a);
}

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags:

//#define WS2811         0x00 // 400 KHz datastream (NeoPixel)
//#define WS2812         0x02 // 800 KHz datastream (NeoPixel)
//#define WS2812B        0x02 // 800 KHz datastream (NeoPixel)
//#define WS2813         0x02 // 800 KHz datastream (NeoPixel)
//#define TM1803         0x03 // 400 KHz datastream (Radio Shack Tri-Color Strip)
//#define TM1829         0x04 // 800 KHz datastream ()
//#define WS2812B2       0x05 // 800 KHz datastream (NeoPixel)
//#define SK6812RGBW     0x06 // 800 KHz datastream (NeoPixel RGBW)
//#define WS2812B_FAST   0x07 // 800 KHz datastream (NeoPixel)
//#define WS2812B2_FAST  0x08 // 800 KHz datastream (NeoPixel)
Adafruit_NeoPixel strip_SV = Adafruit_NeoPixel(PIXEL_COUNT_SV, PIN_SV, WS2812); //Select you neopixel type for the third parameter
Adafruit_NeoPixel strip_LM = Adafruit_NeoPixel(PIXEL_COUNT_LM, PIN_LM, SK6812RGBW); //Select you neopixel type for the third parameter

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

float scale_SV = 1;
float inc_SV = 0.1;
float dir_SV = 1.0;
byte pixelBrightness_SV[arrayLength_SV];
bool pixelDirection_SV[arrayLength_SV];

float scale_LM = 1;
float inc_LM = 0.1;
float dir_LM = 1.0;
byte pixelBrightness_LM[arrayLength_LM];
bool pixelDirection_LM[arrayLength_LM];

// Prepopulate the pixel arrays
void seedArray_SV() {
  for (uint16_t i_SV=0; i_SV < PIXEL_COUNT_SV; i_SV++) {
    uint16_t p_SV = i_SV % arrayLength_SV;
    pixelBrightness_SV[p_SV] = random(255-FLICKER_AMOUNT_SV, 255);
    pixelDirection_SV[p_SV] = !!random(0, 1);
  }
}
void seedArray_LM() {
  for (uint16_t i_LM=0; i_LM < PIXEL_COUNT_LM; i_LM++) {
    uint16_t p_LM = i_LM % arrayLength_LM;
    pixelBrightness_LM[p_LM] = random(255-FLICKER_AMOUNT_LM, 255);
    pixelDirection_LM[p_LM] = !!random(0, 1);
  }
}

void setup() {
//Start the display and clear all values. Also start the neopixels.   
//matrix.begin(0x70);
alpha4.begin(0x70);
//matrix.writeDigitRaw(0, 0);
alpha4.writeDigitRaw(0, 0);
//matrix.writeDigitRaw(1, 0);
alpha4.writeDigitRaw(1, 0);
//matrix.drawColon(false);
//matrix.writeDigitRaw(3, 0);
alpha4.writeDigitRaw(2, 0);
//matrix.writeDigitRaw(4, 0);
alpha4.writeDigitRaw(3, 0);
//matrix.writeDisplay();
alpha4.writeDisplay();
strip_SV.begin();
strip_SV.setBrightness(SV_Brightness);
strip_SV.show();
strip_LM.begin();
strip_LM.setBrightness(LM_Brightness);
strip_LM.show();

//Not strictly required but good practice to declare pin modes
pinMode(SV_COUNTDOWN, OUTPUT);
pinMode(SV_SHUTDOWN, OUTPUT);
pinMode(LM_LANDING, OUTPUT);
pinMode(Fuel_Red, OUTPUT);
pinMode(Fuel_Green, OUTPUT);
pinMode(Fuel_Blue, OUTPUT);
pinMode(Alarm_1201, OUTPUT);
pinMode(Alarm_1202, OUTPUT);

//Set all of the light/audio control pins to inactive states
digitalWrite(SV_COUNTDOWN, HIGH);
digitalWrite(SV_SHUTDOWN, HIGH);
digitalWrite(LM_LANDING, HIGH);
digitalWrite(Fuel_Red, LOW);
digitalWrite(Fuel_Green, LOW);
digitalWrite(Fuel_Blue, LOW);
digitalWrite(Alarm_1201, HIGH);
digitalWrite(Alarm_1202, HIGH);

//Subscribe to whatever Publishes you want to listen to and declare the Particle Variables you want on the device
Particle.subscribe("Launch", Launch, MY_DEVICES);
Particle.subscribe("Landing", Landing, MY_DEVICES);
Particle.variable("SV_Status", SV_Status);
Particle.variable("LM_Status", LM_Status);

//Time zone declaration required for the clock to work properly. For those in the US, keep in mind that Daylight Savings isn't accounted for.
Time.zone(-5);
}

void loop() {
//Have the Saturn V launch every year on the Apollo 11 launch anniversary day and time
if (Time.month() == 7 and Time.day() == 16 and Time.hour() == 7 and Time.minute() == 31 and Time.second() == 26) {
Particle.publish("Launch", "Audio", 60, PRIVATE);
delay(1500);
}

//Have the Saturn V TLI end every year on the Apollo 11 TLI anniversary day and time
if (Time.month() == 7 and Time.day() == 16 and Time.hour() == 10 and Time.minute() == 21 and Time.second() == 43) {
Particle.publish("Launch", "NoGo", 60, PRIVATE);
delay(1500);
}

//Have the LM land every year on the Apollo 11 landing anniversary day and time
if (Time.month() == 7 and Time.day() == 20 and Time.hour() == 14 and Time.minute() == 13 and Time.second() == 16) {
Particle.publish("Landing", "Audio", 60, PRIVATE);
delay(1500);
}

//Variables for keeping track of the current 'time' for the neopixel flicker delay
unsigned long now_SV = millis();
unsigned long now_LM = millis();

//Saturn V Lights Loop (it's fairly long)
//Set of if statements to change the brightness level based on various scenarios/flags
if (((now_SV - lastTime_SV) >= Flicker_Delay) and Launch_Flag == true) {
    lastTime_SV = now_SV;
    if (Audio_Launch_Flag == true) {
        if (Engine_Launch_Flag == true){
            //if statement to ramp up SV Brightness from low to high for full launch
            if (SV_Brightness < 255) {
                SV_Brightness += 1;
                strip_SV.setBrightness(SV_Brightness);
            }
        }
        
        else if (Engine_Sequence_Flag == true) {
            //if statement to ramp up SV_Brightness from off to low for engine startup
            if (SV_Brightness < 5) {
                SV_Brightness += 1;
                strip_SV.setBrightness(SV_Brightness);
            }
        }
    }
    else {
        //if statement to ramp up SV Brightness from 0 to full for a normal launch
        if (SV_Brightness <255) {
            SV_Brightness += 1;
            strip_SV.setBrightness(SV_Brightness);
        }
    }

  // how many leds for each color
  int ledsPerColor_SV = ceil(PIXEL_COUNT_SV / (NUMBER_OF_COLORS_SV-1));

  // the scale animation direction
  if (scale_SV <= 0.5 || scale_SV >= 1) {
    dir_SV = dir_SV * -1;
  }

  // add a random amount to inc
  inc_SV = ((float)random(0, 50)/1000);

  // add the increment amount to the scale
  scale_SV += (inc_SV * dir_SV);

  // constrain the scale
  scale_SV = constrain(scale_SV, 0.5, 1);

  for (uint16_t i_SV=0; i_SV < PIXEL_COUNT_SV; i_SV++) {
    uint16_t p_SV = i_SV % arrayLength_SV;

    float val_SV = ((float)i_SV * scale_SV) / (float)ledsPerColor_SV;
    int currentIndex_SV = floor(val_SV);
    int nextIndex_SV = ceil(val_SV);
    float transition_SV = fmod(val_SV, 1);

    // color variations
    if (pixelDirection_SV[p_SV]) {
      pixelBrightness_SV[p_SV] += FLICKER_SPEED_SV;

      if (pixelBrightness_SV[p_SV] >= 255) {
        pixelBrightness_SV[p_SV] = 255;
        pixelDirection_SV[p_SV] = false;
      }
    } else {
      pixelBrightness_SV[p_SV] -= FLICKER_SPEED_SV;

      if (pixelBrightness_SV[p_SV] <= 255-FLICKER_AMOUNT_SV) {
        pixelBrightness_SV[p_SV] = 255-FLICKER_AMOUNT_SV;
        pixelDirection_SV[p_SV] = true;
      }
    }

    RGBW_SV currentColor_SV = colors_SV[currentIndex_SV];
    RGBW_SV nextColor_SV = colors_SV[nextIndex_SV];
    float flux_SV = (float)pixelBrightness_SV[p_SV] / 255;

    byte r_SV = percentBetween_SV(currentColor_SV.r_SV, nextColor_SV.r_SV, transition_SV) * flux_SV;
    byte g_SV = percentBetween_SV(currentColor_SV.g_SV, nextColor_SV.g_SV, transition_SV) * flux_SV;
    byte b_SV = percentBetween_SV(currentColor_SV.b_SV, nextColor_SV.b_SV, transition_SV) * flux_SV;
    byte w_SV = percentBetween_SV(currentColor_SV.w_SV, nextColor_SV.w_SV, transition_SV) * flux_SV;

    //You may have to swap the RGBW order depending on the neopixel you use
    strip_SV.setPixelColor(i_SV, r_SV, g_SV, b_SV, w_SV);
  }

  strip_SV.show();
}

//Slowly ramp the leds down if you shut it off
  else if(Launch_Flag == false) {
            if (SV_Brightness > 0) {
                SV_Brightness -= 5;
                if (SV_Brightness < 0) {
                    SV_Brightness = 0;
                }
                strip_SV.setBrightness(SV_Brightness);
            }
    strip_SV.show();
    
}

//Lunar Module Lights Loop (it's fairly long)
//Set of if statements to change the brightness level based on various scenarios/flags
if (((now_LM - lastTime_LM) >= Flicker_Delay) and Landing_Flag == true) {
    lastTime_LM = now_LM;
    if (Audio_Landing_Flag == true) {
        if (LM_Shutdown_Flag == true){
            //if statement to turn down LM_Brightness from low to off
            if (LM_Brightness > 0) {
                LM_Brightness -= 1;
                strip_LM.setBrightness(LM_Brightness);
            }
        }
        
        else if (Throttle_Down_Flag == true) {
            //if statement to turn down LM_Brightness from high to low 
            if (LM_Brightness > 25) {
                LM_Brightness -= 1;
                strip_LM.setBrightness(LM_Brightness);
            }
        }
        
        else if (PDI_Flag == true) {
            //if statement to turn up LM_Brightness from off to high 
            if (LM_Brightness < 255) {
                LM_Brightness += 1;
                strip_LM.setBrightness(LM_Brightness);
            }
        }
    }
    else {
        //if statement to ramp up LM Brightness from 0 to full
        if (LM_Brightness <255) {
            LM_Brightness += 1;
            strip_LM.setBrightness(LM_Brightness);
        }
    }



  // how many leds for each color
  int ledsPerColor_LM = ceil(PIXEL_COUNT_LM / (NUMBER_OF_COLORS_LM-1));

  // the scale animation direction
  if (scale_LM <= 0.5 || scale_LM >= 1) {
    dir_LM = dir_LM * -1;
  }

  // add a random amount to inc
  inc_LM = ((float)random(0, 50)/1000);

  // add the increment amount to the scale
  scale_LM += (inc_LM * dir_LM);

  // constrain the scale
  scale_LM = constrain(scale_LM, 0.5, 1);

  for (uint16_t i_LM=0; i_LM < PIXEL_COUNT_LM; i_LM++) {
    uint16_t p_LM = i_LM % arrayLength_LM;

    float val_LM = ((float)i_LM * scale_LM) / (float)ledsPerColor_LM;
    int currentIndex_LM = floor(val_LM);
    int nextIndex_LM = ceil(val_LM);
    float transition_LM = fmod(val_LM, 1);

    // color variations
    if (pixelDirection_LM[p_LM]) {
      pixelBrightness_LM[p_LM] += FLICKER_SPEED_LM;

      if (pixelBrightness_LM[p_LM] >= 255) {
        pixelBrightness_LM[p_LM] = 255;
        pixelDirection_LM[p_LM] = false;
      }
    } else {
      pixelBrightness_LM[p_LM] -= FLICKER_SPEED_LM;

      if (pixelBrightness_LM[p_LM] <= 255-FLICKER_AMOUNT_LM) {
        pixelBrightness_LM[p_LM] = 255-FLICKER_AMOUNT_LM;
        pixelDirection_LM[p_LM] = true;
      }
    }

    RGBW_LM currentColor_LM = colors_LM[currentIndex_LM];
    RGBW_LM nextColor_LM = colors_LM[nextIndex_LM];
    float flux_LM = (float)pixelBrightness_LM[p_LM] / 255;

    byte r_LM = percentBetween_LM(currentColor_LM.r_LM, nextColor_LM.r_LM, transition_LM) * flux_LM;
    byte g_LM = percentBetween_LM(currentColor_LM.g_LM, nextColor_LM.g_LM, transition_LM) * flux_LM;
    byte b_LM = percentBetween_LM(currentColor_LM.b_LM, nextColor_LM.b_LM, transition_LM) * flux_LM;
    byte w_LM = percentBetween_LM(currentColor_LM.w_LM, nextColor_LM.w_LM, transition_LM) * flux_LM;

    //You may have to swap the RGBW order depending on the neopixel you use
    strip_LM.setPixelColor(i_LM, g_LM, r_LM, b_LM, w_LM);
  }

  strip_LM.show();
}

//Slowly ramp the leds down if you shut it off
  else if (Landing_Flag == false) {
            if (LM_Brightness > 0) {
                LM_Brightness -= 5;
                if (LM_Brightness < 0) {
                    LM_Brightness = 0;
                }
                strip_LM.setBrightness(LM_Brightness);
            }
    strip_LM.show();
}

//If statement to display the current time if the device knows the time and is commanded to
if (Time_Display_Flag == true and Time.isValid()) {
    hour = Time.hour();
    minute = Time.minute();
    if (hour < 10) {
				alpha4.writeDigitRaw(0, 0); // blank
			}
			else {
				alpha4.writeDigitAscii(0, hour / 10 + '0');				
			}
			alpha4.writeDigitAscii(1, hour % 10 + '0');
    
    alpha4.writeDigitAscii(2, minute / 10 + '0');				
	alpha4.writeDigitAscii(3, minute % 10 + '0');
}

//Display the timer/stopwatch every second and count down/up depending on the scenario
if (Clock_Display_Flag == true) {
    
    Clock_Display_Flag = false;
    
    if(Clock_Type == 0) { //Timer scenario, which in my case is always less than 10 minutes and the first minute digit is a '-'
        if(Second_2 == 0 and Second_1 == 0 and Minute_2 == 0) {
            Clock_Type = 1;
            //matrix.writeDigitNum(0, Minute_1);
            //alpha4.writeDigitAscii(0, Minute_1 + '0');
            alpha4.writeDigitAscii(0, 'T');
            alpha4.writeDigitAscii(1, '+');
        }
        
        else if(Second_2 == 0 and Second_1 == 0) {
            Minute_2 -= 1;
            Second_1 = 5;
            Second_2 = 9;
            //matrix.writeDigitNum(1, Minute_2);
            alpha4.writeDigitAscii(1, Minute_2 + '0');
            //matrix.writeDigitNum(3, Second_1);
            alpha4.writeDigitAscii(2, Second_1 + '0');
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
        else if(Second_2 ==0){
            Second_1 -=1;
            Second_2=9;
            //matrix.writeDigitNum(3, Second_1);
            alpha4.writeDigitAscii(2, Second_1 + '0');
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
        else {
            Second_2 -=1;
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
    }
    
    if(Clock_Type == 1) { //Stopwatch scenario
        if(Minute_2 == 9 and Second_1 == 5 and Second_2 == 9) {
            Minute_1 += 1;
            Minute_2 = 0;
            Second_1 = 0;
            Second_2 = 0;
            //matrix.writeDigitNum(0, Minute_1);
            alpha4.writeDigitAscii(0, Minute_1 + '0');
            //matrix.writeDigitNum(1, Minute_2);
            alpha4.writeDigitAscii(1, Minute_2 + '0');
            //matrix.writeDigitNum(3, Second_1);
            alpha4.writeDigitAscii(2, Second_1 + '0');
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
        else if(Second_1 ==5 and Second_2 ==9) {
            Minute_2 += 1;
            Second_1 = 0;
            Second_2 = 0;
            
            if (Minute_1 == 0) {
                alpha4.writeDigitAscii(0, '+');
            }
            
            //matrix.writeDigitNum(1, Minute_2);
            alpha4.writeDigitAscii(1, Minute_2 + '0');
            //matrix.writeDigitNum(3, Second_1);
            alpha4.writeDigitAscii(2, Second_1 + '0');
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
        else if(Second_2 == 9) {
            Second_1 += 1;
            Second_2 = 0;
            //matrix.writeDigitNum(3, Second_1);
            alpha4.writeDigitAscii(2, Second_1 + '0');
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
        else {
            Second_2 += 1;
            //matrix.writeDigitNum(4, Second_2);
            alpha4.writeDigitAscii(3, Second_2 + '0');
        }
    }
}
//matrix.writeDisplay(); //Display the new time
alpha4.writeDisplay();
}

//Scenarios for controlling the Saturn V Neopixels based on a Publish event
void Launch(const char *event, const char *data)
{
    if (strcmp(data,"Go")==0) {
        Launch_Flag = true;
        SV_Status = "We Have Liftoff!";
    }
    
    else if (strcmp(data,"NoGo")==0) {
        if (Audio_Launch_Flag == true) {
            SV_Status = "TLI Started";
            digitalWrite(SV_SHUTDOWN,LOW);
            timer_SVSD.start();
            Audio_Launch_Flag = false;
            Engine_Launch_Flag = false;
            timer_CD.stop();
            //Clear the timer display
            //matrix.writeDigitRaw(0, 0);
            alpha4.writeDigitRaw(0, 0);
            //matrix.writeDigitRaw(1, 0);
            alpha4.writeDigitRaw(1, 0);
            //matrix.drawColon(false);
            //matrix.writeDigitRaw(3, 0);
            alpha4.writeDigitRaw(2, 0);
            //matrix.writeDigitRaw(4, 0);
            alpha4.writeDigitRaw(3, 0);
            //matrix.writeDisplay();
            alpha4.writeDisplay();
            Time_Display_Flag = true; //Start displaying the time
        }
        else {
            Launch_Flag = false;
            SV_Status = "TLI Cutoff";
        }
    }
    else if (strcmp(data,"Audio")==0) {
        SV_Brightness = 0;
        strip_SV.setBrightness(SV_Brightness);
        SV_Status = "Countdown Started";
        Audio_Launch_Flag = true;
        Launch_Flag = true;
        Engine_Sequence_Flag = false;
        Engine_Launch_Flag = false; 
        digitalWrite(SV_COUNTDOWN,LOW);
        Time_Display_Flag = false; //Stop displaying the time
        Clock_Display_Flag = false;
        Audio_Landing_Flag = false;
        Landing_Flag = false;
        Clock_Type = 0; //0 is for a timer (count down), 1 is for a stopwatch (count up)
        Minute_1 = 0;
        Minute_2 = 0;
        Second_1 = 3;
        Second_2 = 1;
        timer_CD.start();
        timer_SVES.start();
        timer_SVL.start(); 
        //matrix.writeDigitRaw(0,0x40); //Draw just the middle line, a minus sign.
        alpha4.writeDigitAscii(0, 'T');
        //matrix.writeDigitNum(1, Minute_2);
        alpha4.writeDigitAscii(1, '-');
        //matrix.drawColon(true);
        //matrix.writeDigitNum(3, Second_1);
        alpha4.writeDigitAscii(2, Second_1 + '0');
        //matrix.writeDigitNum(4, Second_2);
        alpha4.writeDigitAscii(3, Second_2 + '0');
    }
}

//Scenarios for controlling the Saturn V Neopixels based on a Publish event
void Landing(const char *event, const char *data)
{
    if (strcmp(data,"Go")==0) {
        Landing_Flag = true;
        LM_Status = "PDI Started";
    }
    
    else if (strcmp(data,"NoGo")==0) {
        Landing_Flag = false;
        LM_Status = "The Eagle Has Landed!";
    }
    
    else if (strcmp(data,"Audio")==0) {
        LM_Brightness = 0;
        strip_LM.setBrightness(LM_Brightness);
        LM_Status = "Landing Started";
        Audio_Landing_Flag = true;
        Landing_Flag = true;
        PDI_Flag = false;
        Throttle_Down_Flag = false;
        LM_Shutdown_Flag = false;
        Time_Display_Flag = false; //Stop displaying the time
        Clock_Display_Flag = false;
        Audio_Launch_Flag = false;
        Engine_Launch_Flag = false;
        Launch_Flag = false;
        digitalWrite(LM_LANDING,LOW);
        timer_CD.stop();
        timer_LMSU.start();
        timer_LMCD.start();
        timer_F1202.start();
        timer_S1202.start();
        timer_LMTD.start();
        timer_1201.start();
        timer_LM60.start();
        timer_LM30.start();
        timer_LMSD.start();
        timer_LMCE.start();
        //Clear the timer display
        //matrix.writeDigitRaw(0, 0);
        alpha4.writeDigitRaw(0, 0);
        //matrix.writeDigitRaw(1, 0);
        alpha4.writeDigitRaw(1, 0);
        //matrix.drawColon(false);
        //matrix.writeDigitRaw(3, 0);
        alpha4.writeDigitRaw(2, 0);
        //matrix.writeDigitRaw(4, 0);
        alpha4.writeDigitRaw(3, 0);
        //matrix.writeDisplay();
        alpha4.writeDisplay();
    }
}

//The beginning of a LOT of timers used for coordinating the audio/visual 'experience' :)
void SV_Engine_Sequence()
{
    SV_Status = "Engines Started";
    Engine_Sequence_Flag = true;
    digitalWrite(SV_COUNTDOWN,HIGH);
}

void SV_Launch()
{
    SV_Status = "We Have Liftoff!";
    Engine_Launch_Flag=true;
    Engine_Sequence_Flag=false;
}

void SV_Shutdown()
{
    SV_Status = "TLI Cutoff";
    Launch_Flag = false;
    digitalWrite(SV_SHUTDOWN,HIGH);
}

void LM_Startup()
{
    LM_Status = "PDI Started";
    PDI_Flag = true;
    digitalWrite(LM_LANDING,HIGH);
}

void LM_Countdown_Start()
{
    //Code to start countdown timer and start displaying it
    Clock_Type = 0; //0 is for a timer (count down), 1 is for a stopwatch (count up)
    Minute_1 = 0;
    Minute_2 = 4;
    Second_1 = 0;
    Second_2 = 0;
    //matrix.writeDigitRaw(0,0x40); //Draw just the middle line, a minus sign.
    alpha4.writeDigitAscii(0, '-');
    //matrix.writeDigitNum(1, Minute_2);
    alpha4.writeDigitAscii(1, Minute_2 + '0');
    //matrix.drawColon(true);
    //matrix.writeDigitNum(3, Second_1);
    alpha4.writeDigitAscii(2, Second_1 + '0');
    //matrix.writeDigitNum(4, Second_2);
    alpha4.writeDigitAscii(3, Second_2 + '0');
    alpha4.writeDisplay();
    
    //Turn Fuel light to white
    digitalWrite(Fuel_Red, HIGH);
    digitalWrite(Fuel_Green, HIGH);
    digitalWrite(Fuel_Blue, HIGH);
    
    timer_CD.start();
}

void Clock_Display()
{
    Clock_Display_Flag = true;
}

void LM_First_1202()
{
    Alarm_Number = 1202;
    Alarm_Loops = 32;
    Alarm_Counter = 1;
    digitalWrite(Alarm_1202, LOW);
    timer_LM_Alarm.start();
}

void LM_Second_1202()
{
    Alarm_Number = 1202;
    Alarm_Loops = 38;
    Alarm_Counter = 1;
    digitalWrite(Alarm_1202, LOW);
    timer_LM_Alarm.start();
}

void LM_Throttle_Down()
{
    LM_Status = "LM Engines Throttled Down";
    PDI_Flag = false;
    Throttle_Down_Flag = true;
}

void LM_1201()
{
    Alarm_Number = 1201;
    Alarm_Loops = 34;
    Alarm_Counter = 1;
    digitalWrite(Alarm_1201, LOW);
    timer_LM_Alarm.start();
}

void LM_60_Seconds()
{
    //Turn Fuel light to yellow
    digitalWrite(Fuel_Red, HIGH);
    digitalWrite(Fuel_Green, HIGH);
    digitalWrite(Fuel_Blue, LOW);
}

void LM_30_Seconds()
{
    //Turn Fuel light to red
    digitalWrite(Fuel_Red, HIGH);
    digitalWrite(Fuel_Green, LOW);
    digitalWrite(Fuel_Blue, LOW);
}

void LM_Shutdown()
{
    LM_Status = "The Eagle Has Landed!";
    Throttle_Down_Flag = false;
    LM_Shutdown_Flag = true;
    timer_CD.stop(); //Stop the fuel countdown timer
}

void LM_Countdown_End()
{
    //Turn Fuel light off
    digitalWrite(Fuel_Red, LOW);
    digitalWrite(Fuel_Green, LOW);
    digitalWrite(Fuel_Blue, LOW);
    
    //Turn off the fuel countdown timer display
    //matrix.writeDigitRaw(0, 0);
    alpha4.writeDigitRaw(0, 0);
    //matrix.writeDigitRaw(1, 0);
    alpha4.writeDigitRaw(1, 0);
    //matrix.drawColon(false);
    //matrix.writeDigitRaw(3, 0);
    alpha4.writeDigitRaw(2, 0);
    //matrix.writeDigitRaw(4, 0);
    alpha4.writeDigitRaw(3, 0);
    //matrix.writeDisplay();
    alpha4.writeDisplay();
    
    Time_Display_Flag = true; //Start displaying the time
    LM_Shutdown_Flag = false;
    Audio_Landing_Flag = false;
    Landing_Flag = false;
}

void LM_Alarm()
{
    if (Alarm_Counter < Alarm_Loops) {
        Alarm_Counter += 1;
        if (Alarm_Number == 1201) {
            if(Alarm_Counter % 2 == 0) {
                digitalWrite(Alarm_1201, HIGH);
            }
            else {
                digitalWrite(Alarm_1201, LOW);
            }
        }
        else if (Alarm_Number ==1202) {
            if (Alarm_Counter % 2 == 0) {
                digitalWrite(Alarm_1202, HIGH);
            }
            else {
                digitalWrite(Alarm_1202, LOW);
            }
        }
        timer_LM_Alarm.start();
    }
}