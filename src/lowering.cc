#include <iostream>
#include <string>

#include "bf.h"
#include "ir.h"
#include "lowering.h"

struct LoopBlocks {
  explicit LoopBlocks(IR::Graph *graph) :
    cond(new IR::Block(graph)),
    loop(new IR::Block(graph)),
    next(new IR::Block(graph)) {}

  IR::Block *cond;
  IR::Block *loop;
  IR::Block *next;
};

struct Builder {
  explicit Builder(IR::Graph &graph, const BF::Program &program) :
    graph(graph),
    config(graph.config),
    program(program),
    b(graph) {}

  const BFVM::Config &config;
  const BF::Program &program;
  IR::Graph &graph;
  IR::Builder b;
  int pos = 0;
  int seekIndex = 0;

  void buildOffset(int offset) {
    if (!offset) return;
    b.pushSetReg(
      IR::R_PTR,
      b.pushGep(
        b.pushReg(IR::R_PTR),
        b.pushImm(offset, IR::T_SIZE)
      )
    );
  }

  LoopBlocks openLoop() {
    LoopBlocks blocks(&graph);

    b.pushGoto(blocks.cond);

    b.setBlock(blocks.cond);
    b.pushIf(
      b.pushLdPtr(),
      blocks.loop,
      blocks.next
    );

    b.setBlock(blocks.loop);
    return blocks;
  }

  void closeLoop(LoopBlocks &blocks) {
    b.pushGoto(blocks.cond);
    b.setBlock(blocks.next);
    b.pushStrPtr(b.pushImm(0));
  }

  void buildSeek(const BF::Seek &seek) {
    buildOffset(seek.offset);
    for (const BF::SeekLoop &loop : seek.loops) {
      auto blocks = openLoop();
      buildSeek(loop.seek);
      closeLoop(blocks);
      buildOffset(loop.offset);
    }
  }

  void buildBody() {
    auto length = program.block.size();
    while (pos != length) {
      auto inst = program.block[pos++];
      switch (inst) {
        case BF::I_ADD: {
          int count = 1;
          while (pos != length && program.block[pos] == BF::I_ADD) {
            count++;
            pos++;
          }
          auto value = b.pushLdPtr();
          value = b.pushAdd(value, b.pushImm(count));
          b.pushStrPtr(value);
          break;
        } case BF::I_SUB: {
          int count = 1;
          while (pos != length && program.block[pos] == BF::I_SUB) {
            count++;
            pos++;
          }
          auto value = b.pushLdPtr();
          value = b.pushSub(value, b.pushImm(count));
          b.pushStrPtr(value);
          break;
        } case BF::I_SEEK: {
          buildSeek(program.seeks[seekIndex++]);
          break;
        } case BF::I_LOOP: {
          auto blocks = openLoop();
          buildBody();
          auto endInst = program.block[pos++];
          assert(endInst == BF::I_END);
          closeLoop(blocks);
          break;
        } case BF::I_END:
          pos--;
          return;
        case BF::I_PUTCHAR:
          b.pushPutchar(b.pushLdPtr());
          break;
        case BF::I_GETCHAR:
          b.pushStrPtr(b.pushGetchar());
          break;
      }
    }
  }

  void buildProgram() {
    b.openBlock();
    buildBody();
    b.pushRet(b.pushReg(IR::R_PTR));
    assert(pos == program.block.size());
  }
};

std::unique_ptr<IR::Graph> Lowering::buildProgram(const BFVM::Config &config, const BF::Program &program) {
  auto graph = std::make_unique<IR::Graph>(config);
  Builder(*graph, program).buildProgram();
  return graph;
}
