#pragma once

#include "../core/Display.h"
#include <LiquidCrystal.h>
#include <string>

class TeensyDisplay : public Display {
public:
  TeensyDisplay(LiquidCrystal* lcd) : lcd(lcd) {}
  virtual ~TeensyDisplay() {}

  void clear() override { if (lcd) lcd->clear(); }
  void setCursor(int col, int row) override { if (lcd) lcd->setCursor(col, row); }
  void print(const std::string &text) override { if (lcd) lcd->print(text.c_str()); }

private:
  LiquidCrystal* lcd;
};
