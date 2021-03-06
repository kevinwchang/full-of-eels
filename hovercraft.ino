#include <Wire.h> // not used, but Zumo 32U4 libs won't compile without this
#include <FastGPIO.h>
#include <Pushbutton.h>
#include <QTRSensors.h>
#include "HovercraftMotors.h"
#include "LiftMotorBuzzer.h"

#define VBAT_DIV     A1
#define BUTTON_INPUT A0
#define BUTTON_GND   11

Pushbutton button(BUTTON_INPUT);

#define NUM_SENSORS 6

QTRSensorsRC lineSensors((unsigned char[]) {6, 4, 3, 2, 1, 0}, NUM_SENSORS, 2500, QTR_NO_EMITTER_PIN);
unsigned int EEMEM storedMinimumOn[NUM_SENSORS];
unsigned int EEMEM storedMaximumOn[NUM_SENSORS];
unsigned int sensorValues[NUM_SENSORS];

const char warningLowBat[] PROGMEM = "! L32 >e>d>cba8 >e>d>cba8";
const char welcomeUSB[] PROGMEM = "! O5 L8 ceg";
const char welcomeCalibrate[] PROGMEM = "! L8 cdec";
const char welcome[] PROGMEM = "a";//"! T240 O3 L8 f# b.f#16b O4 d4.<b4. d.<b16df#4.d4. f#.d16f#a4.<a4. d.<a16df#2";
const char uhoh[] PROGMEM = "!ec";
  
void setup()
{
  FastGPIO::Pin<BUTTON_GND>::setOutput(LOW);
  ledYellow(1);
  delay(1000);
  ledYellow(0);
  LiftMotorBuzzer::init();
  
  if (batteryLow())
  {
    LiftMotorBuzzer::playFromProgramSpace(warningLowBat);
    blinkForever();
  }
  else
  {    
    /*if (vbusPresent())
    {
      // battery ok; USB connected
      LiftMotorBuzzer::playFromProgramSpace(welcomeUSB);
      blinkForever();
    }
    else*/
    {
      // battery ok; USB disconnected
      if (button.isPressed())
      {
        LiftMotorBuzzer::playFromProgramSpace(welcomeCalibrate);
        ledYellow(1);
        calibrate();
      }
      else
      {
        LiftMotorBuzzer::playFromProgramSpace(welcome);
        ledGreen(1);
        loadStoredCalibration();
        button.waitForButton();
        LiftMotorBuzzer::stopPlaying();
        delay(1000);
        LiftMotorBuzzer::playNote(NOTE_A(5), 200);
        delay(1000);
        LiftMotorBuzzer::playNote(NOTE_A(5), 200);
        delay(1000);
        ThrustMotors::init();
        LiftMotorBuzzer::setSpeed(150);
      }
    }
  }
}

void blinkForever()
{
  while (1)
  {
    if (batteryLow())
    {
      if (vbusPresent())
      {
        // short blinks
        ledGreen(1);
        delay(20);
        ledGreen(0);
        delay(980);      
      }
      else
      {
        // no vbus
        // fast blinks
        ledGreen(1);
        delay(20);
        ledGreen(0);
        delay(180);
      }
    }
    else
    {
      // battery ok
      if (vbusPresent())
      {
        // 50% blinks
        ledGreen(1);
        delay(500);
        ledGreen(0);
        delay(500);      
      }
      else
      {
        // no vbus
        // long blinks
        ledGreen(1);
        delay(900);
        ledGreen(0);
        delay(100);
      }
    } 
  }
}

void calibrate()
{
  button.waitForRelease();
  button.waitForButton();
  
  while (!button.getSingleDebouncedPress())
  {
    lineSensors.calibrate();
    ledYellow(millis() & 1 << 8);
  }
  saveStoredCalibration();
  while(1)
  {
    unsigned int position = lineSensors.readLine(sensorValues);
    LiftMotorBuzzer::playFrequency(position / 4 + 300, 150);
    delay(20);
  }
}

void loop()
{
  //ThrustMotors::setSpeeds( -35,  35); return;
  static int lastP = 0;
  
   int p = 2500 - lineSensors.readLine(sensorValues);
  
  int d = p - lastP;
  
  int diff;

 
  int boost = 0;//abs(p)/50;
  int ct = 20;
  
   diff = p / 15+ d*3;
   diff=max(-75, min(75, diff));
    int fwd = 25;//+abs(p)/100;//max(100 - (abs(diff)/2), 0);
    
    
  if (diff > 0)
    ThrustMotors::setSpeeds(fwd-ct-diff+boost, fwd+ct+diff+boost);
  else
    ThrustMotors::setSpeeds(fwd-ct-diff+boost, fwd+ct+diff+boost);

  lastP = p;
}

/*void loop()
{
  static int lastP = 0;
  int p = (1000 - lineSensors.readLine(sensorValues));
  int d = p - lastP;
  
  int diff = p;// + d*10;
  if (diff < 0)
    ThrustMotors::setSpeeds( 250-diff,  250);
  else
    ThrustMotors::setSpeeds( 250,  250+diff);
  lastP = p;
}*/

inline boolean batteryLow()
{
  return ((unsigned long)analogRead(VBAT_DIV) * 2 * 3300 / 1023) < 3650;
}

inline boolean vbusPresent()
{
  return USBSTA & (1 << VBUS);
}

inline void ledYellow(bool on)
{
  FastGPIO::Pin<13>::setOutput(on);
}

inline void ledGreen(bool on)
{
  FastGPIO::Pin<IO_D5>::setOutput(!on);
}

void saveStoredCalibration()
{ 
  for (uint8_t i = 0; i < NUM_SENSORS; i++)
  {
    eeprom_write_word(&storedMinimumOn[i], lineSensors.calibratedMinimumOn[i]);
    eeprom_write_word(&storedMaximumOn[i], lineSensors.calibratedMaximumOn[i]);
  }
}

void loadStoredCalibration()
{
  lineSensors.calibrate(); // allocate stuff
  
  for (uint8_t i = 0; i < NUM_SENSORS; i++)
  {
    lineSensors.calibratedMinimumOn[i] = eeprom_read_word(&storedMinimumOn[i]);
    lineSensors.calibratedMaximumOn[i] = eeprom_read_word(&storedMaximumOn[i]);
  }
}
