#pragma once

#include "../../../AppData/Local/Packages/CanonicalGroupLimited.Ubuntu20.04onWindows_79rhkp1fndgsc/LocalState/rootfs/usr/include/c++/9/unordered_map"
#include "opt.h"

namespace Opt::Fold {
  typedef uint64_t FoldKey;

  FoldKey foldKey(IR::InstKind l, IR::InstKind r);

  enum FoldResultState {
    FR_NONE,
    FR_LEFT,
    FR_RIGHT,
    FR_RETRY,
    FR_DONE,
  };

  struct FoldResult {
    FoldResultState state;
    IR::Inst *inst;
  };

  struct FoldState;
  typedef FoldResult (*FoldFn)(FoldState *state);

  struct FoldRules {
    std::unordered_map<FoldKey, FoldFn> folders;
  };

  struct FoldState {
    IR::Inst *inst;
    IR::Graph *graph;
  };
}