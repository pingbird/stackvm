#pragma once

#include <string>

namespace BFVM {
  struct Config {
    std::string dump;
    int cellWidth = 8;
    uint32_t eofValue = 0;
    bool profile = false;
  };

  void run(const std::string &code, const Config &config = {});
}