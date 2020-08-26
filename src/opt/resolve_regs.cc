#include <map>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <iostream>
#include <cassert>

#include "../opt.h"

using namespace IR;

struct BlockState {
  Inst *states[NUM_REGS] = {nullptr};
  Inst *dep = nullptr;
};

void Opt::resolveRegs(Graph *graph) {
  Builder builder(graph);

  std::map<Block*, BlockState> states;

  for (Block *block : graph->blocks) {
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
            Opt::validate(graph);
            inst->rewriteWith(value);
            Opt::validate(graph);
          } else {
            if (state.dep != nullptr) {
              Opt::validate(graph);
              inst->rewriteWith(state.dep);
              Opt::validate(graph);
            } else {
              state.dep = inst;
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

    Inst *value = state.dep;
    state.dep = nullptr;

    if (value == nullptr) {
      value = builder.pushReg(reg);
    }

    if (!block->predecessors.empty()) {
      std::vector<Inst *> inputs;

      for (Block *pred : block->predecessors) {
        auto &pstate = states[pred];
        Inst *resolved = pstate.states[reg];
        if (resolved == nullptr) {
          resolved = resolve(pred, reg);
          assert(resolved->id > 0);
        }
        inputs.push_back(resolved);
      }

      Inst *oldValue = value;
      if (inputs.size() == 1) {
        value = inputs[0];
      } else {
        builder.setBlock(block, nullptr);
        value = builder.pushPhi(&inputs);
      }
      oldValue->rewriteWith(value);
    }

    if (state.states[reg] == nullptr) {
      assert((unsigned int)value->kind <= I_RET);
      state.states[reg] = value;
    }

    return value;
  };

  for (Block *block : graph->blocks) {
    auto &state = states[block];
    for (int reg = 0; reg < NUM_REGS; reg++) {
      if (state.dep != nullptr) {
        resolve(block, (RegKind)reg);
      }
    }
  }
}