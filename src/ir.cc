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

void Inst::remove() {
  assert(mounted);
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

  block = nullptr;
}

void Inst::safeRemove() {
  assert(outputs.empty());
  remove();
}

void Inst::destroy() {
  if (mounted) {
    remove();
  }
  delete this;
}

void Inst::safeDestroy() {
  if (mounted) {
    safeRemove();
  }
  delete this;
}

void Inst::replaceWith(Inst *inst) {
  Block *oldBlock = block;
  Inst *oldPrev = prev;
  destroy();
  oldBlock->insertAfter(inst, oldPrev);
}

void Inst::rewriteWith(Inst *inst) {
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

void Block::addSuccessor(Block *successor) {
  assert(!orphan);
  successors.push_back(successor);
  successor->predecessors.push_back(this);
}

std::string Block::getLabel() const {
  return std::string("l") + std::to_string(id);
}

void Block::remove() {
  assert(!orphan);
  orphan = true;
  graph->orphanCount++;
  open = true;

  while (first != nullptr) {
    first->destroy();
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

void Graph::destroy() {
  assert(!destroyed);

  for (Block *block : blocks) {
    if (!block->orphan) block->remove();
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

void Builder::setBlock(Block* newBlock) {
  block = newBlock;
  inst = block->last;
}

void Builder::setBlock(Block *newBlock, Inst *newInst) {
  block = newBlock;
  inst = newInst;
}

Block *Builder::openBlock() {
  auto newBlock = new Block(&graph);
  setBlock(newBlock);
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
