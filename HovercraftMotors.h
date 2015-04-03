#pragma once

#include <stdint.h>

class LiftMotor
{
  public:
    static void init();
    static void setSpeed(uint16_t speed);
};

class ThrustMotors
{
  public:
    static void init();
    static void setLeftSpeed(int16_t speed);
    static void setRightSpeed(int16_t speed);
    static void setSpeeds(int16_t leftSpeed, int16_t rightSpeed);
};

