#include "motor.h"

Motor::Motor() {
    _motor = new AccelStepper(AccelStepper::HALF4WIRE, PIN_MOTOR_IN1, PIN_MOTOR_IN3, PIN_MOTOR_IN2, PIN_MOTOR_IN4);
}
Motor::~Motor() {
    delete _motor; // TODO: fix the warning
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
    if (_motor->distanceToGo() != 0) {
        _motor->enableOutputs();
    }
    _motor->run();
    if (_motor->distanceToGo() == 0) {
        if (_traj.isFinished()) {
            _motor->disableOutputs();
        } else {
#ifdef ALBERT_DEBUG
            Serial.println(F(".. trajectory not finished"));
#endif
            _moveNextAngle();
        }
    }
}

void Motor::_moveNextAngle() {
    float nextAngle = _traj.getNextPosition();
#ifdef ALBERT_DEBUG
    Serial.print(F("Starting moving to "));
    Serial.println(nextAngle);
#endif
    if (_traj.absolute) {
        _motor->moveTo(Util::angle2steps(nextAngle));
    } else {
        _motor->move(Util::angle2steps(nextAngle));
    }
}

// Trajectory angles in degrees
void Motor::moveTrajectory(const MTrajectory& traj) {
#ifdef ALBERT_DEBUG
    Serial.print(F("Moving motor with trajectory ("));
    Serial.print(traj.absolute ? F("abs") : F("rel"));
    Serial.print(F("):"));
    for (const auto& ang : traj.angles) {
        Serial.print(ang);
        Serial.print(F(", "));
    }
    Serial.println();
#endif
    if (traj.angles.empty() || traj.isFinished()) {
        return;
    }

    _traj = traj;
    _moveNextAngle();
}

// Angle in degrees
void Motor::moveTo(float absoluteAngle) {
    moveTrajectory(MTrajectory({absoluteAngle}, true));
}

// Angle in degrees
void Motor::move(float relativeAngle) {
    moveTrajectory(MTrajectory({relativeAngle}));
}

void Motor::stop() {
    _motor->stop();
}

void Motor::errorUninit() {
#ifdef ALBERT_DEBUG
    Serial.println(F("AccelStepper uninitialized!"));
#endif
}