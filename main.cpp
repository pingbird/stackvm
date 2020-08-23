#include <iostream>
#include <string>

#include "src/lowering.h"
#include "src/ir_print.h"
#include "src/opt.h"

int main() {
  auto program = BF::parse("-[>-<---]>-.");
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
  graph->destroy();
  delete graph;
  return 0;
}
