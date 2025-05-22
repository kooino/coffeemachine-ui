#ifndef PUMPCONTROLLER_H
#define PUMPCONTROLLER_H

#include <Arduino.h>

class PumpController {
public:
    PumpController(int pumpPin, int ledPin);
    void begin();
    void handleSerial();
    void update();
    void startPump(int mode);
    void abortPump();

private:
    int _pumpPin;
    int _ledPin;
    int _mode;  // 0 = ikke valgt, 1/2/3 = modes
    unsigned long _onDurations[3] = {5400, 5400, 7500}; // Pumpetider i ms
    bool _isPumping;
    unsigned long _pumpStartTime;
};

#endif