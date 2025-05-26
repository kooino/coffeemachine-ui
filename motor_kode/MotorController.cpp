#include "MotorController.h"

MotorController::MotorController(int stepPin, int dirPin, int enablePin)
    : _stepPin(stepPin), _dirPin(dirPin), _enablePin(enablePin),
      _mode(1), _isRunning(false), _startPending(false),
      _stepsDone(0), _lastStepTime(0), _startRequestTime(0),
      _stepDelay(3000)
{}

void MotorController::begin() {
    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    pinMode(_enablePin, OUTPUT);
    digitalWrite(_dirPin, HIGH);
    digitalWrite(_stepPin, LOW);
    digitalWrite(_enablePin, HIGH); // Deaktiver driver (HIGH = slukket)
    Serial.println("MotorController klar – venter på I2C input.");
}

void MotorController::setMode(int mode) {
    if (mode >= 1 && mode <= 3) {
        _mode = mode;
        Serial.print("Mode sat til: ");
        Serial.println(_mode);
    } else {
        Serial.println("Ugyldigt mode ignoreret.");
    }
}

void MotorController::requestStart() {
    if (_isRunning || _startPending) {
        Serial.println("Motor kører allerede – ignorerer start.");
        return;
    }
    _startPending = true;
    _startRequestTime = millis();
    _stepsDone = 0;
    digitalWrite(_enablePin, LOW); // Aktiver driver
    Serial.println("Start anmodning modtaget – venter 5 sekunder...");
}

void MotorController::abort() {
    if (_isRunning || _startPending) {
        _isRunning = false;
        _startPending = false;
        _stepsDone = 0;
        digitalWrite(_stepPin, LOW);
        digitalWrite(_enablePin, HIGH); // Sluk driver
        Serial.println("Motor afbrudt af bruger.");
    }
}

void MotorController::update() {
    if (_startPending && millis() - _startRequestTime >= 5000) {
        _startPending = false;
        _isRunning = true;
        _stepsDone = 0;
        Serial.print("Starter motor i mode ");
        Serial.print(_mode);
        Serial.print(" (");
        Serial.print(_modeSteps[_mode - 1]);
        Serial.println(" steps)");
    }

    if (_isRunning) {
        unsigned long now = micros();
        if (now - _lastStepTime >= _stepDelay) {
            _lastStepTime = now;

            digitalWrite(_stepPin, HIGH);
            delayMicroseconds(10000);
            digitalWrite(_stepPin, LOW);

            _stepsDone++;

            Serial.print("Step ");
            Serial.print(_stepsDone);
            Serial.print(" / ");
            Serial.println(_modeSteps[_mode - 1]);

            if (_stepsDone >= _modeSteps[_mode - 1]) {
                _isRunning = false;
                digitalWrite(_enablePin, HIGH); // Sluk driver
                Serial.println("Motor færdig.");
            }
        }
    }
}
