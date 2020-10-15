#pragma once

#include <unordered_map>
#include "opt.h"

namespace Opt::Fold {
  typedef uint64_t FoldKey;

  static FoldKey foldKey(IR::InstKind ins, IR::InstKind l = IR::I_NOP, IR::InstKind r = IR::I_NOP) {
    return (ins << 16u) | (l << 8u) | r;
  }

  enum FoldResultKind {
    FR_NONE,
    FR_LEFT,
    FR_RIGHT,
    FR_RETRY,
    FR_DONE,
  };

  struct FoldResult {
    IR::Inst *inst;
  };

  struct FoldState;
  typedef void (*FoldFn)(FoldState &state);

  struct FoldRules {
    std::unordered_map<FoldKey, FoldFn> folders;
  };

  struct FoldState {
    FoldState(IR::Graph &graph, FoldRules *rules) : graph(graph), rules(rules) {}
    IR::Inst *inst = nullptr;
    IR::Inst *left = nullptr;
    IR::Inst *right = nullptr;
    FoldResultKind result = FR_NONE;
    IR::Graph &graph;
    FoldRules *rules;
  };
}