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

QTRSensorsRC lineSensors((unsigned char[]) {4, 3, 2}, 3, 2500, QTR_NO_EMITTER_PIN);
unsigned int EEMEM storedMinimumOn[3];
unsigned int EEMEM storedMaximumOn[3];
unsigned int sensorValues[3];

const char warningLowBat[] PROGMEM = "! L32 >e>d>cba8 >e>d>cba8";
const char welcomeUSB[] PROGMEM = "! O5 L8 ceg";
const char welcomeCalibrate[] PROGMEM = "! L8 cdec";
const char welcome[] PROGMEM = "a";//"! T240 O3 L8 f# b.f#16b O4 d4.<b4. d.<b16df#4.d4. f#.d16f#a4.<a4. d.<a16df#2";
  
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
        delay(3000);
        ThrustMotors::init();
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
    LiftMotorBuzzer::playFrequency(position / 2 + 500, 150);
    delay(20);
  }
}

void loop()
{
  static boolean wasBad = false;
  //ThrustMotors::setSpeeds( 35,  55); return;
  static int lastP = 0;
  int p = 1000 - lineSensors.readLine(sensorValues);
  boolean isBad;
  if (wasBad)
    isBad = (abs(p) > 250);
  else
    isBad = (abs(p) > 600);
  
  int d = p - lastP;
  
  int diff = p / 10 + d*5;
  int boost = abs(p)/30;
  if (diff < 0)
    ThrustMotors::setSpeeds( 35+boost-diff,  55+boost);
  else
    ThrustMotors::setSpeeds( 35+boost,  55+boost+diff);
  lastP = p;
  
  LiftMotorBuzzer::setSpeed(max(120, 150 - abs(p) / 10));
  if (isBad && !wasBad)
  {
    LiftMotorBuzzer::setSpeed(0);
    ThrustMotors::setSpeeds(0, 0);
    delay(2000);
  }
  wasBad = isBad;
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
  for (uint8_t i = 0; i < 3; i++)
  {
    eeprom_write_word(&storedMinimumOn[i], lineSensors.calibratedMinimumOn[i]);
    eeprom_write_word(&storedMaximumOn[i], lineSensors.calibratedMaximumOn[i]);
  }
}

void loadStoredCalibration()
{
  lineSensors.calibrate(); // allocate stuff
  
  for (uint8_t i = 0; i < 3; i++)
  {
    lineSensors.calibratedMinimumOn[i] = eeprom_read_word(&storedMinimumOn[i]);
    lineSensors.calibratedMaximumOn[i] = eeprom_read_word(&storedMaximumOn[i]);
  }
}
