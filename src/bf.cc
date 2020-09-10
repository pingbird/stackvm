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

Program BF::parse(const std::string &str) {
  Program program;
  for (const char *c = str.c_str(); *c;) {
    switch (*c) {
      case '+': program.block.push_back(I_ADD); break;
      case '-': program.block.push_back(I_SUB); break;
      case '<': {
        program.block.push_back(I_SEEK);
        int offset = 1;
        c++;
        while (*c == '<') {
          offset++;
          c++;
        }
        program.seeks.emplace_back();
        auto &seek = program.seeks[program.seeks.size() - 1];
        seek.offset = -offset;
        continue;
      } case '>': {
        program.block.push_back(I_SEEK);
        int offset = 1;
        c++;
        while (*c == '>') {
          offset++;
          c++;
        }
        program.seeks.emplace_back();
        auto &seek = program.seeks[program.seeks.size() - 1];
        seek.offset = offset;
        continue;
      } case '[': program.block.push_back(I_LOOP); break;
      case ']': program.block.push_back(I_END); break;
      case '.': program.block.push_back(I_PUTCHAR); break;
      case ',': program.block.push_back(I_GETCHAR); break;
      default: break;
    }
    c++;
  }
  return program;
}

std::string BF::print(const Program &program) {
  std::string str;
  int seekIndex = 0;
  for (char inst : program.block) {
    switch (inst) {
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
    printSeekOffset(out, seek.offset);
  }
  return out;
}