#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <variant>

namespace BF {
  struct SeekLoop;
  struct Seek {
    int offset = 0;
    std::vector<SeekLoop> loops;

    [[nodiscard]] bool equals(const Seek &other) const;
    [[nodiscard]] std::string print() const;
  };

  typedef uint32_t DefIndex;

  struct Def {
    DefIndex index;
    std::vector<std::variant<Def, Seek>> body;

    explicit Def(DefIndex index);

    Def& pushDef(DefIndex index);
    Seek& pushSeek();

    [[nodiscard]] std::string print() const;
  };

  struct SeekLoop {
    Seek seek;
    int offset = 0;
  };

  enum Inst {
    I_ADD,
    I_SUB,
    I_DEF,
    I_SEEK,
    I_LOOP,
    I_END,
    I_PUTCHAR,
    I_GETCHAR,
  };

  struct Program {
    DefIndex nextDef = 0;

    std::vector<Def> defs;
    std::vector<DefIndex> seeks;
    std::vector<Inst> block;

    static Program parse(const std::string &str);
    [[nodiscard]] std::string print() const;
  };

  std::string printDefIndex(DefIndex index);
}