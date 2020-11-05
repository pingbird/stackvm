#pragma once

#include <vector>
#include <set>
#include <cassert>
#include <unordered_set>

#include "bfvm.h"

namespace IR {
  enum RegKind {
    R_PTR,
    R_DEF,
  };

  static const int NUM_REGS = R_DEF + 1;
  extern const char *regNames[NUM_REGS];

  enum InstKind : uint8_t {
    I_NOP,
    I_IMM,
    I_ADD,
    I_SUB,
    I_GEP,
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

  typedef uint16_t TypeId;

  enum BuiltinTypeId : TypeId {
    T_INVALID,
    T_NONE,
    T_PTR,
    T_SIZE,
    T_I8,
    T_I16,
    T_I32,
    T_I64,
    T_USER_START,
  };

  const TypeId T_LOW = T_I8;
  const TypeId T_HI = T_I64;

  std::string typeString(TypeId type);

  static TypeId typeForWidth(int width) {
    switch (width) {
      case 8:
        return T_I8;
      case 16:
        return T_I16;
      case 32:
        return T_I32;
      case 64:
        return T_I64;
      default:
        abort();
    }
  }

  static int minType(TypeId x, TypeId y) {
    if (x == y) return x;
    assert(x >= T_LOW && x <= T_HI);
    assert(y >= T_LOW && y <= T_HI);
    return x > y ? y : x;
  }

  static int maxType(TypeId x, TypeId y) {
    if (x == y) return x;
    assert(x >= T_LOW && x <= T_HI);
    assert(y >= T_LOW && y <= T_HI);
    return x > y ? x : y;
  }

  static bool instIsPure(InstKind kind) {
    switch (kind) {
      case I_NOP:
      case I_IMM:
      case I_ADD:
      case I_SUB:
      case I_GEP:
      case I_LD:
      case I_REG:
      case I_PHI:
        return true;
      default:
        return false;
    }
  }

  static bool instIsOrd(InstKind kind) {
    switch (kind) {
      case I_LD:
      case I_REG:
        return true;
      default:
        return false;
    }
  }

  struct Block;
  struct Graph;

  struct Inst {
    explicit Inst(
      Block *block,
      InstKind kind,
      const std::vector<Inst*> *inputs = nullptr
    );

    int id;

    TypeId type = T_INVALID;

    Inst* prev = nullptr;
    Inst* next = nullptr;

    Block* block = nullptr;

    bool mounted = false;

    InstKind kind;

    std::vector<Inst*> inputs;
    std::vector<Inst*> outputs;

#ifndef NDEBUG
    std::string comment;
#endif

    union {
      int64_t immValue;
      RegKind immReg;
    };

    void* passData = nullptr;

    // Detaches this instruction without touching inputs and outputs.
    void detach();

    // Forcefully removes this instruction, unwiring all inputs and outputs.
    void forceRemove();

    // Removes this instruction without destroying it, unwiring inputs.
    void remove();

    // Forcefully destroys this instruction, unwiring all inputs and outputs.
    void forceDestroy();

    // Destroys this instruction, only unwiring inputs.
    void destroy();

    void addInput(Inst *input);

    // Safely destroys this instruction and inserts the provided one in its place.
    void replaceWith(Inst *inst);

    // Destroys this instruction, rewiring any outputs to the new instruction.
    void rewriteWith(Inst *inst);

    // Inserts the given instruction after itself.
    void insertAfter(Inst *inst);

    // Moves the given instruction after itself.
    void moveAfter(Inst *inst);

    // Inserts the given instruction before itself.
    void insertBefore(Inst *inst);

    // Move the given instruction before itself.
    void moveBefore(Inst *inst);

    void setComment(const std::string& newComment);
  };

  struct Block {
    explicit Block(Graph *graph);

    int id;

    Inst *first = nullptr;
    Inst *last = nullptr;

    Graph *graph = nullptr;

    bool open = true;
    bool orphan = false;

    void* passData = nullptr;

    void clearPassData();

    void destroy();

    // Inserts [inst] after [after], where null is before the first instruction
    void insertAfter(Inst *inst, Inst *after);

    // Inserts [inst] after [before], where null is after the last instruction
    void insertBefore(Inst *inst, Inst *before);

    // Moves [inst] after [after], where null is before the first instruction
    void moveAfter(Inst *inst, Inst *after);

    // Moves [inst] before [before], where null is after the last instruction
    void moveBefore(Inst *inst, Inst *before);

    std::vector<Block*> predecessors;
    std::vector<Block*> successors;

    Block *dominator = nullptr;

    Block *getDominator();

    // Whether this block is dominated by [block], including itself and null
    bool dominatedBy(Block *block);

    // Whether this block dominates [block], including itself, but never null
    bool dominates(Block *block);

    void setDominator(Block *block);
    void removeDominator();
    void addSuccessor(Block *successor);

    void assignCommonDominator(Block *predecessor);

    // Whether this block can ever reach the given block.
    bool reaches(Block *block);

    // Whether this block can ever be reached by the given block.
    bool reachedBy(Block *block);

    // Whether this block dominates the given block, without using the dominator tree.
    bool alwaysReaches(Block *block);

    // Whether this block is dominated by the given block, without using the dominator tree.
    bool alwaysReachedBy(Block *block);

    [[nodiscard]] std::string getLabel() const;
  };

  struct Graph {
    const BFVM::Config &config;

    int nextBlockId = 1;
    int nextInstId = 1;

    std::vector<Block*> blocks;

    int orphanCount = 0;

    bool destroyed = false;

    bool builtDominators = false;

    explicit Graph(const BFVM::Config &config);

    void clearPassData();

    void buildDominators();

    void clearDominators();

    void destroy();
  };

  struct Builder {
    explicit Builder(Graph &graph);

    const BFVM::Config &config;
    Graph &graph;
    Block *block = nullptr;
    Inst *inst = nullptr;

    void setAfter(Block *newBlock, Inst *after = nullptr);
    void setAfter(Inst *after);
    void setBefore(Block *newBlock, Inst *before = nullptr);
    void setBefore(Inst *before);

    Inst *push(
      InstKind kind,
      const std::vector<Inst*> *inputs = nullptr
    );

    Inst *pushUnary(InstKind kind, Inst *x);
    Inst *pushBinary(InstKind kind, Inst *x, Inst *y);

    Inst *pushNop() { return push(I_NOP); }
    Inst *pushImm(int64_t imm, TypeId typeId = T_INVALID);
    Inst *pushAdd(Inst *x, Inst *y) { return pushBinary(I_ADD, x, y); }
    Inst *pushSub(Inst *x, Inst *y) { return pushBinary(I_SUB, x, y); }
    Inst *pushGep(Inst *x, Inst *y) { return pushBinary(I_GEP, x, y); }
    Inst *pushLd(Inst *x) { return pushUnary(I_LD, x); }
    Inst *pushStr(Inst *x, Inst *y) { return pushBinary(I_STR, x, y); }
    Inst *pushReg(RegKind reg);
    Inst *pushSetReg(RegKind reg, Inst *x);
    Inst *pushGetchar() { return push(I_GETCHAR); }
    Inst *pushPutchar(Inst *x) { return pushUnary(I_PUTCHAR, x); }
    Inst *pushPhi(const std::vector<Inst*> *inputs = nullptr) { return push(I_PHI, inputs); }

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

    Inst *pushRet(Inst *x) {
      std::vector<Inst*> inputs = {x};
      return closeBlock(I_RET, &inputs);
    }
  };
}