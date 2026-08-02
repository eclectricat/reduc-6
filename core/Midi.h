#pragma once

#include <stdint.h>

class Midi {
public:
  virtual ~Midi() {}
  virtual void begin() = 0;
  virtual void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
  virtual void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
};
