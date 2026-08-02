#include "TeensySystem.h"
#include <Arduino.h>

uint32_t TeensySystem::millis() {
  return ::millis();
}

void TeensySystem::delay(uint32_t ms) {
  ::delay(ms);
}

void TeensySystem::log(const std::string &msg) {
  Serial.print(msg.c_str());
}
