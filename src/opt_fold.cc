#include "opt.h"

using namespace IR;
using namespace Opt;

bool Opt::equal(IR::Inst *a, IR::Inst *b) {
  if (a == b) return true;
  if (a == nullptr) return false;
  if (b == nullptr) return false;
  if (a->kind != b->kind) return false;
  switch (a->kind) {
    case I_NOP:
      return true;
    case I_IMM:
      return a->immValue == b->immValue;
    case I_ADD:
    case I_SUB:
    case I_GEP:
    case I_LD:
      for (int i = 0; i < a->inputs.size(); i++) {
        if (!equal(a->inputs[i], b->inputs[i])) {
          return false;
        }
      }
      return true;
    case I_REG:
    case I_PHI:
    case I_SETREG:
    case I_GETCHAR:
      return false;
    case I_IF:
    case I_PUTCHAR:
    case I_GOTO:
    case I_RET:
    case I_STR:
      abort();
  }
  abort();
}

void foldInst(FoldState &state) {
  Inst *inst = state.inst;
  size_t inputCount = inst->inputs.size();
  Inst *left = inputCount < 1 ? nullptr : inst->inputs[0];
  Inst *right = inputCount < 2 ? nullptr : inst->inputs[1];
  state.left = left;
  state.right = right;
  InstKind leftKind = inputCount < 1 ? I_NOP : left->kind;
  InstKind rightKind = inputCount < 2 ? I_NOP : right->kind;
  FoldKey key = {inst->kind, leftKind, rightKind};
  uint64_t lastKey = 0;

  for (unsigned int i = 0; i < 4; i++) {
    uint64_t maskedKey = key.value;
    if ((i & 1u) != 0) {
      maskedKey &= ~0xFFu;
    } else if ((i & 10u) != 0) {
      maskedKey &= ~0xFF00u;
    }
    if (maskedKey == lastKey) continue;
    lastKey = maskedKey;
    if (state.rules.folders.count(maskedKey)) {
      state.b.setBefore(inst);
      Inst *result = state.rules.folders.at(maskedKey)(state);

      if (result == nullptr) {
        // No match, continue
        continue;
      } else if (result == inst) {
        // Instruction was modified in-place, retry fold
        return foldInst(state);
      } else {
        // Done, rewrite
        state.inst->rewriteWith(result);
        return;
      }
    }
  }
}

void Opt::fold(Graph &graph, const FoldRules &rules) {
  FoldState state(graph, rules);
  for (Block *block : graph.blocks) {
    state.inst = block->first;
    while (state.inst != nullptr) {
      Inst *next = state.inst->next;
      foldInst(state);
      state.inst = next;
    }
  }
}

FoldRules Opt::standardFoldRules() {
  FoldRules rules;

  // Optimize GEP

  rules.add({{I_GEP, I_GEP, I_IMM}}, [](FoldState &s) -> Inst* {
    Inst *leftOperand = s.left->inputs[1];
    if (leftOperand->kind == I_IMM) {
      // i[imm x][imm y] -> i[x + y]
      s.inst->replaceInput(0, s.left->inputs[0]);
      s.inst->replaceInput(1, s.b.pushImm(s.right->immValue + leftOperand->immValue, T_SIZE));
      return s.inst;
    }
    return nullptr;
  });

  rules.add({{I_GEP, I_NOP, I_IMM}}, [](FoldState &s) -> Inst* {
    if (s.right->immValue == 0) {
      // i[0] -> i
      return s.left;
    }
    return nullptr;
  });

  return rules;
}