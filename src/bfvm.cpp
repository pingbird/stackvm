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
  std::unordered_map<std::string, int64_t> events;

  explicit CommandLineDiag(const BFVM::Config &config) : config(config) {}

  void log(const std::string& string) override {
    std::cerr << "[ log    ] " << string << std::endl;
  }

  void event(const std::string& name) override {
    std::cerr << "[ event  ] " << name << std::endl;
  }

  void eventStart(const std::string& name) override {
    if (events.count(name)) {
      abort();
    }
    std::cerr
      << "[ start  ] "
      << std::string(events.size() + 1, '>')
      << " " << name << std::endl;
    events[name] = Time::getTime();
  }

  void eventFinish(const std::string& name) override {
    int64_t endTime = Time::getTime();
    if (!events.count(name)) {
      abort();
    }
    std::cerr
      << "[ finish ] "
      << std::string(events.size(), '<')
      << " " << name << " - "
      << Time::printTime(endTime - events[name])
      << std::endl;
    events.erase(name);
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
  auto artifactsPath = std::filesystem::weakly_canonical(config.artifactsDir);
  if (artifactsPath.empty()) {
    std::cerr << "Error: Artifacts directory cannot be empty.";
    abort();
  }
  std::filesystem::remove_all(config.artifactsDir);
  assert(std::filesystem::create_directory(config.artifactsDir));
#endif
  DIAG(eventStart, "No-op")
  DIAG(eventFinish, "No-op")

  DIAG(eventStart, "Build")

  DIAG(eventStart, "Parse")
  auto program = BF::parse(code);
  DIAG(eventFinish, "Parse")

  DIAG_ARTIFACT("bf.txt", BF::print(program))

  DIAG(eventStart, "Lower")
  auto graph = Lowering::buildProgram(&program);
  Opt::validate(graph);
  DIAG(eventFinish, "Lower")

  DIAG_ARTIFACT("ir_unopt.txt", IR::printGraph(graph))

  DIAG(eventStart, "Optimize")
  Opt::resolveRegs(graph);
  Opt::validate(graph);
  DIAG(eventFinish, "Optimize")

  DIAG_ARTIFACT("ir.txt", IR::printGraph(graph))

  DIAG(event, "Initialize JIT")

  JIT::init();
  JIT::Pipeline jit;
  DIAG_FWD(jit)
  jit.addSymbol("native_putchar", putchar);

  auto handle = jit.compile(graph);
  graph->destroy();

  DIAG(eventFinish, "Build")

  DIAG(eventStart, "Run")

  char memory[4096] = {0};
  handle->entry(memory);

  DIAG(eventFinish, "Run")

  delete graph;
  delete diag;
}