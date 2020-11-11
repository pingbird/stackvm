#include <string>

#include "ir_print.h"

using namespace IR;

bool impureBetween(Inst *from, Inst *to) {
  if (from == nullptr) return true;
  Inst *cur = from;
  while (cur != nullptr) {
    if (cur == to) return false;
    if (!instIsPure(cur->kind)) return true;
    cur = cur->next;
  }
  return true;
}

bool validRange(Inst &inst, Inst *from, Inst *&upper) {
  if (inst.inputs.empty()) {
    if (!instIsPure(inst.kind)) {
      upper = &inst;
    }
    return true;
  }

  Inst *last = nullptr;
  int startIndex = 0;
  if (from != nullptr) {
    for (Inst *input : inst.inputs) {
      if (input == from) break;
      startIndex++;
    }
  }

  for (int i = startIndex; i < inst.inputs.size(); i++) {
    Inst *input = inst.inputs[i];
    if (last == nullptr) {
      last = input;
      continue;
    }
    Inst *inUpper = nullptr;
    if (input->block == inst.block) {
      if (!validRange(*input, nullptr, inUpper)) return false;
      if (inUpper != nullptr) {
        if (impureBetween(last->next, inUpper)) return false;
        if (upper == nullptr) {
          upper = inUpper;
        }
      }
      last = input;
    }
  }

  return last == nullptr || last->next == &inst || !impureBetween(last->next, &inst);
}

static bool shouldInline(Inst &inst, Inst &from) {
#ifndef NDEBUG
  if (!inst.comment.empty()) return false;
#endif
  if (inst.kind == I_IMM) return true;
  if (inst.kind == I_PHI) return false;
  if (inst.outputs.size() > 1) return false;
  if (inst.block != from.block) return false;
  if (!instIsPure(inst.kind)) return false;
  Inst *upper = nullptr;
  return validRange(from, &inst, upper);
}

static bool shouldOmit(Inst &inst) {
#ifndef NDEBUG
  if (!inst.comment.empty()) return false;
#endif
  if (!instIsPure(inst.kind)) return false;
  for (Inst *output : inst.outputs) {
    if (!shouldInline(inst, *output)) return false;
  }
  return true;
}

struct Precedence {
  int lhs;
  int rhs;
};

static const int defaultPrecedence = 4;

static Precedence instPrecedence(InstKind kind) {
  switch (kind) {
    case I_ADD:
    case I_SUB:
    case I_GEP:
      return {2, 3};
    case I_SETREG:
    case I_STR:
      return {1, 1};
    case I_LD:
    case I_RET:
      return {0, defaultPrecedence};
    default: return {defaultPrecedence, defaultPrecedence};
  }
}

struct SubCtx {
  Inst *inst;
  int precedence;
};

static std::string printSubInst(Inst &inst, SubCtx ctx) {
  if (!shouldInline(inst, *ctx.inst)) return "%" + std::to_string(inst.id);
  if (instPrecedence(inst.kind).rhs < ctx.precedence) {
    return "(" + printInst(inst) + ")";
  } else {
    return printInst(inst);
  }
}

static std::string inputStr(SubCtx ctx, int index) {
  return printSubInst(*ctx.inst->inputs[index], ctx);
}

std::string IR::printGraph(Graph &graph) {
  std::string str;
  bool first = true;
  for (Block *block : graph.blocks) {
    if (first) {
      first = false;
    } else {
      str += "\n\n";
    }
    str += printBlock(*block);
  }
  return str;
}

std::string IR::printBlock(Block &block) {
  std::string str = ".";
  str += block.getLabel();
  str += ":\n";
  Inst *inst = block.first;
  for (;;) {
    if (inst == nullptr) break;
    if (shouldOmit(*inst)) {
      inst = inst->next;
      continue;
    }
    str += "  ";
    if (!inst->outputs.empty()) {
      str += "%";
      str += std::to_string(inst->id);
      str += " = ";
    }
    str += printInst(*inst);
    inst = inst->next;
    if (inst != nullptr) {
      str += '\n';
    }
  }
  return str;
}

static std::string printInstBody(Inst &inst) {
  Block *block = inst.block;
  auto precedence = instPrecedence(inst.kind);
  SubCtx ctx = {&inst, precedence.lhs};
  switch (inst.kind) {
    case I_NOP: return "nop";
    case I_IMM: return std::to_string(inst.immValue);
    case I_ADD: return inputStr(ctx, 0) + " + " + inputStr({&inst, precedence.rhs}, 1);
    case I_SUB: return inputStr(ctx, 0) + " - " + inputStr({&inst, precedence.rhs}, 1);
    case I_LD: return "[" + inputStr(ctx, 0) + "]";
    case I_STR: return "[" + inputStr(ctx, 0) + "] <- " + inputStr(ctx, 1);
    case I_REG: return regNames[inst.immReg];
    case I_SETREG: return std::string(regNames[inst.immReg]) + " <- " + inputStr(ctx, 0);
    case I_GETCHAR: return "getchar";
    case I_PUTCHAR: return "putchar " + inputStr(ctx, 0);
    case I_PHI: {
      std::string str = "phi ";
      for (int i = 0; i < inst.inputs.size(); i++) {
        if (i != 0) {
          str += ", ";
        }
        str += ".";
        str += block->predecessors[i]->getLabel();
        str += ": ";
        str += inputStr(ctx, i);
      }
      return str;
    } case I_IF: return
      "if " + inputStr(ctx, 0) +
      " then ." +
      block->successors[0]->getLabel() +
      " else ." +
      block->successors[1]->getLabel();
    case I_GOTO: return "goto ." + block->successors[0]->getLabel();
    case I_RET: return "return " + inputStr(ctx, 0);
    case I_GEP:
      auto r = inst.inputs[1];
      if (r->kind == I_IMM && r->immValue < 0) {
        return inputStr(ctx, 0) + " &- " + std::to_string(-r->immValue);
      }
      return inputStr(ctx, 0)
        + " &+ "
        + inputStr({&inst, precedence.rhs}, 1);
  }
  return "unreachable";
}

std::string IR::printInst(Inst &inst) {
  std::string out = printInstBody(inst);
#ifndef NDEBUG
  if (!inst.comment.empty()) {
    out += " ; " + inst.comment;
  }
#endif
  return out;
}