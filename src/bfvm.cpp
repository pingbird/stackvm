#include <iostream>
#include <filesystem>
#include <fstream>
#include <cassert>

#include "bfvm.h"
#include "ir_print.h"
#include "bf.h"
#include "diagnostics.h"
#include "lowering.h"
#include "opt.h"
#include "jit.h"

#ifndef NDIAG
struct CommandLineDiag : Diag {
  const BFVM::Config &config;

  explicit CommandLineDiag(const BFVM::Config &config) : config(config) {}

  void log(const std::string& string) override {
    std::cerr << "[log] " << string << std::endl;
  }

  void event(const std::string& name) override {
    std::cerr << "[event] " << name << std::endl;
  }

  void eventStart(const std::string& name) override {
    std::cerr << "[start] " << name << std::endl;
  }

  void eventFinish(const std::string& name) override {
    std::cerr << "[finish] " << name << std::endl;
  }

  void artifact(const std::string& name, const DiagCollector &contents) override {
    if (config.enableArtifacts) {
      std::ofstream ofs(config.artifactsDir + "/" + name);
      ofs << contents();
      ofs.close();
    }
  }
};
#endif

void BFVM::run(const std::string &code, const Config &config) {
#ifndef NDIAG
  auto diag = new CommandLineDiag(config);
  auto artifactsPath = std::filesystem::canonical(config.artifactsDir);
  if (artifactsPath.empty()) {
    std::cerr << "Error: Artifacts directory cannot be empty.";
    abort();
  }
  std::filesystem::remove_all(config.artifactsDir);
  assert(std::filesystem::create_directory(config.artifactsDir));
#endif
  DIAG(eventStart, "Compile")

  DIAG(event, "Parse BF")
  auto program = BF::parse(code);

  DIAG_ARTIFACT("bf.txt", BF::print(program))

  DIAG(event, "Lower BF")
  auto graph = Lowering::buildProgram(&program);

  DIAG_ARTIFACT("ir_unopt.txt", IR::printGraph(graph))

  Opt::validate(graph);

  DIAG(event, "Optimize IR")

  Opt::resolveRegs(graph);
  Opt::validate(graph);

  DIAG_ARTIFACT("ir.txt", IR::printGraph(graph))

  DIAG(event, "Initialize JIT")

  JIT::init();
  JIT::Pipeline jit;
  DIAG_FWD(&jit)
  jit.addSymbol("native_putchar", putchar);

  auto handle = jit.compile(graph);
  graph->destroy();

  std::cout << "Running..." << std::endl;
  char memory[4096] = {0};
  handle->entry(memory);

  delete graph;
  delete diag;
}