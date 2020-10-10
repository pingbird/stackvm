#include <iostream>
#include <string>
#include <unordered_map>

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
  int defIndex = 0;

  std::unordered_map<BF::DefIndex, IR::Inst*> defs;

  void buildOffset(int offset, IR::RegKind reg = IR::R_PTR) {
    if (!offset) return;
    b.pushSetReg(
      reg,
      b.pushGep(
        b.pushReg(reg),
        b.pushImm(offset, IR::T_SIZE)
      )
    );
  }

  LoopBlocks openLoop(IR::RegKind reg = IR::R_PTR) {
    LoopBlocks blocks(&graph);

    b.pushGoto(blocks.cond);

    b.setBlock(blocks.cond);
    b.pushIf(
      b.pushLd(b.pushReg(reg)),
      blocks.loop,
      blocks.next
    );

    b.setBlock(blocks.loop);
    return blocks;
  }

  void closeLoop(LoopBlocks &blocks, IR::RegKind reg = IR::R_PTR) {
    b.pushGoto(blocks.cond);
    b.setBlock(blocks.next);
    b.pushStr(b.pushReg(reg), b.pushImm(0));
  }

  void buildSeek(const BF::Seek &seek) {
    buildOffset(seek.offset, IR::R_DEF);
    for (const BF::SeekLoop &loop : seek.loops) {
      auto blocks = openLoop(IR::R_DEF);
      buildSeek(loop.seek);
      closeLoop(blocks, IR::R_DEF);
      buildOffset(loop.offset, IR::R_DEF);
    }
  }

  void buildDef(const BF::Def &def) {
    for (auto sub : def.body) {
      if (auto def = std::get_if<BF::Def>(&sub)) {
        buildDef(*def);
      } else if (auto seek = std::get_if<BF::Seek>(&sub)) {
        buildSeek(*seek);
      }
    }
    defs[def.index] = b.pushReg(IR::R_DEF);
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
        } case BF::I_DEF: {
          b.pushSetReg(IR::R_DEF, b.pushReg(IR::R_PTR));
          buildDef(program.defs[defIndex++]);
          break;
        } case BF::I_SEEK: {
          IR::Inst *defReg = defs[program.seeks[seekIndex++]];
          assert(defReg != nullptr);
          b.pushSetReg(IR::R_PTR, defReg);
          break;
        } case BF::I_LOOP: {
          auto blocks = openLoop();
          buildBody();
          if (pos < program.block.size()) {
            auto endInst = program.block[pos++];
            assert(endInst == BF::I_END);
          }
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
