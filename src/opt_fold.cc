#include "fold.h"
#include "../opt.h"


using namespace IR;
using namespace Opt;
using namespace Opt::Fold;

FoldRules *Opt::defaultFoldRules() {
  auto rules = new FoldRules();

  return rules;
}

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
    case I_TAG:
    case I_STR:
      abort();
  }
  abort();
}

void foldInst(FoldState &state, Inst *inst) {
  size_t inputCount = inst->inputs.size();
  Inst *left = inputCount < 1 ? nullptr : inst->inputs[0];
  Inst *right = inputCount < 2 ? nullptr : inst->inputs[1];
  InstKind leftKind = left == nullptr ? I_NOP : left->kind;
  InstKind rightKind = right == nullptr ? I_NOP : right->kind;
  FoldKey key = foldKey(inst->kind, leftKind, rightKind);
  FoldKey lastKey = 0;
  for (unsigned int i = 0; i < 4; i++) {
    FoldKey maskedKey = key;
    if ((i & 1u) != 0) {
      maskedKey &= ~0xFFu;
    } else if ((i & 10u) != 0) {
      maskedKey &= ~0xFF00u;
    }
    if (maskedKey == lastKey) continue;
    lastKey = maskedKey;
    if (state.rules->folders.count(maskedKey)) {
      state.result = FR_NONE;
      state.rules->folders[maskedKey](state);
      break;
    }
  }
}

void fold(Graph &graph, FoldRules *rules) {
  FoldState state(graph, rules);
  for (Block *b : graph.blocks) {
    
  }
}
