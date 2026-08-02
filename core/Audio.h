#pragma once

#include <stdint.h>
#include <cstddef>

class Audio {
public:
  virtual ~Audio() {}
  virtual void begin() = 0;
  virtual void setSampleRate(uint32_t sampleRate) = 0;
  virtual void process(float *in, float *out, size_t frames) = 0;
};
