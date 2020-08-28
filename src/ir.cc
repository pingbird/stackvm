#include "ir.h"

#include <cassert>
#include <string>
#include <algorithm>

std::string IR::typeString(TypeId type) {
  switch (type) {
    case T_INVALID: return "Invalid";
    case T_PTR: return "Ptr";
    case T_I32: return "I32";
    case T_I24: return "I24";
    case T_I16: return "I16";
    case T_I8: return "I8";
    default:
      return "User" + std::to_string(type - T_USER_START);
  }
}

const char *IR::regNames[1] = {
  "ptr",
};

void IR::Inst::remove() {
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

void IR::Inst::safeRemove() {
  assert(outputs.empty());
  remove();
}

void IR::Inst::destroy() {
  if (mounted) {
    remove();
  }
  delete this;
}

void IR::Inst::safeDestroy() {
  if (mounted) {
    safeRemove();
  }
  delete this;
}

void IR::Inst::replaceWith(Inst *inst) {
  Block *oldBlock = block;
  Inst *oldPrev = prev;
  destroy();
  oldBlock->insertAfter(inst, oldPrev);
}

void IR::Inst::rewriteWith(Inst *inst) {
  for (Inst *output : outputs) {
    auto &i = output->inputs;
    std::replace(i.begin(), i.end(), this, inst);
    inst->outputs.push_back(output);
  }
  outputs.clear();
  destroy();
}

void IR::Inst::insertAfter(IR::Inst *inst) {
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

void IR::Inst::insertBefore(IR::Inst *inst) {
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

IR::Inst::Inst(
  Block *block,
  IR::InstKind kind,
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

IR::Block::Block(Graph *graph) : graph(graph) {
  id = graph->nextBlockId++;
  graph->blocks.push_back(this);
}

void IR::Block::insertAfter(IR::Inst *inst, IR::Inst *after) {
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

void IR::Block::insertBefore(IR::Inst *inst, IR::Inst *before) {
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

void IR::Block::addSuccessor(IR::Block *successor) {
  assert(!orphan);
  successors.push_back(successor);
  successor->predecessors.push_back(this);
}

std::string IR::Block::getLabel() const {
  return std::string("l") + std::to_string(id);
}

void IR::Block::remove() {
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

void IR::Graph::destroy() {
  assert(!destroyed);

  for (Block *block : blocks) {
    if (!block->orphan) block->remove();
    block->graph = nullptr;
    delete block;
  }
  blocks.clear();

  destroyed = true;
}

IR::Inst *IR::Builder::push(
  IR::InstKind kind,
  const std::vector<Inst*> *inputs
) {
  auto newInst = new Inst(block, kind, inputs);
  block->insertAfter(newInst, inst);
  inst = newInst;
  return newInst;
}

IR::Inst *IR::Builder::pushImm(Constants::Imm imm) {
  auto newInst = push(I_IMM);
  newInst->immValue = imm;
  return newInst;
}

IR::Inst *IR::Builder::pushUnary(IR::InstKind kind, IR::Inst *x) {
  std::vector<Inst*> inputs = {x};
  return push(kind, &inputs);
}

IR::Inst *IR::Builder::pushBinary(IR::InstKind kind, IR::Inst *x, IR::Inst *y) {
  std::vector<Inst*> inputs = {x, y};
  return push(kind, &inputs);
}

IR::Inst *IR::Builder::pushIf(
  IR::Inst *cond,
  IR::Block *whenTrue,
  IR::Block *whenFalse
) {
  std::vector<Block*> successors = {whenTrue, whenFalse};
  std::vector<Inst*> inputs = {cond};
  return closeBlock(I_IF, &inputs, &successors);
}

void IR::Builder::setBlock(Block* newBlock) {
  block = newBlock;
  inst = block->last;
}

void IR::Builder::setBlock(IR::Block *newBlock, IR::Inst *newInst) {
  block = newBlock;
  inst = newInst;
}

IR::Block *IR::Builder::openBlock() {
  auto newBlock = new Block(graph);
  setBlock(newBlock);
  return newBlock;
}

IR::Inst* IR::Builder::closeBlock(
  IR::InstKind kind,
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

IR::Inst *IR::Builder::pushReg(IR::RegKind reg) {
  auto newInst = push(I_REG);
  newInst->immReg = reg;
  return newInst;
}

IR::Inst *IR::Builder::pushSetReg(IR::RegKind reg, IR::Inst *x) {
  auto newInst = pushUnary(I_SETREG, x);
  newInst->immReg = reg;
  return newInst;
}