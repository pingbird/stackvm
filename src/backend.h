#pragma once

#include "ir.h"

namespace Backend::LLVM {
  void compile(IR::Graph *graph);
}