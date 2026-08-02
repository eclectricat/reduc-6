#pragma once

#include "../core/Display.h"

#include <array>
#include <string>

// Small in-memory 2x16 display used by the iPlug2 app layer.
class IPlugDisplay : public Display {
public:
  IPlugDisplay();

  void clear() override;
  void setCursor(int col, int row) override;
  void print(const std::string& text) override;

  std::string getLine(int row) const;

private:
  static constexpr int kCols = 16;
  static constexpr int kRows = 2;

  int cursorCol = 0;
  int cursorRow = 0;
  std::array<std::string, kRows> lines;
};
