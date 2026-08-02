#include "TeensyMidi.h"
#include <Arduino.h>
#include <usb_midi.h>

void TeensyMidi::begin() {
  // Nothing special: usbMIDI callbacks are set in the sketch
}

void TeensyMidi::sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  // usbMIDI uses (note, velocity, channel)
  usbMIDI.sendNoteOn(note, velocity, channel);
}

void TeensyMidi::sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  usbMIDI.sendNoteOff(note, velocity, channel);
}
