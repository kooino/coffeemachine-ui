#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>

class MotorController {
public:
    MotorController(int stepPin, int dirPin, int enablePin);
    void begin();
    void setMode(int mode);
    void requestStart();
    void abort();
    void update();

private:
    int _stepPin, _dirPin, _enablePin;
    int _mode;
    bool _isRunning;
    bool _startPending;
    unsigned long _stepsDone;
    unsigned long _lastStepTime;
    unsigned long _startRequestTime;
    unsigned long _stepDelay;

    const int _modeSteps[3] = {0, 400, 556}; // 1 = 0 steps, 2 = lille, 3 = stor
};

#endif
