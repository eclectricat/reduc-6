#pragma once

#include "../core/Audio.h"

class TeensyAudio : public Audio {
public:
  TeensyAudio() : sampleRate(44100) {}
  virtual ~TeensyAudio() {}

  void begin() override {}
  void setSampleRate(uint32_t sr) override { sampleRate = sr; }
  void process(float *in, float *out, size_t frames) override {
    // No-op stub: audio handled by Teensy Audio library graph (engine)
  }

private:
  uint32_t sampleRate;
};
