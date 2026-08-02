#include "../core/System.h"

#include <chrono>
#include <iostream>
#include <thread>

uint32_t System::millis() {
  using namespace std::chrono;
  static const auto t0 = steady_clock::now();
  return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now() - t0).count());
}

void System::delay(uint32_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void System::log(const std::string& msg) {
  std::cout << msg;
}
