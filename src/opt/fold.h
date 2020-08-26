#pragma once

#include "../opt.h"

namespace Opt {
  typedef uint64_t FoldKey;

  struct FoldState {
    IR::Inst *inst;
    IR::Graph *graph;
  };
}