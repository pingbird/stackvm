#include "opt.h"

using namespace IR;

uint64_t instKey(Inst *inst) {
  uint64_t kind = inst->kind;
  uint64_t input0 = inst->inputs.empty() ? 0 : inst->inputs[0]->id & 0xFFFFu;
  uint64_t input1 = inst->inputs.size() < 2 ? 0 : inst->inputs[1]->id & 0xFFFFu;
  uint64_t imm = 0;
  switch (inst->kind) {
    case I_IMM:
      imm = (uint64_t)inst->immValue & 0xFFFFu;
      break;
    case I_REG:
      imm = inst->immReg;
      break;
    default:
      break;
  }
  return (kind << 48u) | (input0 << 32u) | (input1 << 16u) | imm;
}

bool instEqual(Inst *a, Inst *b) {
  if (a->kind != b->kind || a->inputs.size() != b->inputs.size()) return false;

  Inst *aInput0 = a->inputs.empty() ? nullptr : a->inputs[0];
  Inst *bInput0 = b->inputs.empty() ? nullptr : b->inputs[0];
  if (aInput0 != bInput0) return false;

  Inst *aInput1 = a->inputs.size() < 2 ? nullptr : a->inputs[1];
  Inst *bInput1 = b->inputs.size() < 2 ? nullptr : b->inputs[1];
  if (aInput1 != bInput1) return false;

  return true;
}

struct CSEEngine {
  std::unordered_map<uint64_t, std::vector<Inst*>> cache;

  Inst *optimizeInst(Inst *inst) {
    uint64_t key = instKey(inst);
    auto &vec = cache[key];
    
    for (Inst *candidate : vec) {
      assert(inst != candidate);
      if (instEqual(inst, candidate)) {
        // Because we optimize predecessors first, this should not affect existing keys
        inst->rewriteWith(candidate);
        return candidate;
      }
    }

    vec.push_back(inst);
    return inst;
  }

  std::unordered_set<Block*> visitedBlocks;

  void optimizeBlock(Block *block) {
    if (visitedBlocks.contains(block)) {
      return;
    } else {
      visitedBlocks.insert(block);
    }

    Inst *inst = block->first;
    while (inst != nullptr) {
      Inst *next = inst->next;
      if (instIsPure(inst->kind)) {
        optimizeInst(inst);
      }
      inst = next;
    }
  }
};

void Opt::optimizeCommonExpr(Graph &graph) {
  CSEEngine cse;
  cse.optimizeBlock(graph.blocks[0]);

}