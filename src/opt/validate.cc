#include <unordered_set>
#include <algorithm>
#include <cassert>

#include "../opt.h"

using namespace IR;

void Opt::validate(Graph *graph) {
  std::unordered_set<int> ids;
  for (Block *block : graph->blocks) {
    assert(block->id < graph->nextBlockId);
    std::vector<Inst*> insts;
    Inst *cur = block->first;
    while (cur != nullptr) {
      insts.push_back(cur);
      assert(!ids.count(cur->id));
      assert(cur->id < graph->nextInstId);
      ids.insert(cur->id);
      cur = cur->next;
    }
    assert(insts[insts.size() - 1] == block->last);
    cur = block->last;
    for (size_t i = insts.size(); i > 0; i--) {
      assert(cur == insts[i - 1]);
      cur = cur->prev;
    }
  }

  for (Block *block : graph->blocks) {
    Inst *cur = block->first;
    while (cur != nullptr) {
      for (Inst *input : cur->inputs) {
        assert(ids.count(input->id));
        auto &v = input->outputs;
        assert(std::find(v.begin(), v.end(), cur) != v.end());
      }
      for (Inst *output : cur->outputs) {
        assert(ids.count(output->id));
        auto &v = output->inputs;
        assert(std::find(v.begin(), v.end(), cur) != v.end());
      }
      cur = cur->next;
    }
  }
}