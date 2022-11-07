#include "motor.h"

Motor::Motor() {
    _motor = new AccelStepper(AccelStepper::HALF4WIRE, PIN_MOTOR_IN1, PIN_MOTOR_IN3, PIN_MOTOR_IN2, PIN_MOTOR_IN4);
}
Motor::~Motor() {
    delete _motor;
}

void Motor::begin() {
    if (!_motor) {
        return errorUninit();
    }
    // set the speed and acceleration
    _motor->setMaxSpeed(MOTOR_MAX_SPEED);
    _motor->setAcceleration(MOTOR_ACCELERATION);
    
    // // set target position
    // motor.moveTo(MOTOR_STEPS_PER_REVOLUTION);
}

void Motor::loop() {
    if (!_motor) {
        return errorUninit();
    }
    if (!_isRunning) {
        return;
    }
    if (_motor->distanceToGo() == 0) {
        _isRunning = false;
        return;
    }
    _motor->run();
}

void Motor::moveTo(long absolutePosition) {
    _isRunning = true;
    _motor->moveTo(absolutePosition);
}


void Motor::errorUninit() {
    Serial.println(F("AccelStepper uninitialized!"));
}