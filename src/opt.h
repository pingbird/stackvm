#pragma once

#include "ir.h"

namespace Opt {
  void resolveRegs(IR::Graph *graph);
  void validate(IR::Graph *graph);

  namespace Fold {
    struct FoldRules;
  }
  Fold::FoldRules *defaultFoldRules();
  void fold(Fold::FoldRules *rules, IR::Inst *inst);

  IR::TypeId resolveType(IR::Inst *inst);
}