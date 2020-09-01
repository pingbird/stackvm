#include <iostream>
#include <string>

#include "src/lowering.h"
#include "src/ir_print.h"
#include "src/opt.h"
#include "src/jit.h"

extern "C" {
  int testPutchar(int c) {
    std::cout << "char(" << std::to_string(c) << ")" << std::endl;
    return 0;
  }
}

int main() {
  auto program = BF::parse("-[-]>-.");

  std::cout << BF::print(program) << std::endl;

  std::cout << "----------------------------------------" << std::endl;

  auto graph = Lowering::buildProgram(&program);
  Opt::validate(graph);
  auto dump = printGraph(graph);
  std::cout << dump << std::endl;

  std::cout << "----------------------------------------" << std::endl;

  Opt::resolveRegs(graph);
  Opt::validate(graph);
  dump = printGraph(graph);
  std::cout << dump << std::endl;

  std::cout << "----------------------------------------" << std::endl;

  JIT::init();
  JIT::Pipeline jit;
  jit.addSymbol("native_putchar", testPutchar);
  auto handle = jit.compile(graph);

  char memory[4096];
  handle->entry(memory);

  graph->destroy();
  delete graph;
  return 0;
}
