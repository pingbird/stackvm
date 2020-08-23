#pragma once

#include "ir.h"

namespace Opt {
  void resolveRegs(IR::Graph *graph);
  void validate(IR::Graph *graph);
  void fold(IR::Graph *graph);
}