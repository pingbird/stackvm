#pragma once

#include <vector>
#include <set>

#include "constants.h"

namespace IR {
  enum RegKind {
    R_PTR,
  };

  static const int NUM_REGS = R_PTR + 1;
  extern const char *regNames[NUM_REGS];

  enum InstKind {
    I_NOP,
    I_IMM,
    I_ADD,
    I_SUB,
    I_LD,
    I_STR,
    I_REG,
    I_SETREG,
    I_GETCHAR,
    I_PUTCHAR,
    I_PHI,
    I_IF,
    I_GOTO,
    I_RET,
  };

  static bool instIsPure(InstKind kind) {
    switch (kind) {
      case I_NOP:
      case I_IMM:
      case I_ADD:
      case I_SUB:
      case I_LD:
      case I_REG:
      case I_PHI:
        return true;
      default:
        return false;
    }
  };

  static bool instIsOrd(InstKind kind) {
    switch (kind) {
      case I_LD:
      case I_REG:
        return true;
      default:
        return false;
    }
  };

  struct Block;
  struct Graph;

  struct Inst {
    explicit Inst(
      Block *block,
      InstKind kind,
      const std::vector<Inst*> *inputs = nullptr
    );

    int id;

    Inst* prev = nullptr;
    Inst* next = nullptr;

    Block* block = nullptr;

    bool mounted = false;

    InstKind kind;

    std::vector<Inst*> inputs;
    std::vector<Inst*> outputs;

    int64_t immValue = 0;

    void remove();
    void safeRemove();

    void destroy();
    void safeDestroy();

    void replaceWith(Inst *inst);
    void rewriteWith(Inst *inst);

    void insertAfter(Inst *inst);
    void insertBefore(Inst *inst);
  };

  struct Block {
    explicit Block(Graph *graph);

    int id;

    Inst *first = nullptr;
    Inst *last = nullptr;

    Graph *graph = nullptr;

    bool open = true;
    bool orphan = false;

    std::vector<Block*> predecessors;
    std::vector<Block*> successors;

    void remove();
    void insertAfter(Inst *inst, Inst *after);
    void insertBefore(Inst *inst, Inst *before);

    void addSuccessor(Block* successor);

    [[nodiscard]] std::string getLabel() const;
  };

  struct Graph {
    int nextBlockId = 1;
    int nextInstId = 1;

    std::vector<Block*> blocks;

    bool destroyed = false;

    void destroy();
  };

  struct Builder {
    explicit Builder(Graph *graph) : graph(graph) {}

    Graph *graph = nullptr;
    Block *block = nullptr;
    Inst *inst = nullptr;

    void setBlock(Block* newBlock);
    void setBlock(Block* newBlock, Inst* newInst);

    Inst *push(
      InstKind kind,
      const std::vector<Inst*> *inputs = nullptr
    );

    Inst *pushUnary(InstKind kind, Inst *x);
    Inst *pushBinary(InstKind kind, Inst *x, Inst *y);

    Inst *pushNop() { return push(I_NOP); }
    Inst *pushImm(Constants::Word imm);
    Inst *pushAdd(Inst *x, Inst* y) { return pushBinary(I_ADD, x, y); }
    Inst *pushSub(Inst *x, Inst* y) { return pushBinary(I_SUB, x, y); }
    Inst *pushLd(Inst *x) { return pushUnary(I_LD, x); }
    Inst *pushStr(Inst *x, Inst* y) { return pushBinary(I_STR, x, y); }
    Inst *pushReg(RegKind reg);
    Inst *pushSetReg(RegKind reg, Inst *x);
    Inst *pushGetchar() { return push(I_GETCHAR); }
    Inst *pushPutchar(Inst *x) { return pushUnary(I_PUTCHAR, x); }
    Inst *pushPhi(const std::vector<Inst*> *inputs) { return push(I_PHI, inputs); }

    Inst *pushLdPtr() { return pushLd(pushReg(R_PTR)); }
    Inst *pushStrPtr(Inst *x) { return pushStr(pushReg(R_PTR), x); }

    Block *openBlock();

    Inst *closeBlock(
      InstKind kind,
      const std::vector<Inst*> *inputs = nullptr,
      const std::vector<Block*> *successors = nullptr
    );

    Inst *pushIf(
      Inst *cond,
      Block *whenTrue,
      Block *whenFalse
    );

    Inst *pushGoto(Block *toBlock) {
      std::vector<Block*> successors = {toBlock};
      return closeBlock(I_GOTO, nullptr, &successors);
    }

    Inst *pushRet() { return closeBlock(I_RET); }
  };
}