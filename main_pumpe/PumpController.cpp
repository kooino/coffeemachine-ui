#include "PumpController.h"

PumpController::PumpController(int pumpPin, int ledPin)
    : _pumpPin(pumpPin), _ledPin(ledPin), _mode(0),
      _isPumping(false), _pumpStartTime(0) {}

void PumpController::begin() {
    pinMode(_pumpPin, OUTPUT);
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_pumpPin, LOW);
    digitalWrite(_ledPin, LOW);
    Serial.println("PumpController klar. Vælg mode (1/2/3), og tryk 's' for at starte.");
}

void PumpController::handleSerial() {
    if (Serial.available()) {
        char input = Serial.read();

        if (input == '1' || input == '2' || input == '3') {
            _mode = input - '0';
            Serial.print("Mode sat til: ");
            Serial.println(_mode);
        } else if (input == 's') {
            startPump(_mode);
        }
    }
}

void PumpController::update() {
    if (_isPumping) {
        if (millis() - _pumpStartTime >= _onDurations[_mode - 1]) {
            digitalWrite(_pumpPin, LOW);
            digitalWrite(_ledPin, LOW);
            _isPumping = false;
            Serial.println("Pumpen er SLUKKET (tiden er udløbet).\n");
        }
    }
}

void PumpController::startPump(int mode) {
    if (mode >= 1 && mode <= 3) {
        _mode = mode;
        digitalWrite(_pumpPin, HIGH);
        digitalWrite(_ledPin, HIGH);
        _isPumping = true;
        _pumpStartTime = millis();
        Serial.print("Pumpen STARTET i mode ");
        Serial.print(_mode);
        Serial.print(" (");
        Serial.print(_onDurations[_mode - 1] / 1000);
        Serial.println(" sekunder)\n");
    } else {
        Serial.println("Ugyldig mode i startPump().\n");
    }
}

void PumpController::abortPump() {
    if (_isPumping) {
        digitalWrite(_pumpPin, LOW);
        digitalWrite(_ledPin, LOW);
        _isPumping = false;
        Serial.println("Pumpen er blevet afbrudt.\n");
    }
}