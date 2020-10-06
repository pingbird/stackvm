#pragma once

#include <vector>
#include <string>
#include <unordered_set>

namespace BF {
  struct SeekLoop;
  struct Seek {
    int offset = 0;
    std::vector<SeekLoop> loops;
    [[nodiscard]] bool equals(const Seek &other) const;
  };

  struct SeekLoop {
    Seek seek;
    int offset = 0;
  };

  enum Inst {
    I_ADD,
    I_SUB,
    I_SEEK,
    I_LOOP,
    I_END,
    I_PUTCHAR,
    I_GETCHAR,
  };

  struct Program {
    std::vector<Seek> seeks;
    std::vector<Inst> block;
  };

  Program parse(const std::string &str);
  std::string print(const Program &block);
  std::string printSeek(const Seek &seek);
}