#ifndef MOTOR_H__
#define MOTOR_H__

#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"

class Motor {
    AccelStepper* _motor;
    bool _isRunning{ false };

    void errorUninit();
public:
    Motor();
    ~Motor();

    void begin();
    void loop();

    void moveTo(long absolutePosition);

    bool isRunning() const { return _isRunning; }
};

#endif // MOTOR_H__