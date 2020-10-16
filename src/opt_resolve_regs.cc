#include <map>
#include <functional>
#include <cassert>

#include "opt.h"

using namespace IR;

struct BlockState {
  // The value of the register after the block has completed
  Inst *states[NUM_REGS] = {nullptr};
};

// Search for immediate states in a block, skipping past frontiers
static Inst *findState(Block *&dominator, RegKind reg) {
  Inst *dominatorReg = nullptr;
  while (dominator->predecessors.size() == 1) {
    dominator = dominator->predecessors.at(0);
    if (dominator->passData) {
      auto dominatorState = (BlockState *)dominator->passData;
      dominatorReg = dominatorState->states[reg];
      if (dominatorReg != nullptr) {
        break;
      }
    }
  }
  return dominatorReg;
}

void Opt::resolveRegs(Graph &graph) {
  Builder builder(graph);
  graph.clearPassData();

  std::vector<Inst*> unresolved;

  // Collapse states and deps
  for (Block *block : graph.blocks) {
    Inst *inst = block->first;
    auto state = new BlockState();
    block->passData = state;
    while (inst != nullptr) {
      switch (inst->kind) {
        case I_REG: {
          RegKind reg = inst->immReg;
          if (state->states[reg] != nullptr) {
            // Rewrite register access with previous state
            inst->rewriteWith(state->states[reg]);
          } else {
            // Search our immediate dominators for previous states
            Block *dominator = block;
            Inst *dominatorReg = findState(dominator, reg);

            if (dominatorReg != nullptr) {
              state->states[reg] = dominatorReg;
              inst->rewriteWith(dominatorReg);
            } else {
              if (dominator != block) {
                // Shift instruction up to the dominator
                dominator->moveAfter(inst, nullptr);
              }
              // This is the first register access of this block, make it a dep
              state->states[reg] = inst;
              unresolved.push_back(inst);
            }
          }
          break;
        } case I_SETREG:
          state->states[inst->immReg] = inst->inputs.at(0);
          inst->destroy();
          break;
        default:
          break;
      }
      inst = inst->next;
    }
  }

  while (!unresolved.empty()) {
    Inst *cur = unresolved.back();
    unresolved.pop_back();
    Block *block = cur->block;
    auto state = (BlockState*)block->passData;
    std::vector<Block*> &predecessors = block->predecessors;
    if (predecessors.empty()) {
      // We hit an entry, its fine for unresolved registers to be here
      continue;
    } else {
      // Should not be a frontier
      assert(predecessors.size() >= 2);

      Opt::validate(graph);

      // Build a new phi node with state inputs from each predecessor
      builder.setAfter(cur);
      Inst *phi = builder.pushPhi();
      for (Block *predecessor : predecessors) {
        Block *dominator = predecessor;
        Inst *dominatorReg = findState(dominator, cur->immReg);

        if (dominatorReg == nullptr) {
          // Predecessor has no state, push reg instruction and enqueue it
          builder.setAfter(dominator);
          dominatorReg = builder.pushReg(cur->immReg);
          unresolved.push_back(dominatorReg);
          auto dominatorState = (BlockState*)dominator->passData;
          dominatorState->states[cur->immReg] = dominatorReg;
        }

        phi->addInput(dominatorReg);
      }

      // Rewrite unresolved register with new phi
      state->states[cur->immReg] = phi;
      cur->rewriteWith(phi);

      Opt::validate(graph);
    }
  }

  // Clean up BlockStates
  for (Block *block : graph.blocks) {
    delete (BlockState*)block->passData;
  }
}