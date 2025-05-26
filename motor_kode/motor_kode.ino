#include <Wire.h>
#include "MotorController.h"

MotorController motor(9, 8, 7); // stepPin = 9, dirPin = 8, enablePin = 7

void onReceive(int numBytes) {
  while (Wire.available()) {
    char cmd = Wire.read();

    if (cmd >= '1' && cmd <= '3') {
      motor.setMode(cmd - '0');
    } else if (cmd == 's') {
      motor.requestStart();
    } else if (cmd == 'a') {
      motor.abort();
    }
  }
}

void setup() {
  Serial.begin(115200);
  motor.begin();
  Wire.begin(0x30); // I2C adresse til RPi
  Wire.onReceive(onReceive);
}

void loop() {
  motor.update();
}
