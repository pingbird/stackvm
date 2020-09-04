#include "fold.h"

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
}

void Opt::fold(FoldRules *rules, Inst *inst) {
  switch (inst->kind) {
    case I_NOP:
    case I_IMM:
    case I_LD:
    case I_STR:
    case I_REG:
    case I_SETREG:
    case I_GETCHAR:
    case I_PUTCHAR:
    case I_PHI:
    case I_IF:
    case I_GOTO:
    case I_RET:
    case I_ADD:
    case I_SUB:
    case I_GEP:
    case I_TAG:
      break;
  }

  Inst *left = inst->inputs[0];
  Inst *right = inst->inputs[1];

  FoldFn folder = nullptr;
}
