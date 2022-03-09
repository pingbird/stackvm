#pragma once

#include "ir.h"

namespace Opt {
  void resolveRegs(IR::Graph &graph);
  void validate(IR::Graph &graph);

  struct FoldKey {
    FoldKey(IR::InstKind ins, IR::InstKind l = IR::I_NOP, IR::InstKind r = IR::I_NOP)
      : value((ins << 16u) | (l << 8u) | r) {}
    uint64_t value;
  };

  struct FoldState;
  typedef IR::Inst *(*FoldFn)(FoldState &state);

  struct FoldRules {
    FoldRules() = default;

    std::unordered_map<uint64_t, FoldFn> folders = {};

    void add(std::initializer_list<FoldKey> keys, FoldFn fn) {
      for (auto key : keys) {
        assert(!folders.contains(key.value));
        folders[key.value] = fn;
      }
    }
  };

  struct FoldState {
    FoldState(IR::Graph &graph, const FoldRules &rules) : b(graph), rules(rules) {}
    IR::Inst *inst = nullptr;
    IR::Inst *left = nullptr;
    IR::Inst *right = nullptr;
    IR::Builder b;
    const FoldRules &rules;
  };

  FoldRules standardFoldRules();

  void fold(IR::Graph &graph, const FoldRules &rules);

  IR::TypeId resolveType(IR::Inst *inst);

  bool equal(IR::Inst *a, IR::Inst *b);

  void optimizeLoops(IR::Graph &graph);
  void optimizeCommonExpr(IR::Graph &graph);
}