#pragma once

#include <vector>
#include <string>

namespace BF {
  enum Inst {
    I_ADD,
    I_SUB,
    I_LEFT,
    I_RIGHT,
    I_LOOP,
    I_END,
    I_PUTCHAR,
    I_GETCHAR,
  };

  std::vector<Inst> parse(const std::string &str);
  std::string print(const std::vector<Inst> &block);
}