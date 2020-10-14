#pragma once

#include "ir.h"

namespace Opt {
  void resolveRegs(IR::Graph &graph);
  void validate(IR::Graph &graph);

  namespace Fold {
    struct FoldRules;
  }
  Fold::FoldRules *defaultFoldRules();
  void fold(IR::Graph &graph, Fold::FoldRules &rules);

  IR::TypeId resolveType(IR::Inst *inst);

  bool equal(IR::Inst *a, IR::Inst *b);

  void frame(IR::Graph &graph);
}