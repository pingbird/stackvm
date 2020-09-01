#pragma once

#include <string>

namespace BFVM {
  struct Config {
    bool enableArtifacts = false;
    std::string artifactsDir = "bfvm_out";
  };

  void run(const std::string &code, const Config &config = {});
}