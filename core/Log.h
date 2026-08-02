#pragma once

#include "System.h"

#include <sstream>
#include <string>

struct CoreLogger {
  CoreLogger() = default;

  template <typename T>
  void print(const T& value) {
    System::log(toString(value));
  }

  template <typename T, typename U>
  void print(const T& value, const U&) {
    // Keep compatibility with Arduino-style Serial.print(value, DEC)
    // by ignoring the second formatting argument for now.
    print(value);
  }

  template <typename T>
  void println(const T& value) {
    System::log(toString(value));
    System::log("\n");
  }

  template <typename T, typename U>
  void println(const T& value, const U&) {
    // Keep compatibility with Arduino-style Serial.println(value, DEC)
    println(value);
  }

  void println() {
    System::log("\n");
  }

private:
  static std::string toString(const std::string& value) { return value; }
  static std::string toString(const char* value) { return value ? std::string(value) : std::string(); }
  static std::string toString(char* value) { return value ? std::string(value) : std::string(); }

  template <typename T>
  static std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }
};

inline CoreLogger CBLog;
