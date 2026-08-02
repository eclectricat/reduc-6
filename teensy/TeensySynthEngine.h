#pragma once

#include <Arduino.h>
#include <AudioStream.h>

#include "../SynthEngine.h"

class TeensySynthEngine : public SynthEngine, public AudioStream {
public:
  TeensySynthEngine() : AudioStream(0, nullptr) {}

  void update(void) override;

private:
  void renderBlock(int16_t* mainL, int16_t* mainR, int16_t* directOut[NB_PARTS], size_t frames);
};
