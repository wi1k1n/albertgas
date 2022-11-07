#ifndef MOTOR_H__
#define MOTOR_H__

#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"

struct MTrajectory {
    std::vector<uint16_t> positions;
    // MTrajectory(const std::vector<uint16_t>& poss) : positions(poss) {}
};

class Motor {
    AccelStepper* _motor;

    void errorUninit();
public:
    Motor();
    ~Motor();

    void begin();
    void loop();

    void moveTo(long absolutePosition);
    void move(long relative);
    void moveTrajectory(const MTrajectory& traj);
    void stop();

    bool isRunning() const { return _motor->isRunning(); }
};

#endif // MOTOR_H__