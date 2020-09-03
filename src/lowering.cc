#include <iostream>
#include <string>

#include "bf.h"
#include "ir.h"
#include "lowering.h"

struct Builder {
  explicit Builder(IR::Graph &graph, const std::vector<BF::Inst> &program) :
    graph(graph),
    config(graph.config),
    program(program),
    b(graph) {}

  const BFVM::Config &config;
  const std::vector<BF::Inst> &program;
  IR::Graph &graph;
  IR::Builder b;
  int pos = 0;

  void buildBody() {
    auto length = program.size();
    while (pos != length) {
      auto inst = program[pos++];
      switch (inst) {
        case BF::I_ADD: {
          int count = 1;
          while (pos != length && program[pos] == BF::I_ADD) {
            count++;
            pos++;
          }
          auto value = b.pushLdPtr();
          value = b.pushAdd(value, b.pushImm(count));
          b.pushStrPtr(value);
          break;
        } case BF::I_SUB: {
          int count = 1;
          while (pos != length && program[pos] == BF::I_SUB) {
            count++;
            pos++;
          }
          auto value = b.pushLdPtr();
          value = b.pushSub(value, b.pushImm(count));
          b.pushStrPtr(value);
          break;
        } case BF::I_LEFT: {
          int count = 1;
          while (pos != length && program[pos] == BF::I_LEFT) {
            count++;
            pos++;
          }
          b.pushSetReg(
            IR::R_PTR,
            b.pushGep(
              b.pushReg(IR::R_PTR),
              b.pushImm(-count, IR::T_PTR)
            )
          );
          break;
        } case BF::I_RIGHT: {
          int count = 1;
          while (pos != length && program[pos] == BF::I_RIGHT) {
            count++;
            pos++;
          }
          b.pushSetReg(
            IR::R_PTR,
            b.pushGep(
              b.pushReg(IR::R_PTR),
              b.pushImm(count, IR::T_PTR)
            )
          );
          break;
        } case BF::I_LOOP: {
          auto condBlock = new IR::Block(&graph);
          auto loopBlock = new IR::Block(&graph);
          auto nextBlock = new IR::Block(&graph);

          b.pushGoto(condBlock);

          b.setBlock(condBlock);
          b.pushIf(
            b.pushLdPtr(),
            loopBlock,
            nextBlock
          );

          b.setBlock(loopBlock);
          buildBody();
          auto endInst = program[pos++];
          assert(endInst == BF::I_END);
          b.pushGoto(condBlock);

          b.setBlock(nextBlock);
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
    assert(pos == program.size());
  }
};

std::unique_ptr<IR::Graph> Lowering::buildProgram(const BFVM::Config &config, const std::vector<BF::Inst> &program) {
  auto graph = std::make_unique<IR::Graph>(config);
  Builder(*graph, program).buildProgram();
  return graph;
}
