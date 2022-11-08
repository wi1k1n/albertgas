#ifndef MOTOR_H__
#define MOTOR_H__

#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"
#include "util.h"

struct MTrajectory {
    std::vector<float> angles;
    bool absolute{ false };
    uint8_t cursor{ 0 };

    MTrajectory() {}
    MTrajectory(const std::vector<float>& angs, bool abs = false) : angles(angs), absolute(abs), cursor(0) {}

    float getNextPosition() {
        if (isFinished())
            return 0.f;
        return angles[cursor++];
    }
    bool isFinished() const { return cursor >= angles.size(); }
};

class Motor {
    AccelStepper* _motor;
    MTrajectory _traj;

    void _moveNextAngle();

    void errorUninit();
public:
    Motor();
    ~Motor();

    void begin();
    void loop();

    void moveTo(float absoluteAngle);
    void move(float relativeAngle);
    void moveTrajectory(const MTrajectory& traj);
    void stop();

    bool isRunning() const { return _motor->isRunning(); }
};

#endif // MOTOR_H__