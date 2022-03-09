#include <map>
#include <functional>
#include <cassert>

#include "opt.h"

using namespace IR;

struct BlockState {
  // The value of the register after the block has completed
  Inst *states[NUM_REGS] = {nullptr};
};

static void validateBlocks(Graph &graph) {
#ifndef NDEBUG
  for (Block *block : graph.blocks) {
    if (block->passData) {
      auto dominatorState = (BlockState*)block->passData;
      for (int i = 0; i < 2; i++) {
        Inst *reg = dominatorState->states[i];
        if (reg) {
          assert(reg->mounted);
          assert(reg->block->dominates(block));
        }
      }
    }
  }
#endif
}

// Search for immediate states in a block, skipping past frontiers
static Inst *findState(Block *&dominator, RegKind reg) {
  Inst *dominatorReg = nullptr;
  while (dominator->predecessors.size() == 1) {
    dominator = dominator->predecessors.at(0);
    if (dominator->passData != nullptr) {
      auto dominatorState = (BlockState*)dominator->passData;
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
      Inst *next = inst->next;
      validateBlocks(graph);
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
              inst->rewriteWith(dominatorReg);
            } else {
              if (dominator != block) {
                // Shift instruction up to the dominator
                dominator->moveAfter(inst, nullptr);
                inst->setComment("shifted");
              }
              // This is the first register access of this block
              auto dominatorState = (BlockState*)dominator->passData;
              assert(dominatorState->states[reg] == nullptr);
              dominatorState->states[reg] = inst;
              assert(inst->block->dominates(dominator));
              unresolved.push_back(inst);
            }
          }
          break;
        } case I_SETREG:
          state->states[inst->immReg] = inst->inputs.at(0);
          assert(state->states[inst->immReg]->block->dominates(inst->block));
          inst->destroy();
          break;
        default:
          break;
      }
      validateBlocks(graph);
      inst = next;
    }
  }

  while (!unresolved.empty()) {
    Inst *cur = unresolved.back();
    unresolved.pop_back();
    assert(cur->kind == I_REG);
    RegKind reg = cur->immReg;
    Block *block = cur->block;
    auto state = (BlockState*)block->passData;
    std::vector<Block*> &predecessors = block->predecessors;
    validateBlocks(graph);
    if (predecessors.empty()) {
      // We hit an entry, its fine for unresolved registers to be here
      continue;
    } else {
      validateBlocks(graph);
      // Should not be a frontier
      assert(predecessors.size() >= 2);

      // Build a new phi node with state inputs from each predecessor
      builder.setAfter(cur);
      validateBlocks(graph);
      Inst *phi = builder.pushPhi();
      validateBlocks(graph);
      for (Block *predecessor : predecessors) {
        validateBlocks(graph);
        Block *dominator = predecessor;
        auto dominatorState = (BlockState*)dominator->passData;
        Inst *dominatorReg;
        assert(dominator->dominates(predecessor));
        validateBlocks(graph);

        if (dominatorState->states[reg] == nullptr) {
          dominatorReg = findState(dominator, cur->immReg);
          // assert(dominatorReg == nullptr || dominatorReg->block->dominates(predecessor));
          if (dominatorReg == nullptr) {
            // Predecessor has no state, push reg instruction and enqueue it
            builder.setAfter(dominator);
            dominatorReg = builder.pushReg(cur->immReg);
            dominatorReg->setComment("pushed");
            unresolved.push_back(dominatorReg);
            dominatorState = (BlockState*)dominator->passData;
            dominatorState->states[cur->immReg] = dominatorReg;
            assert(dominatorReg->block->dominates(predecessor));
            validateBlocks(graph);
          }
        } else {
          dominatorReg = dominatorState->states[reg];
          assert(dominatorReg->block->dominates(predecessor));
          validateBlocks(graph);
        }

        phi->addInput(dominatorReg);
      }
      validateBlocks(graph);

      // Rewrite unresolved register with new phi
      state->states[cur->immReg] = phi;
      cur->rewriteWith(phi);
      // TODO: make this efficient, it has quadratic complexity
      for (Block *curBlock : graph.blocks) {
        auto curState = (BlockState*)curBlock->passData;
        for (Inst *&inst : curState->states) {
          if (inst == cur) {
            inst = phi;
          }
        }
      }
      validateBlocks(graph);
    }
    validateBlocks(graph);
  }

  // Clean up BlockStates
  for (Block *block : graph.blocks) {
    delete (BlockState*)block->passData;
    block->passData = nullptr;
  }
}