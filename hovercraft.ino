#include <Wire.h> // not used, but Zumo 32U4 libs won't compile without this
#include <FastGPIO.h>
#include <Pushbutton.h>
#include "HovercraftMotors.h"
#include "LiftMotorBuzzer.h"

#define VBAT_DIV     A1
#define BUTTON_INPUT A0
#define BUTTON_GND   11

Pushbutton button(BUTTON_INPUT);

const char warningLowBat[] PROGMEM = "! L32 >e>d>cba8 >e>d>cba8";
const char welcomeUSB[] PROGMEM = "! O5 L8 ceg";
const char welcome[] PROGMEM = "! T240 O3 L8 f# b.f#16b O4 d4.<b4. d.<b16df#4.d4. f#.d16f#a4.<a4. d.<a16df#2";
  
void setup()
{
  LiftMotorBuzzer::init();
  
  if (batteryLow())
  {
    LiftMotorBuzzer::playFromProgramSpace(warningLowBat);
    blinkForever();
  }
  else
  {    
    if (vbusPresent())
    {
      // battery ok; USB connected
      LiftMotorBuzzer::playFromProgramSpace(welcomeUSB);
      blinkForever();
    }
    else
    {
      // battery ok; USB disconnected
      LiftMotorBuzzer::playFromProgramSpace(welcome);
      ledGreen(1);
      FastGPIO::Pin<BUTTON_GND>::setOutput(LOW);
      button.waitForButton();
      LiftMotorBuzzer::stopPlaying();
      delay(1000);
      LiftMotorBuzzer::setSpeed(150);
      delay(3000);
      ThrustMotors::init();
      ThrustMotors::setSpeeds(100, 100);
      delay(2000);
      ThrustMotors::setSpeeds(0, 0);
      delay(1000);
      LiftMotorBuzzer::setSpeed(0);
    }
  }
}

void loop()
{
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
