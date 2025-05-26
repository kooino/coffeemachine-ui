#include <Wire.h>
#include "PumpController.h"

#define PUMP_PIN 12
#define LED_PIN 13
#define HEATER_PIN 7

PumpController pumpController(PUMP_PIN, LED_PIN);

int currentMode = 0;

void receiveCommand(int bytes) {
  String cmd = "";
  while (Wire.available()) cmd += (char)Wire.read();
  cmd.trim();

  if (cmd.startsWith("mode:")) {
    currentMode = cmd.substring(5).toInt();
    Serial.print("📥 Mode sat via I2C: "); Serial.println(currentMode);
  }
  else if (cmd == "s") {
    if (currentMode) pumpController.startPump(currentMode);
    else Serial.println("⚠️ Mode ikke valgt endnu");
  }
  else if (cmd == "a") {
    pumpController.abortPump();
    Serial.println("🛑 Pumpen stoppet via I2C kommando 'a'");
  }
}

void setup() {
  Serial.begin(115200);
  pumpController.begin();
  pinMode(HEATER_PIN, OUTPUT);
  Wire.begin(0x08); // I2C-adresse
  Wire.onReceive(receiveCommand);
}

void loop() {
  pumpController.update();

  static unsigned long prev = 0;
  static bool heaterOn = false;
  unsigned long now = millis();

  if (!heaterOn && now - prev > 30000) {
    digitalWrite(HEATER_PIN, HIGH);
    heaterOn = true;
    prev = now;
  }

  if (heaterOn && now - prev > 5000) {
    digitalWrite(HEATER_PIN, LOW);
    heaterOn = false;
    prev = now;
  }
}