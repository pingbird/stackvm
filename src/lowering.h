#pragma once

#include <cassert>

#include "ir.h"
#include "bf.h"

namespace Lowering {
  IR::Graph *buildProgram(const std::vector<BF::Inst> *program);
}