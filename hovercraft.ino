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

#define FRONT_SENSORS 4
#define REAR_SENSORS 3

QTRSensorsRC frontSensors((unsigned char[]) {3, 2, 1, 0}, FRONT_SENSORS, 2500, QTR_NO_EMITTER_PIN);
QTRSensorsRC rearSensors((unsigned char[]) {4, 12, 6}, REAR_SENSORS, 2500, QTR_NO_EMITTER_PIN);
unsigned int EEMEM storedFrontMinimumOn[FRONT_SENSORS];
unsigned int EEMEM storedFrontMaximumOn[FRONT_SENSORS];
unsigned int EEMEM storedRearMinimumOn[REAR_SENSORS];
unsigned int EEMEM storedRearMaximumOn[REAR_SENSORS];
unsigned int frontSensorValues[FRONT_SENSORS];
unsigned int rearSensorValues[REAR_SENSORS];

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
        LiftMotorBuzzer::setSpeed(200);
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
  boolean playFront = true;
  unsigned int position;
  
  button.waitForRelease();
  button.waitForButton();
  
  while (!button.getSingleDebouncedPress())
  {
    frontSensors.calibrate();
    rearSensors.calibrate();
    ledYellow(millis() & 1 << 8);
  }
  saveStoredCalibration();
  while(1)
  {
    if (button.getSingleDebouncedPress())
      playFront = !playFront;
      
    if (playFront)
    {
      position = frontSensors.readLine(frontSensorValues);
      LiftMotorBuzzer::playFrequency(position / 4 + 300, 150);
    }
    else
    {
      position = rearSensors.readLine(rearSensorValues);
      LiftMotorBuzzer::playFrequency(position / 2 + 200, 150);
    }
    delay(20);
  }
}

void loop()
{//return;
  boolean slow = false;
  //ThrustMotors::setSpeeds( -35,  35); return;
  
  static int lastP = 0;
  
  int p = 1500 - frontSensors.readLine(frontSensorValues);
  
  int d = p - lastP;
  
 if (abs(p) > 500)
    slow = true;
  
  
  int diff, fwd;
  

    diff = p / 20+d*10;
    fwd = 180 - abs(p)/30;
    float boost = 1;
    //float boost = 1+(float)p / 500;
    //boost = min(2, boost);
    
  if (diff < 0)
    ThrustMotors::setSpeeds((fwd-diff)*boost, (fwd+diff)*boost);
  else
    ThrustMotors::setSpeeds((fwd-diff)*boost, (fwd+diff)*boost);

  lastP = p;
  /*Serial.print(rotP);
  Serial.print('\t');
 Serial.println(posP);*/
 int ls = 350 - p;// * 9 / 10;
 ls = max(0, ls);
 boolean burst = (((millis() >> 4) & 0b1100) == 0b1100);
 LiftMotorBuzzer::setSpeed(400 * burst);
 ledYellow(burst);  
}

/*void loop()
{
  static int lastP = 0;
  int p = (1000 - frontSensors.readLine(frontSensorValues));
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
  for (uint8_t i = 0; i < FRONT_SENSORS; i++)
  {
    eeprom_write_word(&storedFrontMinimumOn[i], frontSensors.calibratedMinimumOn[i]);
    eeprom_write_word(&storedFrontMaximumOn[i], frontSensors.calibratedMaximumOn[i]);
  }
  
  for (uint8_t i = 0; i < REAR_SENSORS; i++)
  {
    eeprom_write_word(&storedRearMinimumOn[i], rearSensors.calibratedMinimumOn[i]);
    eeprom_write_word(&storedRearMaximumOn[i], rearSensors.calibratedMaximumOn[i]);
  }
}

void loadStoredCalibration()
{
  frontSensors.calibrate(); // allocate stuff
  rearSensors.calibrate();
  
  for (uint8_t i = 0; i < FRONT_SENSORS; i++)
  {
    frontSensors.calibratedMinimumOn[i] = eeprom_read_word(&storedFrontMinimumOn[i]);
    frontSensors.calibratedMaximumOn[i] = eeprom_read_word(&storedFrontMaximumOn[i]);
  }
   for (uint8_t i = 0; i < FRONT_SENSORS; i++)
  {
    rearSensors.calibratedMinimumOn[i] = eeprom_read_word(&storedRearMinimumOn[i]);
    rearSensors.calibratedMaximumOn[i] = eeprom_read_word(&storedRearMaximumOn[i]);
  }
}
