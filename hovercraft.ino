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
    LiftMotorBuzzer::playFrequency(position / 2 + 500, 150);
    delay(20);
  }
}

void loop()
{
  static boolean cut = false, slowMode = false;
  static unsigned int cutTime = 0;
  //LiftMotorBuzzer::setSpeed(125);
  //ThrustMotors::setSpeeds( -20,  20); return;
  static int lastP = 0;
  
   int p = 1000 - lineSensors.readLine(sensorValues);
  
  int d = p - lastP;
  
  int diff;

  float boost = 1;//abs(p)/600;
  int fwd = 20;
  int bwd = 100;
  if (slowMode)
  {

  
   diff = p / 16 + d*4;
    if (diff < 0)
    ThrustMotors::setSpeeds(-bwd-diff, -bwd+diff);
  else
    ThrustMotors::setSpeeds(-bwd-diff, -bwd+diff);
  }
  else
  {

  
   diff = p / 15 + d*7;
  if (diff < 0)
    ThrustMotors::setSpeeds(fwd+ boost*-diff, fwd+ boost+diff/2);
  else
    ThrustMotors::setSpeeds(fwd+  boost-diff/2, fwd+ boost*+diff);
  }
  lastP = p;
  
  //max(70, 120 - abs(p) / 10));
  /*if (isBad && !wasBad)
  {
    LiftMotorBuzzer::setSpeed(0);
    ThrustMotors::setSpeeds(0, 0);
    delay(2000);
  }*/
  
  if (!cut && abs(p) == 1000)
  {
    //LiftMotorBuzzer::setSpeed(0);
    cut = true;
    slowMode = true;
    cutTime = millis();
  }
  else if (cut && (unsigned int)(millis() - cutTime) > 200)
  {
    LiftMotorBuzzer::setSpeed(150);
    slowMode = false;
  }
  if (abs(p) < 500)
  {
    cut = false;
    LiftMotorBuzzer::setSpeed(150);
    slowMode = false;
  }
  ledYellow(slowMode);
  /*if (sensorValues[0] > 700 && sensorValues[1] > 700 && sensorValues[2] > 700)
  {
    LiftMotorBuzzer::setSpeed(0);
    ThrustMotors::setSpeeds(0, 0);
    LiftMotorBuzzer::play(uhoh);
    while(1) {}
  }*/
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
