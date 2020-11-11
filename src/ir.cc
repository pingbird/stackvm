#include "ir.h"

#include <cassert>
#include <string>
#include <algorithm>

using namespace IR;

std::string IR::typeString(TypeId type) {
  switch (type) {
    case T_INVALID: return "Invalid";
    case T_NONE: return "None";
    case T_PTR: return "Ptr";
    case T_SIZE: return "Size";
    case T_I64: return "I64";
    case T_I32: return "I32";
    case T_I16: return "I16";
    case T_I8: return "I8";
    default:
      return "User" + std::to_string(type - T_USER_START);
  }
}

const char *IR::regNames[2] = {
  "ptr",
  "def",
};

void Inst::detach() {
  assert(mounted);
  assert(block != nullptr);
  mounted = false;
  if (prev == nullptr) {
    block->first = next;
    if (next == nullptr) {
      assert(block->open);
      block->last = nullptr;
    } else {
      next->prev = nullptr;
    }
  } else if (next == nullptr) {
    assert(block->open);
    block->last = prev;
    prev->next = nullptr;
  } else {
    prev->next = next;
    next->prev = prev;
  }
  next = nullptr;
  prev = nullptr;
  block = nullptr;
}

void Inst::forceRemove() {
  detach();

  for (Inst *input : inputs) {
    std::vector<Inst*> &v = input->outputs;
    v.erase(
      std::remove(
        v.begin(),
        v.end(),
        this
      ),
      v.end()
    );
  }

  for (Inst *output : outputs) {
    std::vector<Inst*> &v = output->inputs;
    v.erase(
      std::remove(
        v.begin(),
        v.end(),
        this
      ),
      v.end()
    );
  }

  inputs.clear();
}

void Inst::remove() {
  detach();

  assert(outputs.empty());

  for (Inst *input : inputs) {
    std::vector<Inst*> &v = input->outputs;
    // Remove ALL occurrences of this
    v.erase(
      std::remove(
        v.begin(),
        v.end(),
        this
      ),
      v.end()
    );
  }
}

void Inst::forceDestroy() {
  if (mounted) {
    forceRemove();
  }
  delete this;
}

void Inst::destroy() {
  if (mounted) {
    remove();
  }
  delete this;
}

void Inst::removeOutput(Inst *inst) {
  // Remove ONE occurrence of inst
  auto iter = std::find(outputs.begin(), outputs.end(), inst);
  assert(iter != outputs.end());
  outputs.erase(iter);
}

void Inst::addInput(Inst *input) {
  inputs.push_back(input);
  input->outputs.push_back(this);
}

void Inst::replaceInput(size_t input, Inst *inst) {
  inputs[input]->removeOutput(this);
  inputs[input] = inst;
  inst->outputs.push_back(this);
}

void Inst::replaceWith(Inst *inst) {
  if (inst == this) return;
  Block *oldBlock = block;
  Inst *oldPrev = prev;
  destroy();
  oldBlock->insertAfter(inst, oldPrev);
}

void Inst::rewriteWith(Inst *inst) {
  assert(inst->mounted);
  if (inst == this) return;
  for (Inst *output : outputs) {
    auto &i = output->inputs;
    std::replace(i.begin(), i.end(), this, inst);
    inst->outputs.push_back(output);
  }
  outputs.clear();
  destroy();
}

void Inst::insertAfter(Inst *inst) {
  assert(mounted);
  assert(!inst->mounted);
  inst->mounted = true;
  inst->block = block;
  inst->prev = this;
  inst->next = next;
  if (next == nullptr) {
    assert(block->open);
    block->last = inst;
  } else {
    next->prev = inst;
  }
  next = inst;
}

void Inst::insertBefore(Inst *inst) {
  assert(mounted);
  assert(!inst->mounted);
  inst->mounted = true;
  inst->block = block;
  inst->prev = prev;
  inst->next = this;
  if (prev == nullptr) {
    block->first = inst;
  } else {
    prev->next = inst;
  }
  prev = inst;
}

void Inst::moveAfter(Inst *inst) {
  inst->detach();
  insertAfter(inst);
}

void Inst::moveBefore(Inst *inst) {
  inst->detach();
  insertBefore(inst);
}

Inst::Inst(
  Block *block,
  InstKind kind,
  const std::vector<Inst*> *inputs
) : kind(kind), block(block) {
  id = block->graph->nextInstId++;
  if (inputs != nullptr) {
    this->inputs = *inputs;
    for (Inst *inst : *inputs) {
      inst->outputs.push_back(this);
    }
  }
}

void Inst::setComment(const std::string &newComment) {
#ifndef NDEBUG
  comment = newComment;
#endif
}

Block::Block(Graph *graph) : graph(graph) {
  id = graph->nextBlockId++;
  graph->blocks.push_back(this);
}

void Block::insertAfter(Inst *inst, Inst *after) {
  assert(!orphan);
  assert(inst != nullptr);
  if (after == nullptr) {
    if (first == nullptr) {
      assert(open);
      first = inst;
      last = inst;
      inst->mounted = true;
      inst->block = this;
    } else {
      first->insertBefore(inst);
    }
  } else {
    after->insertAfter(inst);
  }
}

void Block::insertBefore(Inst *inst, Inst *before) {
  assert(!orphan);
  assert(inst != nullptr);
  if (before == nullptr) {
    if (first == nullptr) {
      assert(open);
      first = inst;
      last = inst;
      inst->mounted = true;
      inst->block = this;
    } else {
      last->insertAfter(inst);
    }
  } else {
    before->insertBefore(inst);
  }
}

void Block::moveAfter(Inst *inst, Inst *after) {
  inst->detach();
  insertAfter(inst, after);
}

void Block::moveBefore(Inst *inst, Inst *before) {
  inst->detach();
  insertBefore(inst, before);
}

void Block::addSuccessor(Block *successor) {
  assert(!orphan);
  successors.push_back(successor);
  successor->predecessors.push_back(this);
}

std::string Block::getLabel() const {
  return std::string("l") + std::to_string(id);
}

void Block::destroy() {
  assert(!orphan);
  orphan = true;
  graph->orphanCount++;
  open = true;

  while (first != nullptr) {
    first->forceDestroy();
  }

  for (Block *predecessor : predecessors) {
    std::vector<Block*> &v = predecessor->successors;
    v.erase(
      std::remove(
        v.begin(),
        v.end(),
        this
      ),
      v.end()
    );
  }
  predecessors.clear();

  for (Block *successor : successors) {
    std::vector<Block*> &v = successor->predecessors;
    v.erase(
      std::remove(
        v.begin(),
        v.end(),
        this
      ),
      v.end()
    );
  }
  successors.clear();
}

void Block::clearPassData() {
  Inst *inst = first;
  while (inst) {
    inst->passData = nullptr;
    inst = inst->next;
  }
}

bool Block::alwaysReaches(Block *block) {
  std::unordered_set<Block*> visited;
  std::vector<Block*> queue = {this};
  bool reaches = false;
  while (!queue.empty()) {
    Block *cur = queue.back();
    visited.insert(cur);
    queue.pop_back();
    if (cur == block) {
      reaches = true;
    } else {
      if (cur->successors.empty()) {
        // We hit an end node
        return false;
      }

      for (Block *successor : cur->successors) {
        if (!visited.contains(successor)) {
          queue.push_back(successor);
        }
      }
    }
  }
  return reaches;
}

bool Block::reaches(Block *block) {
  std::unordered_set<Block*> visited;
  std::vector<Block*> queue = {this};
  while (!queue.empty()) {
    Block *cur = queue.back();
    visited.insert(cur);
    queue.pop_back();
    for (Block *successor : cur->successors) {
      if (successor == block) {
        return true;
      } else if (!visited.contains(successor)) {
        queue.push_back(successor);
      }
    }
  }
  return false;
}

bool Block::reachedBy(Block *block) {
  if (block == nullptr) return true;
  std::unordered_set<Block*> visited;
  std::vector<Block*> queue = {this};
  while (!queue.empty()) {
    Block *cur = queue.back();
    visited.insert(cur);
    queue.pop_back();
    for (Block *predecessor : cur->predecessors) {
      if (predecessor == block) {
        return true;
      } else if (!visited.contains(predecessor)) {
        queue.push_back(predecessor);
      }
    }
  }
  return false;
}

bool Block::alwaysReachedBy(Block *block) {
  if (block == nullptr) return true;
  std::unordered_set<Block*> visited;
  std::vector<Block*> queue = {this};
  bool reaches = false;
  while (!queue.empty()) {
    Block *cur = queue.back();
    queue.pop_back();
    if (cur == block) {
      reaches = true;
    } else {
      if (cur->predecessors.empty()) {
        // We hit the entry node
        return false;
      }

      for (Block *predecessor : cur->predecessors) {
        if (!visited.contains(predecessor)) {
          visited.insert(cur);
          queue.push_back(predecessor);
        }
      }
    }
  }
  return reaches;
}

void Block::assignCommonDominator(Block *predecessor) {
  if (dominator == nullptr) {
    setDominator(predecessor);
  } else if (predecessor->dominator != nullptr) {
    Block *left = dominator;
    Block *right = predecessor;

    while (left != right) {
      if (left->id > right->id) {
        left = left->dominator;
      } else {
        right = right->dominator;
      }
      assert(left != nullptr);
      assert(right != nullptr);
    }

    if (dominator != left) {
      removeDominator();
      setDominator(left);
    }
  }
}

void Block::setDominator(Block *block) {
  dominator = block;
}

void Block::removeDominator() {
  dominator = nullptr;
}

bool Block::dominatedBy(Block *block) {
  if (block == nullptr || block == this) return true;
  if (dominator == nullptr) return false;
  return dominator->dominatedBy(block);
}

bool Block::dominates(Block *block) {
  if (block == nullptr) return false;
  return block->dominatedBy(this);
}

Block *Block::getDominator() {
  if (!graph->builtDominators) {
    graph->buildDominators();
  }
  return dominator;
}

void Graph::destroy() {
  assert(!destroyed);

  for (Block *block : blocks) {
    if (!block->orphan) block->destroy();
    block->graph = nullptr;
    delete block;
  }
  blocks.clear();

  destroyed = true;
}

void Graph::clearPassData() {
  for (Block *block : blocks) {
    block->passData = nullptr;
    block->clearPassData();
  }
}

Graph::Graph(const BFVM::Config &config) : config(config) {}

void Graph::clearDominators() {
  builtDominators = false;
}

void Graph::buildDominators() {
  for (Block *block : blocks) {
    block->dominator = nullptr;
  }

  for (Block *block : blocks) {
    for (Block *predecessor : block->predecessors) {
      if (predecessor->id < block->id) {
        block->assignCommonDominator(predecessor);
      }
    }
    assert(block->predecessors.empty() || block->dominator != nullptr);
  }

  builtDominators = true;
}

Builder::Builder(Graph &graph) : config(graph.config), graph(graph) {}

Inst *Builder::push(
  InstKind kind,
  const std::vector<Inst*> *inputs
) {
  auto newInst = new Inst(block, kind, inputs);
  block->insertAfter(newInst, inst);
  inst = newInst;
  return newInst;
}

Inst *Builder::pushImm(int64_t imm, TypeId typeId) {
  auto newInst = push(I_IMM);
  if (typeId == T_INVALID) {
    typeId = typeForWidth(graph.config.cellWidth);
  }
  newInst->type = typeId;
  newInst->immValue = imm;
  return newInst;
}

Inst *Builder::pushUnary(InstKind kind, Inst *x) {
  std::vector<Inst*> inputs = {x};
  return push(kind, &inputs);
}

Inst *Builder::pushBinary(InstKind kind, Inst *x, Inst *y) {
  std::vector<Inst*> inputs = {x, y};
  return push(kind, &inputs);
}

Inst *Builder::pushIf(
  Inst *cond,
  Block *whenTrue,
  Block *whenFalse
) {
  std::vector<Block*> successors = {whenTrue, whenFalse};
  std::vector<Inst*> inputs = {cond};
  return closeBlock(I_IF, &inputs, &successors);
}

void Builder::setAfter(Block *newBlock, Inst *after) {
  block = newBlock;
  inst = after;
}

void Builder::setAfter(Inst *after) {
  assert(after != nullptr);
  block = after->block;
  inst = after;
}

void Builder::setBefore(Block *newBlock, Inst *before) {
  block = newBlock;
  if (before == nullptr) {
    inst = block->last;
  } else {
    inst = before->prev;
  }
}

void Builder::setBefore(Inst *before) {
  assert(before != nullptr);
  block = before->block;
  inst = before->prev;
}

Block *Builder::openBlock() {
  auto newBlock = new Block(&graph);
  setBefore(newBlock);
  return newBlock;
}

Inst* Builder::closeBlock(
  InstKind kind,
  const std::vector<Inst*> *inputs,
  const std::vector<Block*> *successors
) {
  assert(inst == block->last);
  auto newInst = push(kind, inputs);
  block->open = false;
  assert(block->successors.empty());
  if (successors != nullptr) {
    for (Block* successor : *successors) {
      block->addSuccessor(successor);
    }
    block->successors = *successors;
  }
  return newInst;
}

Inst *Builder::pushReg(RegKind reg) {
  auto newInst = push(I_REG);
  newInst->immReg = reg;
  return newInst;
}

Inst *Builder::pushSetReg(RegKind reg, Inst *x) {
  auto newInst = pushUnary(I_SETREG, x);
  newInst->immReg = reg;
  return newInst;
}