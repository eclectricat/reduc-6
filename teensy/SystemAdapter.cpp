#include "../core/System.h"
#include <Arduino.h>

uint32_t System::millis() { return ::millis(); }
void System::delay(uint32_t ms) { ::delay(ms); }
void System::log(const std::string &msg) { Serial.print(msg.c_str()); }
