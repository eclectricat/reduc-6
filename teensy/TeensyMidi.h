#pragma once

#include "../core/Midi.h"

class TeensyMidi : public Midi {
public:
  TeensyMidi() {}
  virtual ~TeensyMidi() {}

  void begin() override;
  void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override;
  void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) override;
};
