#include "HovercraftMotors.h"
#include <FastGPIO.h>

#define PWM_LIFT 5
#define PWM_THRUST_L 10
#define PWM_THRUST_R 9
#define DIR_THRUST_L 8
#define DIR_THRUST_R 7

void LiftMotor::init()
{
  FastGPIO::Pin<PWM_LIFT>::setOutputLow();

  // prescaler: clockI/O / 1
  // output A enabled
  // phase-correct PWM
  // top of 400
  //
  // PWM frequency calculation
  // 16MHz / 1 (prescaler) / 2 (phase-correct) / 400 (top) = 20kHz

  TCCR3A = 0b10000000;
  TCCR3B = 0b00010001;
  ICR3 = 400;
  OCR3A = 0;
}

void LiftMotor::setSpeed(uint16_t speed)
{
  if (speed > 400)    // Max PWM duty cycle.
  {
      speed = 400;
  }
  
  OCR3A = speed;
}

void ThrustMotors::init()
{
  FastGPIO::Pin<PWM_THRUST_L>::setOutputLow();
  FastGPIO::Pin<PWM_THRUST_R>::setOutputLow();
  FastGPIO::Pin<DIR_THRUST_L>::setOutputLow();
  FastGPIO::Pin<DIR_THRUST_R>::setOutputLow();
  
  // prescaler: clockI/O / 1
  // outputs enabled
  // phase-correct PWM
  // top of 400
  //
  // PWM frequency calculation
  // 16MHz / 1 (prescaler) / 2 (phase-correct) / 400 (top) = 20kHz
  
  TCCR1A = 0b10100000;
  TCCR1B = 0b00010001;
  ICR1 = 400;
  OCR1A = 0;
  OCR1B = 0;
}

// set speed for left motor; speed is a number between -400 and 400
void ThrustMotors::setLeftSpeed(int16_t speed)
{
    bool reverse = 0;

    if (speed < 0)
    {
        speed = -speed; // Make speed a positive quantity.
        reverse = 1;    // Preserve the direction.
    }
    if (speed > 400)    // Max PWM duty cycle.
    {
        speed = 400;
    }

    OCR1B = speed;

    FastGPIO::Pin<DIR_THRUST_L>::setOutput(reverse);
}

// set speed for right motor; speed is a number between -400 and 400
void ThrustMotors::setRightSpeed(int16_t speed)
{
    bool reverse = 0;

    if (speed < 0)
    {
        speed = -speed;  // Make speed a positive quantity.
        reverse = 1;     // Preserve the direction.
    }
    if (speed > 400)     // Max PWM duty cycle.
    {
        speed = 400;
    }

    OCR1A = speed;

    FastGPIO::Pin<DIR_THRUST_R>::setOutput(reverse);
}

// set speed for both motors
void ThrustMotors::setSpeeds(int16_t leftSpeed, int16_t rightSpeed)
{
  setLeftSpeed(leftSpeed);
  setRightSpeed(rightSpeed);
}

