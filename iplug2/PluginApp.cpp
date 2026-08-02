#include "PluginApp.h"

#include "IPlugDisplay.h"

#include "../Modes.h"
#include "../SynthEngine.h"
#include "../core/System.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>

namespace {
constexpr int kPotUnlockThreshold = 30;
constexpr uint32_t kPotDisplayUpdateIntervalMs = 33;
}

CookieBoxPluginApp::CookieBoxPluginApp() {}
CookieBoxPluginApp::~CookieBoxPluginApp() {}

void CookieBoxPluginApp::initialize() {
  if (!display) {
    internalDisplay = std::make_unique<IPlugDisplay>();
    display = internalDisplay.get();
  }

  engine = std::make_unique<SynthEngine>();
  globalState = std::make_unique<GlobalState>(engine.get(), display);

  // Mirrors CookieBox.ino setup order for mode/menu wiring.
  globalState->setup();
  engine->buildEngine(globalState->synthMode.allSynthParameters, globalState->sequencerMode.bpm);

  globalState->synthMode.setup();
  globalState->partConfigMode.setup();
  globalState->sequencerMode.setup();
  globalState->sequencerModeGraphic.setup(&(globalState->sequencerMode));
  globalState->mixMuteMode.setup();
  globalState->keyboardMode.setup();
  globalState->partConfigMode.resetEngineTypeAndVoices();

  lockPotentiometers(true);

  display->clear();
  globalState->selectedMode->fullDisplayUpdate();

  initialized = true;
}

void CookieBoxPluginApp::processAudio(double** outputs, int nFrames, int nChannels) {
  if (!initialized || !engine || !outputs || nFrames <= 0 || nChannels <= 0) return;

  for (int ch = 0; ch < nChannels; ++ch) {
    if (outputs[ch]) {
      std::memset(outputs[ch], 0, static_cast<size_t>(nFrames) * sizeof(double));
    }
  }

  engine->renderBlock(outputs, nFrames, nChannels);
}

void CookieBoxPluginApp::setDisplay(Display* disp) {
  display = disp;
  if (display) {
    internalDisplay.reset();
  }
}

void CookieBoxPluginApp::setKnobNormalized(int knobIndex, float normalizedValue) {
  const int value = normalizedToPot(normalizedValue);
  if (knobIndex < 0 || knobIndex >= kNumKnobs) return;
  if (!initialized || !globalState) return;

  currentPotVals[knobIndex] = value;
  const uint32_t now = System::millis();

  if (potsMoved[knobIndex]) {
    const bool updateDisplay = (now - mLastPotDisplayUpdateMs) >= kPotDisplayUpdateIntervalMs;
    if (updateDisplay) {
      mLastPotDisplayUpdateMs = now;
    }
    globalState->selectedMode->processPotValue(knobIndex, currentPotVals[knobIndex], updateDisplay);
    return;
  }

  if (std::abs(currentPotVals[knobIndex] - potValuesOnParamChange[knobIndex]) > kPotUnlockThreshold) {
    potsMoved[knobIndex] = true;
    mLastPotDisplayUpdateMs = now;
    globalState->selectedMode->processPotValue(knobIndex, currentPotVals[knobIndex], true);
  }
}

void CookieBoxPluginApp::setButtonState(int buttonIndex, bool isDown) {
  if (buttonIndex < 0 || buttonIndex >= kNumButtons) return;
  if (!initialized || !globalState) return;

  if (buttonsDown[buttonIndex] == isDown) return;
  buttonsDown[buttonIndex] = isDown;

  const char keyLabel = keyMap[buttonIndex];
  std::fprintf(stderr, "[PluginApp] button %d (%c) %s\n", buttonIndex, keyLabel, isDown ? "down" : "up");

  if (isDown) {
    handleButtonPressed(buttonIndex);
  } else {
    handleButtonReleased(buttonIndex);
  }
}

void CookieBoxPluginApp::setKeyState(char key, bool isDown) {
  const int buttonIndex = keyToButtonIndex(key);
  if (buttonIndex < 0) {
    std::fprintf(stderr, "[PluginApp] key %c ignored (not mapped)\n", key);
    return;
  }

  std::fprintf(stderr, "[PluginApp] key %c mapped to button %d\n", key, buttonIndex);
  setButtonState(buttonIndex, isDown);
}

void CookieBoxPluginApp::tickUI() {
  if (!initialized || !globalState) return;

  if (engine) {
    const uint32_t now = System::millis();
    if (now - mLastMaxLevelLogMs >= 1000U) {
      mLastMaxLevelLogMs = now;
      const float maxLevel = engine->getAndResetMaxLevel();
      std::fprintf(stderr, "[Audio] SynthEngine.maxSignalLevel=%.6f\n", maxLevel);
    }
  }

  if (globalState->delayedDisplayRefresh > 0) {
    if (static_cast<int32_t>(System::millis()) > globalState->delayedDisplayRefresh) {
      globalState->delayedDisplayRefresh = -1;
      globalState->selectedMode->fullDisplayUpdate();
    }
  }

  if (globalState->selectedMode->parameterRequestingConfirmation != nullptr) {
    globalState->selectedMode->confirmationDisplayNotification();
  }
}

std::string CookieBoxPluginApp::getDisplayLine(int row) const {
  if (auto* memDisplay = dynamic_cast<IPlugDisplay*>(display)) {
    return memDisplay->getLine(row);
  }
  return std::string();
}

void CookieBoxPluginApp::lockPotentiometers(bool refreshReadingFirst) {
  for (int i = 0; i < kNumKnobs; ++i) {
    if (refreshReadingFirst) {
      potValuesOnParamChange[i] = currentPotVals[i];
    }
    potsMoved[i] = false;
    potValuesOnParamChange[i] = currentPotVals[i];
  }
}

void CookieBoxPluginApp::handleButtonPressed(int buttonIndex) {
  const bool needToLock = globalState->selectedMode->pushButtonPressed(buttonIndex);
  if (needToLock) {
    lockPotentiometers(false);
  }
}

void CookieBoxPluginApp::handleButtonReleased(int buttonIndex) {
  const bool needToLock = globalState->selectedMode->pushButtonReleased(buttonIndex);
  if (needToLock) {
    lockPotentiometers(false);
  }
}

int CookieBoxPluginApp::keyToButtonIndex(char key) const {
  char upper = key;
  if (upper >= 'a' && upper <= 'z') {
    upper = static_cast<char>(upper - ('a' - 'A'));
  }

  for (int i = 0; i < kNumButtons; ++i) {
    if (keyMap[i] == upper) return i;
  }
  return -1;
}

int CookieBoxPluginApp::normalizedToPot(float normalized) {
  const float clamped = std::clamp(normalized, 0.0f, 1.0f);
  return static_cast<int>(std::lround(clamped * 1023.0f));
}
