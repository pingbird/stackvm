#pragma once

#include <string>

#include "memory.h"

namespace BFVM {
  struct Config {
    Memory::Config memory;
    std::string dump;
    int cellWidth = 8;
    uint32_t eofValue = 0;
#ifndef NDIAG
    int profile = 0;
    bool quiet = false;
#endif
  };

  void run(const std::string &code, const Config &config = {});
}