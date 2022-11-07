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
}

void Motor::loop() {
    if (!_motor) {
        return errorUninit();
    }
    if (!isRunning()) {
        return;
    }
    if (_motor->distanceToGo() == 0) {
        return;
    }
    _motor->run();
}

void Motor::moveTo(long absolutePosition) {
    _motor->moveTo(absolutePosition);
}

void Motor::moveTrajectory(const MTrajectory& traj) {
    // TODO: implement move trajectory
}

void Motor::stop() {
    _motor->stop();
}


void Motor::errorUninit() {
    Serial.println(F("AccelStepper uninitialized!"));
}