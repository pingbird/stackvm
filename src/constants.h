#pragma once

#include <cstdint>

namespace Constants {
  const unsigned int wordBits = 8;
  const unsigned int wordMax = (1u << wordBits) - 1;
  typedef uint8_t Word;
}