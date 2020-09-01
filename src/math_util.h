#pragma once

#include <cstdint>

namespace Math {
#ifdef __GNUC__
  static unsigned int clz(uint32_t x) {
    return __builtin_clz(x);
  }
#else
  static unsigned int clz(uint32_t x) {
    x = ~x;
    uint32_t t = 0;
    while ((x >> (31 - t)) & 1u) t++;
    return t;
  }
#endif
}