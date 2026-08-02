#pragma once

#include <stdint.h>
#include <string>

// Minimal system abstraction for timing and logging on Teensy
class TeensySystem {
public:
  static uint32_t millis();
  static void delay(uint32_t ms);
  static void log(const std::string &msg);
};
