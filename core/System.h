#pragma once

#include <stdint.h>
#include <string>

struct System {
  static uint32_t millis();
  static void delay(uint32_t ms);
  static void log(const std::string &msg);
};
