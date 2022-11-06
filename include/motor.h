#ifndef MOTOR_H__
#define MOTOR_H__

#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"

class Motor {
    AccelStepper* motor;
public:
    Motor() {
        motor = new AccelStepper(AccelStepper::HALF4WIRE, PIN_MOTOR_IN1, PIN_MOTOR_IN3, PIN_MOTOR_IN2, PIN_MOTOR_IN4);
    }
    ~Motor() {
        delete motor;
    }

    void begin() {
        if (!motor) {
            return;
        }
        // set the speed and acceleration
        motor->setMaxSpeed(MOTOR_MAX_SPEED);
        motor->setAcceleration(MOTOR_ACCELERATION);
        
        // // set target position
        // motor.moveTo(MOTOR_STEPS_PER_REVOLUTION);
    }

    void loop() {
        if (!motor) {
            return;
        }
        // // check current stepper motor position to invert direction
        // if (motor.distanceToGo() == 0){
        // motor.moveTo(-motor.currentPosition());
        // Serial.println("Changing direction");
        // }
        // // move the stepper motor (one step at a time)
        // motor.run();
    }
};

#endif // MOTOR_H__