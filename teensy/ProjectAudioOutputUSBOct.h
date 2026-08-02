#pragma once

#include <Audio.h>

// Project-local 8-channel USB audio wrapper to avoid editing Teensy core files.
#ifdef AUDIO_INTERFACE
#include <usb_audio.h>

class ProjectAudioOutputUSBOct : public AudioOutputUSB {
public:
  ProjectAudioOutputUSBOct() : AudioOutputUSB() {}
  // expose begin() for parity with core usage
  void begin() { AudioOutputUSB::begin(); }
};
#else
// Fallback stub when AUDIO_INTERFACE is not available during build.
// Provides AudioStream-compatible surface so AudioConnection code compiles.
class ProjectAudioOutputUSBOct : public AudioStream {
public:
  ProjectAudioOutputUSBOct() : AudioStream(2, inputQueueArray) {}
  void begin() {}
  virtual void update() {}
private:
  audio_block_t *inputQueueArray[2];
};
#endif
