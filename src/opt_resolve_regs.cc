#include <map>
#include <functional>
#include <cassert>

#include "opt.h"

using namespace IR;

struct BlockState {
  Inst *states[NUM_REGS] = {nullptr};
  Inst *dep[NUM_REGS] = {nullptr};
};

void Opt::resolveRegs(Graph &graph) {
  Builder builder(graph);

  std::map<Block*, BlockState> states;

  for (Block *block : graph.blocks) {
    auto &state = states[block];
    Inst *inst = block->first;
    while (inst != nullptr) {
      Inst *next = inst->next;
      switch (inst->kind) {
        case I_SETREG:
          assert(inst->inputs.size() == 1);
          state.states[inst->immReg] = inst->inputs[0];
          inst->safeDestroy();
          break;
        case I_REG: {
          Inst *value = state.states[inst->immReg];
          if (value != nullptr) {
            inst->rewriteWith(value);
          } else {
            if (state.dep[inst->immReg] != nullptr) {
              inst->rewriteWith(state.dep[inst->immReg]);
            } else {
              state.dep[inst->immReg] = inst;
            }
          }
          break;
        }
        default:
          break;
      }
      inst = next;
    }
  }

  std::function<Inst*(Block*, RegKind)> resolve = [&](Block* block, RegKind reg) -> Inst* {
    auto &state = states[block];

    builder.setBlock(block, nullptr);

    Inst *value = state.dep[reg];
    state.dep[reg] = nullptr;

    if (value == nullptr) {
      value = builder.pushReg(reg);
    }

    Inst *phi = nullptr;

    if (!block->predecessors.empty()) {
      Inst *oldValue = value;
      builder.setBlock(block, nullptr);
      value = (phi = builder.pushPhi());
      oldValue->rewriteWith(value);
    }

    if (state.states[reg] == nullptr) {
      assert((unsigned int)value->kind <= I_RET);
      state.states[reg] = value;
    }

    if (phi) {
      for (Block *pred : block->predecessors) {
        auto &pstate = states[pred];
        Inst *resolved = pstate.states[reg];
        if (resolved == nullptr) {
          resolved = resolve(pred, reg);
          assert(resolved->id > 0);
        }
        phi->inputs.push_back(resolved);
        resolved->outputs.push_back(phi);
      }
    }

    return value;
  };

  for (Block *block : graph.blocks) {
    auto &state = states[block];
    for (int reg = 0; reg < NUM_REGS; reg++) {
      if (state.dep[reg] != nullptr) {
        resolve(block, (RegKind)reg);
      }
    }
  }
}