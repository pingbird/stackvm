#include "fold.h"

using namespace IR;
using namespace Opt;
using namespace Opt::Fold;

FoldRules *Opt::defaultFoldRules() {
  auto rules = new FoldRules();

  return rules;
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
      return;
    case I_ADD:
    case I_SUB:
      break;
  }

  Inst *left = inst->inputs[0];
  Inst *right = inst->inputs[1];

  FoldFn folder = nullptr;
}
