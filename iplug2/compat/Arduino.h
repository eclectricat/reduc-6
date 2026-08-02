#pragma once

#include <cmath>
#include <cstdint>

using byte = std::uint8_t;

#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef DEC
#define DEC 10
#endif

class CookieBoxSerialStub {
public:
  template <typename T>
  void print(const T&) {}

  template <typename T, typename U>
  void print(const T&, const U&) {}

  template <typename T>
  void println(const T&) {}

  template <typename T, typename U>
  void println(const T&, const U&) {}

  void println() {}
  void begin(unsigned long) {}
};

inline CookieBoxSerialStub Serial;

inline std::uint32_t ARM_DWT_CYCCNT = 0;
inline std::uint32_t ARM_DEMCR = 0;
inline std::uint32_t ARM_DWT_CTRL = 0;

#ifndef ARM_DEMCR_TRCENA
#define ARM_DEMCR_TRCENA 0
#endif

#ifndef ARM_DWT_CTRL_CYCCNTENA
#define ARM_DWT_CTRL_CYCCNTENA 0
#endif
