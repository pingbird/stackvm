#include <map>
#include <cassert>
#include <memory>
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

LoopBalance SeekData::getBalance() const {
  uint8_t balance = LB_BALANCED;

  int totalOffset = offset;

  for (const SeekLoop &loop : loops) {
    balance |= loop.seek.getBalance();
    totalOffset += loop.offset;
  }

  if (totalOffset > 0) {
    balance |= LB_RIGHT;
  } else if (totalOffset < 0) {
    balance |= LB_LEFT;
  }

  return balance;
}

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
          // Fall through
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
          parseSeek(program.seeks.emplace_back());
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

Program BF::parse(const std::string &str) {
  Program program;
  Parser parser(program, str);
  parser.scan();
  parser.pos = 0;
  parser.parse();
  return program;
}

std::string BF::print(const Program &program) {
  std::string str;
  int seekIndex = 0;
  for (auto inst : program.instructions) {
    switch (inst.kind) {
      case I_ADD: str.push_back('+'); break;
      case I_SUB: str.push_back('-'); break;
      case I_SEEK: str.append(printSeek(program.seeks[seekIndex++])); break;
      case I_LOOP: str.push_back('['); break;
      case I_END: str.push_back(']'); break;
      case I_PUTCHAR: str.push_back('.'); break;
      case I_GETCHAR: str.push_back(','); break;
      default: break;
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

std::string BF::printSeek(const Seek &seek) {
  std::string out;
  printSeekOffset(out, seek.offset);
  for (auto &loop : seek.loops) {
    out += '[';
    out.append(printSeek(loop.seek));
    out += ']';
    printSeekOffset(out, loop.offset);
  }
  return out;
}