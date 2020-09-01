#pragma once

#include <string>

namespace BFVM {
  struct Config {
    bool enableArtifacts = false;
    std::string artifactsDir = "bfvm_out";
    int cellSize = 1;
    uint32_t eofValue = 0;
  };

  void run(const std::string &code, const Config &config = {});
}