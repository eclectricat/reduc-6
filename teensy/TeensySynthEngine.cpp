#include "TeensySynthEngine.h"

void TeensySynthEngine::renderBlock(int16_t* mainL, int16_t* mainR, int16_t* directOut[NB_PARTS], size_t frames) {
  if (outputSignal == NULL) return;

  float currentSample[2];
  float tempval;

  for (size_t i = 0; i < frames; i++) {
    // update all Signals in order
    for (int s = 0; s < registry.nbActiveSignals; s++) {
      registry.activeSignals[s]->update_instrumented();
    }

    for (int s = 0; s < 2; s++) {
      tempval = outputSignal->getValue(s);
      if (abs(tempval) > maxSignalLevel) {
        maxSignalLevel = abs(tempval);
      }

      currentSample[s] = 0.5f * fast_tanh(tempval * 2); // softclip
    }

    mainL[i] = (short)(55000 * currentSample[0]);   // TODO: do all the scaling in a single place
    mainR[i] = (short)(55000 * currentSample[1]);   // TODO: do all the scaling in a single place

    for (int part = 0; part < NB_PARTS; part++) {
      if (directOut[part] != nullptr) {
        directOut[part][i] = (short)(55000 * (signals[part]->getValue() + signals[part + NB_PARTS]->getValue()));
      }
    }
  }
}

void TeensySynthEngine::update(void) {
  audio_block_t* blockL = allocate();
  if (blockL == nullptr) return;

  audio_block_t* blockR = allocate();
  if (blockR == nullptr) {
    release(blockL);
    return;
  }

  audio_block_t* directBlocks[NB_PARTS];
  for (int i = 0; i < NB_PARTS; i++) {
    directBlocks[i] = allocate();
    if (directBlocks[i] == nullptr) {
      release(blockL);
      release(blockR);
      for (int j = 0; j < i; j++) {
        release(directBlocks[j]);
      }
      return;
    }
  }

  int16_t* directPtrs[NB_PARTS];
  for (int i = 0; i < NB_PARTS; i++) {
    directPtrs[i] = directBlocks[i]->data;
  }

  renderBlock(blockL->data, blockR->data, directPtrs, AUDIO_BLOCK_SAMPLES);

  transmit(blockL, 0);
  transmit(blockR, 1);

  for (int i = 0; i < NB_PARTS; i++) {
    transmit(directBlocks[i], i + 2);
  }

  release(blockL);
  release(blockR);
  for (int i = 0; i < NB_PARTS; i++) {
    release(directBlocks[i]);
  }
}
