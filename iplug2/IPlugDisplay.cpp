#include "IPlugDisplay.h"

#include <algorithm>

IPlugDisplay::IPlugDisplay() {
  clear();
}

void IPlugDisplay::clear() {
  for (auto& line : lines) {
    line.assign(kCols, ' ');
  }
  cursorCol = 0;
  cursorRow = 0;
}

void IPlugDisplay::setCursor(int col, int row) {
  cursorRow = std::clamp(row, 0, kRows - 1);
  cursorCol = std::clamp(col, 0, kCols - 1);
}

void IPlugDisplay::print(const std::string& text) {
  int col = cursorCol;
  for (char c : text) {
    if (col >= kCols) break;
    lines[cursorRow][col] = c;
    ++col;
  }
  cursorCol = std::min(col, kCols - 1);
}

std::string IPlugDisplay::getLine(int row) const {
  const int r = std::clamp(row, 0, kRows - 1);
  return lines[r];
}
