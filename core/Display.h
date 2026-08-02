#ifndef COOKIEBOX_DISPLAY_H
#define COOKIEBOX_DISPLAY_H

#include <string>

// Abstract display interface used by core code (platform implementations provide concrete classes)
class Display {
public:
  virtual ~Display() {}

  // Clear the display contents
  virtual void clear() = 0;

  // Set cursor position for text printing (column, row)
  virtual void setCursor(int col, int row) = 0;

  // Print text at current cursor position (non-real-time)
  virtual void print(const std::string &text) = 0;

  // Convenience overload
  virtual void print(const char *text) { print(std::string(text)); }

  // Numeric convenience overloads
  virtual void print(int value) { print(std::to_string(value)); }
  virtual void print(unsigned int value) { print(std::to_string(value)); }
  virtual void print(long value) { print(std::to_string(value)); }
  virtual void print(unsigned long value) { print(std::to_string(value)); }
};

#endif // COOKIEBOX_DISPLAY_H
