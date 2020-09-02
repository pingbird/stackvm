#pragma once

#include <string>

#include "memory.h"

namespace BFVM {
  struct Config {
    Memory::Config memory;
    std::string dump;
    int cellWidth = 8;
    uint32_t eofValue = 0;
    bool profile = false;
  };

  void run(const std::string &code, const Config &config = {});
}