#include "bf.h"

using namespace BF;

std::vector<Inst> BF::parse(const std::string &str) {
  std::vector<Inst> block;
  for (char c : str) {
    switch (c) {
      case '+': block.push_back(I_ADD); break;
      case '-': block.push_back(I_SUB); break;
      case '<': block.push_back(I_LEFT); break;
      case '>': block.push_back(I_RIGHT); break;
      case '[': block.push_back(I_LOOP); break;
      case ']': block.push_back(I_END); break;
      case '.': block.push_back(I_PUTCHAR); break;
      case ',': block.push_back(I_GETCHAR); break;
      default: break;
    }
  }
  return block;
}

std::string BF::print(const std::vector<Inst>& block) {
  std::string str;
  for (char inst : block) {
    switch (inst) {
      case I_ADD: str.push_back('+'); break;
      case I_SUB: str.push_back('-'); break;
      case I_LEFT: str.push_back('<'); break;
      case I_RIGHT: str.push_back('>'); break;
      case I_LOOP: str.push_back('['); break;
      case I_END: str.push_back(']'); break;
      case I_PUTCHAR: str.push_back('.'); break;
      case I_GETCHAR: str.push_back(','); break;
      default: break;
    }
  }
  return str;
}