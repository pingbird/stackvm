#include <unordered_set>
#include <algorithm>
#include <cassert>

#include "../opt.h"

using namespace IR;

void Opt::validate(Graph *graph) {
#ifdef NDEBUG
  return;
#else
  std::unordered_set<int> instIds;

  for (Block *block : graph->blocks) {
    assert(block->id < graph->nextBlockId);
    std::vector<Inst*> insts;
    Inst *cur = block->first;
    while (cur != nullptr) {
      insts.push_back(cur);
      assert(!instIds.count(cur->id));
      assert(cur->id < graph->nextInstId);
      instIds.insert(cur->id);
      cur = cur->next;
    }
    assert(insts[insts.size() - 1] == block->last);
    cur = block->last;
    for (size_t i = insts.size(); i > 0; i--) {
      assert(cur == insts[i - 1]);
      cur = cur->prev;
    }
  }

  bool first = true;
  for (Block *block : graph->blocks) {
    if (first) {
      assert(block->predecessors.empty());
      first = false;
    }

    for (Block *predecessor : block->predecessors) {
      auto &v = predecessor->successors;
      assert(std::find(v.begin(), v.end(), block) != v.end());
    }

    for (Block *successor : block->successors) {
      auto &v = successor->predecessors;
      assert(std::find(v.begin(), v.end(), block) != v.end());
    }

    Inst *cur = block->first;
    while (cur != nullptr) {
      for (Inst *input : cur->inputs) {
        assert(instIds.count(input->id));
        auto &v = input->outputs;
        assert(std::find(v.begin(), v.end(), cur) != v.end());
      }
      for (Inst *output : cur->outputs) {
        assert(instIds.count(output->id));
        auto &v = output->inputs;
        assert(std::find(v.begin(), v.end(), cur) != v.end());
      }
      cur = cur->next;
    }
  }
#endif
}