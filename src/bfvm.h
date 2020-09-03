#pragma once

#include <string>

#include "memory.h"

namespace BFVM {
  struct Result {
    std::string output;
  };

  struct Config {
    Memory::Config memory;
    std::string dump;
    int cellWidth = 8;
    uint32_t eofValue = 0;
    int profile = 0;
    bool quiet = false;
  };

  void run(const std::string &code, const Config &config = {});
}