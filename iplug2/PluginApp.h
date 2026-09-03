// Minimal Plugin skeleton for iPlug2 integration
// This header is intentionally lightweight — include the real iPlug2 headers
// in your platform-specific implementation files.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include "../core/Display.h"

class SynthEngine;
class GlobalState;
class IPlugDisplay;
class Storage;

class CookieBoxPluginApp {
public:
  CookieBoxPluginApp();
  ~CookieBoxPluginApp();

  // Initialize plugin and core engine
  void initialize(double sampleRate = 44100.0);

  // Called by host/audio thread to process audio (platform-specific adapter)
  void processAudio(double** outputs, int nFrames, int nChannels);
  void processMidiMessage(uint8_t status, uint8_t data1, uint8_t data2);

  // Inject an external display implementation (optional).
  // If not set, an internal 2x16 display implementation is used.
  void setDisplay(Display* disp);

  // UI input surface (mirrors CookieBox.ino wiring)
  // - 4 knobs (0..3), values normalized to [0, 1]
  // - 10 buttons (0..9) mapped from keyboard keys Q W E R T Z U I O P
  void setKnobNormalized(int knobIndex, float normalizedValue);
  void setButtonState(int buttonIndex, bool isDown);
  void setKeyState(char key, bool isDown);

  // Call once per UI frame/timer tick.
  void tickUI();

  // Read back display lines for host drawing.
  std::string getDisplayLine(int row) const;

private:
  static constexpr int kNumKnobs = 4;
  static constexpr int kNumButtons = 10;

  void lockPotentiometers(bool refreshReadingFirst);
  void handleButtonPressed(int buttonIndex);
  void handleButtonReleased(int buttonIndex);
  int keyToButtonIndex(char key) const;
  static int normalizedToPot(float normalized);

  bool initialized = false;

  // Not owned when injected, otherwise points to internalDisplay.
  Display* display = nullptr;
  std::unique_ptr<IPlugDisplay> internalDisplay;

  std::unique_ptr<SynthEngine> engine;
  std::unique_ptr<GlobalState> globalState;
  std::unique_ptr<Storage> storage;

  std::array<int, kNumKnobs> currentPotVals = {0, 0, 0, 0};
  std::array<int, kNumKnobs> potValuesOnParamChange = {0, 0, 0, 0};
  std::array<bool, kNumKnobs> potsMoved = {false, false, false, false};
  std::array<bool, kNumButtons> buttonsDown = {false, false, false, false, false, false, false, false, false, false};
  std::array<char, kNumButtons> keyMap = {'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P'};
  uint32_t mLastPotDisplayUpdateMs = 0;
  uint32_t mLastMaxLevelLogMs = 0;
};
