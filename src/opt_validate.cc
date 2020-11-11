#include <unordered_set>
#include <algorithm>
#include <cassert>

#include "opt.h"

using namespace IR;

void Opt::validate(Graph &graph) {
#ifdef NDEBUG
  return;
#else
  std::unordered_set<int> instIds;

  for (Block *block : graph.blocks) {
    // Make sure our block id hasn't been clobbered
    assert(block->id < graph.nextBlockId);

    // Make sure our successors have higher block ids, except loops
    for (Block *successor : block->successors) {
      if (successor->id <= block->id) {
        // If the successor has a lower block id, make sure it's a back-branch
        assert(successor->reaches(block));
      }
    }

    // If builtDominators is true, the dominator tree should be valid
    if (graph.builtDominators) {
      // Dominator should always reach us
      assert(block->alwaysReachedBy(block->dominator));
    }

    // Build list of instructions
    std::vector<Inst*> insts;
    Inst *cur = block->first;
    while (cur != nullptr) {
      insts.push_back(cur);
      // Make sure instruction is in current block
      assert(cur->block == block);
      // Make sure there are no duplicates
      assert(!instIds.count(cur->id));
      instIds.insert(cur->id);
      // Make sure instruction id hasn't been clobbered
      assert(cur->id < graph.nextInstId);
      cur = cur->next;
    }

    // Make sure reverse list matches forward list
    cur = block->last;
    for (size_t i = insts.size(); i > 0; i--) {
      assert(cur == insts[i - 1]);
      cur = cur->prev;
    }
  }

  bool first = true;
  for (Block *block : graph.blocks) {
    // Make sure entry has no predecessor
    if (first) {
      assert(block->predecessors.empty());
      first = false;
    }

    // Make sure we are a successor of our predecessors
    for (Block *predecessor : block->predecessors) {
      auto &v = predecessor->successors;
      assert(std::find(v.begin(), v.end(), block) != v.end());
    }

    // Make sure we are a predecessor of our successors
    for (Block *successor : block->successors) {
      auto &v = successor->predecessors;
      assert(std::find(v.begin(), v.end(), block) != v.end());
    }

    std::unordered_set<Inst*> visited;

    Inst *cur = block->first;
    while (cur != nullptr) {
      assert(cur->mounted);

      switch (cur->kind) {
        case I_NOP:
          assert(cur->inputs.empty());
          assert(cur->outputs.empty());
          break;
        case I_REG:
        case I_IMM:
        case I_GETCHAR:
          assert(cur->inputs.empty());
          break;
        case I_GEP:
          assert(resolveType(cur->inputs[0]) == T_PTR);
          assert(resolveType(cur->inputs[1]) == T_SIZE);
          assert(cur->inputs.size() == 2);
          break;
        case I_SUB:
        case I_ADD:
        case I_STR:
          assert(cur->inputs.size() == 2);
          break;
        case I_SETREG:
        case I_LD:
        case I_PUTCHAR:
          assert(cur->inputs.size() == 1);
          break;
        case I_PHI:
          assert(cur->inputs.size() == block->predecessors.size());
          break;
        case I_IF:
          assert(cur->inputs.size() == 1);
          assert(cur->next == nullptr);
          break;
        case I_GOTO:
          assert(cur->inputs.empty());
          assert(cur->next == nullptr);
          assert(block->successors.size() == 1);
          break;
        case I_RET:
          assert(cur->inputs.size() == 1);
          assert(cur->next == nullptr);
          assert(block->successors.empty());
          break;
      }

      for (size_t i = 0; i < cur->inputs.size(); i++) {
        Inst *input = cur->inputs[i];
        assert(input != nullptr);

        // Make sure input exists in graph
        assert(instIds.count(input->id));
        assert(input->mounted);
        // Make sure we are an output of our inputs
        auto &v = input->outputs;
        assert(std::find(v.begin(), v.end(), cur) != v.end());

        if (cur->kind == I_PHI) {
          // Make sure phi input dominates predecessor
          assert(block->predecessors[i]->alwaysReachedBy(input->block));
        } else {
          if (block == input->block) {
            // Make sure input is before its use in same block
            assert(visited.count(input));
          } else {
            // Make sure input dominates uses
            assert(block->alwaysReachedBy(input->block));
          }
        }
      }

      for (Inst *output : cur->outputs) {
        // Make sure output exists in graph
        assert(instIds.count(output->id));
        // Make sure we are an input of our outputs
        auto &v = output->inputs;
        assert(std::find(v.begin(), v.end(), cur) != v.end());
      }

      visited.insert(cur);
      cur = cur->next;
    }
  }
#endif
}