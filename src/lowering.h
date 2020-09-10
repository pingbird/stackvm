#pragma once

#include <cassert>
#include <memory>

#include "ir.h"
#include "bf.h"

namespace Lowering {
  std::unique_ptr<IR::Graph> buildProgram(const BFVM::Config &config, const BF::Program &program);
}