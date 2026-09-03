#include "PluginApp.h"

#include "IPlugDisplay.h"

#include "../Modes.h"
#include "../SynthEngine.h"
#include "../core/Storage.h"
#include "../core/System.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace {
constexpr int kPotUnlockThreshold = 30;
constexpr uint32_t kPotDisplayUpdateIntervalMs = 33;

class PluginStorage final : public Storage {
public:
  PluginStorage() {
    const char* home = std::getenv("HOME");
    if (home && home[0] != '\0') {
      mBasePath = std::filesystem::path(home) / "Library" / "Application Support" / "CookieBox";
    } else {
      mBasePath = std::filesystem::temp_directory_path() / "CookieBox";
    }
  }

  bool begin() override {
    std::error_code ec;
    std::filesystem::create_directories(mBasePath, ec);
    return !ec;
  }

  bool exists(const std::string& path) override {
    std::error_code ec;
    return std::filesystem::exists(resolve(path), ec) && !ec;
  }

  bool mkdir(const std::string& path) override {
    std::error_code ec;
    std::filesystem::create_directories(resolve(path), ec);
    return !ec;
  }

  bool remove(const std::string& path) override {
    std::error_code ec;
    std::filesystem::remove_all(resolve(path), ec);
    return !ec;
  }

  bool writeLines(const std::string& path, const std::vector<std::string>& lines) override {
    const auto fullPath = resolve(path);
    std::error_code ec;
    std::filesystem::create_directories(fullPath.parent_path(), ec);
    if (ec) return false;

    std::ofstream out(fullPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return false;

    for (size_t i = 0; i < lines.size(); ++i) {
      out << lines[i];
      if (i + 1 < lines.size()) {
        out << '\n';
      }
    }

    return out.good() || out.eof();
  }

  bool readLines(const std::string& path, std::vector<std::string>& outLines) override {
    std::ifstream in(resolve(path));
    if (!in.is_open()) return false;

    outLines.clear();
    std::string line;
    while (std::getline(in, line)) {
      outLines.push_back(line);
    }

    return in.good() || in.eof();
  }

  bool listDir(const std::string& path, std::vector<std::string>& outEntries) override {
    const auto fullPath = resolve(path);
    std::error_code ec;
    if (!std::filesystem::exists(fullPath, ec) || ec) return false;

    outEntries.clear();
    for (const auto& entry : std::filesystem::directory_iterator(fullPath, ec)) {
      if (ec) return false;
      outEntries.push_back(entry.path().filename().string());
    }

    return true;
  }

private:
  std::filesystem::path resolve(const std::string& path) const {
    std::filesystem::path relative(path);
    if (relative.is_absolute()) {
      relative = relative.relative_path();
    }
    return mBasePath / relative;
  }

  std::filesystem::path mBasePath;
};
}

CookieBoxPluginApp::CookieBoxPluginApp() {}
CookieBoxPluginApp::~CookieBoxPluginApp() {}

void CookieBoxPluginApp::initialize(double sampleRate) {
  if (sampleRate <= 1.0) {
    sampleRate = 44100.0;
  }

  gAudioSampleRate = static_cast<float>(sampleRate);

  if (!display) {
    internalDisplay = std::make_unique<IPlugDisplay>();
    display = internalDisplay.get();
  }

  engine = std::make_unique<SynthEngine>();
  globalState = std::make_unique<GlobalState>(engine.get(), display);
  storage = std::make_unique<PluginStorage>();
  storage->begin();
  globalState->setStorage(storage.get());

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

void CookieBoxPluginApp::processMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
  if (!initialized || !globalState) return;

  switch (status) {
    case 0xF8: // MIDI clock (24 PPQN)
      globalState->sequencerMode.tick();
      return;
    case 0xFA: // MIDI start
      globalState->seqPlaying = true;
      globalState->sequencerMode.startSync();
      globalState->sequencerMode.displayPlayStatus();
      return;
    case 0xFB: // MIDI continue
      globalState->seqPlaying = true;
      globalState->sequencerMode.continueSync();
      globalState->sequencerMode.displayPlayStatus();
      return;
    case 0xFC: // MIDI stop
      globalState->seqPlaying = false;
      globalState->sequencerMode.stopSync();
      globalState->sequencerMode.displayPlayStatus();
      return;
    default:
      break;
  }

  // Channel voice fallback: allow host keyboard MIDI to trigger engine directly.
  const uint8_t statusHi = status & 0xF0;
  const uint8_t channel = (status & 0x0F) + 1;

  if (statusHi == 0x90) {
    if (data2 == 0) {
      globalState->myNoteOff(channel, data1, data2);
    } else {
      globalState->myNoteOn(channel, data1, data2);
    }
  } else if (statusHi == 0x80) {
    globalState->myNoteOff(channel, data1, data2);
  }
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

  if (globalState->seqPlaying && !globalState->sequencerMode.playingSync) {
    // Internal sequencer clock; tempo is derived from current BPM in maybePlay().
    globalState->sequencerMode.maybePlay();
  }

  if (engine) {
    const uint32_t now = System::millis();
    if (now - mLastMaxLevelLogMs >= 1000U) {
      mLastMaxLevelLogMs = now;
      const float maxLevel = engine->getAndResetMaxLevel();
      //std::fprintf(stderr, "[Audio] SynthEngine.maxSignalLevel=%.6f\n", maxLevel);
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
