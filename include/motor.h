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
    long timestamp{ 0 };

    MTrajectory() {}
    MTrajectory(const std::vector<float>& angs, bool abs = false) : angles(angs), absolute(abs), cursor(0), timestamp(millis()) {}

    float getNextPosition() {
        if (isFinished())
            return 0.f;
        return angles[cursor++];
    }
    bool isFinished() const { return cursor >= angles.size(); }
};

class Motor {
    AccelStepper* _motor;
    int8_t _direction{ 1 };
    MTrajectory _traj;
    bool _trajNotified{ false };

    void _moveNextAngle();

    void errorUninit();
public:
    Motor();
    ~Motor();

    void begin(int8_t direction = 1);
    void loop();

    void moveTo(float absoluteAngle);
    void move(float relativeAngle);
    void moveTrajectory(const MTrajectory& traj);
    void stop();

    const MTrajectory& getTraj() const { return _traj; }
    bool isRunning() const { return _motor->isRunning(); }
    bool hasJustFinishedTrajectory() { if (_traj.isFinished() && !_trajNotified) { _trajNotified = false; return true; } return false; }
};

#endif // MOTOR_H__