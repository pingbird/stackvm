#include <map>
#include <cassert>
#include "bf.h"

using namespace BF;

bool Seek::equals(const Seek &other) const {
  if (
    other.offset != offset ||
    other.loops.size() != loops.size()
  ) {
    return false;
  }

  for (int i = 0; i < loops.size(); i++) {
    const SeekLoop &loop = loops[i];
    const SeekLoop &otherLoop = other.loops[i];
    if (loop.offset != otherLoop.offset || !loop.seek.equals(otherLoop.seek)) {
      return false;
    }
  }
  return true;
}

struct LoopInfo {
  bool pure = true;
};

struct Parser {
  Program &program;
  const std::string &str;

  Parser(Program &program, const std::string &str) : program(program), str(str) {}

  size_t pos = 0;
  size_t loopIndex = 1;

  std::vector<LoopInfo> loopCache;

  size_t scan() {
    size_t loop = loopCache.size();
    loopCache.emplace_back();
    for (;;) {
      switch (str[pos]) {
        case ',':
        case '.':
        case '+':
        case '-':
          loopCache[loop].pure = false;
          break;
        case '[': {
          pos++;
          auto child = scan();
          loopCache[loop].pure = loopCache[loop].pure && loopCache[child].pure;
          if (str[pos]) {
            assert(str[pos] == ']');
            break;
          }
          return loop;
        } case ']':
        case 0:
          return loop;
        default:
          break;
      }
      pos++;
    }
  }

  void parseSeek(Seek &seek) {
    for (;;) {
      switch (str[pos]) {
        case '<':
          if (seek.loops.empty()) {
            seek.offset--;
          } else {
            seek.loops[seek.loops.size() - 1].offset--;
          }
          break;
        case '>':
          if (seek.loops.empty()) {
            seek.offset++;
          } else {
            seek.loops[seek.loops.size() - 1].offset++;
          }
          break;
        case '[':
          if (!loopCache[loopIndex].pure) {
            return;
          }
          pos++;
          loopIndex++;
          parseSeek(seek.loops.emplace_back().seek);
          if (str[pos]) {
            assert(str[pos] == ']');
            pos++;
          } else {
            program.block.push_back(I_END);
          }
          continue;
        case 0:
        case ']':
        case '+':
        case '-':
        case '.':
        case ',':
          return;
        default:
          break;
      }
      pos++;
    }
  }

  void parse() {
    for (;;) {
      switch (str[pos]) {
        case '+': program.block.push_back(I_ADD); break;
        case '-': program.block.push_back(I_SUB); break;
        case '[':
          if (!loopCache[loopIndex].pure) {
            loopIndex++;
            program.block.push_back(I_LOOP);
            break;
          }
          // Fall through
        case '<':
        case '>': {
          size_t startPos = pos;
          Def& def = program.defs.emplace_back(program.nextDef++);
          parseSeek(def.pushSeek());
          program.block.push_back(I_DEF);
          program.seeks.push_back(def.index);
          program.block.push_back(I_SEEK);
          assert(pos != startPos);
          continue;
        } case ']': program.block.push_back(I_END); break;
        case '.': program.block.push_back(I_PUTCHAR); break;
        case ',': program.block.push_back(I_GETCHAR); break;
        case 0: return;
        default: break;
      }
      pos++;
    }
  }
};

Program Program::parse(const std::string &str) {
  Program program;
  Parser parser(program, str);
  parser.scan();
  parser.pos = 0;
  parser.parse();
  return program;
}

std::string Program::print() const {
  std::string str;
  int seekIndex = 0;
  int defIndex = 0;
  for (Inst inst : block) {
    switch (inst) {
      case I_ADD: str.push_back('+'); break;
      case I_SUB: str.push_back('-'); break;
      case I_LOOP: str.push_back('['); break;
      case I_END: str.push_back(']'); break;
      case I_PUTCHAR: str.push_back('.'); break;
      case I_GETCHAR: str.push_back(','); break;
      case I_DEF:
        str += '{';
        str += defs[defIndex++].print();
        str += '}';
        break;
      case I_SEEK:
        str += printDefIndex(seeks[seekIndex++]);
        break;
    }
  }
  return str;
}

void printSeekOffset(std::string &out, int offset) {
  if (offset > 0) {
    for (int i = 0; i < offset; i++) {
      out += '>';
    }
  } else {
    for (int i = 0; i < -offset; i++) {
      out += '<';
    }
  }
}

std::string Seek::print() const {
  std::string out;
  printSeekOffset(out, offset);
  for (auto &loop : loops) {
    out += '[';
    out.append(loop.seek.print());
    out += ']';
    printSeekOffset(out, loop.offset);
  }
  return out;
}

std::string Def::print() const {
  std::string out;
  for (int i = 0; i < body.size(); i++) {
    if (auto def = std::get_if<Def>(&body[i])) {
      if (i == body.size() - 1) {
        out += def->print();
      } else {
        out += '{';
        out += def->print();
        out += '}';
      }
    } else if (auto seek = std::get_if<Seek>(&body[i])) {
      out += seek->print();
    }
  }
  out += printDefIndex(index);
  return out;
}

Def::Def(DefIndex index) : index(index) {}

Def &Def::pushDef(DefIndex subIndex) {
  return std::get<0>(body.emplace_back(Def(subIndex)));
}

Seek &Def::pushSeek() {
  return std::get<1>(body.emplace_back(Seek()));
}

std::string BF::printDefIndex(DefIndex index) {
  if (index < 27) {
    return std::string(1, index + 'a');
  } else {
    return std::string(1, index + 'a') + "_" + std::to_string(index / 27);
  }
}