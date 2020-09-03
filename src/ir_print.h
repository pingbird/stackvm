#pragma once

#include "ir.h"

namespace IR {
  std::string printGraph(Graph &graph);
  std::string printBlock(Block &block);
  std::string printInst(Inst &inst);
}