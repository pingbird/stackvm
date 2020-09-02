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
#include "memory.h"

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
    if (!config.dump.empty()) {
      std::ofstream ofs(config.dump + "/" + name);
      ofs << contents();
      ofs.close();
    }
  }
};
#endif

int nativeGetchar(int closed) {
  int c = getchar();
  return c == -1 ? closed : c;
}

void nativePutchar(int x) {
  putchar(x);
}

void BFVM::run(const std::string &code, const Config &config) {
#ifndef NDIAG
  CommandLineDiag *diag = nullptr;

  if (config.profile || !config.dump.empty()) {
    diag = new CommandLineDiag(config);
    if (
      !config.dump.empty() &&
      !std::filesystem::exists(config.dump) &&
      !std::filesystem::create_directory(config.dump)
    ) {
      std::cerr << "Error: Failed to create output directory \"" + config.dump + "\"";
      exit(1);
    }
  }
#endif
  DIAG(eventStart, "No-op baseline")
  DIAG(eventFinish, "No-op baseline")

  DIAG(eventStart, "Build")

  DIAG(eventStart, "Parse")
  auto program = BF::parse(code);
  DIAG(eventFinish, "Parse")

  DIAG_ARTIFACT("bf.txt", BF::print(program))

  DIAG(eventStart, "Lower")
  auto graph = Lowering::buildProgram(config, &program);
  Opt::validate(graph);
  DIAG(eventFinish, "Lower")

  DIAG_ARTIFACT("ir_unopt.txt", IR::printGraph(graph))

  DIAG(eventStart, "Optimize")
  Opt::resolveRegs(graph);
  Opt::validate(graph);
  DIAG(eventFinish, "Optimize")

  DIAG_ARTIFACT("ir.txt", IR::printGraph(graph))

  DIAG(event, "JIT Initialization")

  JIT::init();
  JIT::Pipeline jit(config);
  DIAG_FWD(jit)
  jit.addSymbol("native_putchar", nativePutchar);
  jit.addSymbol("native_getchar", nativeGetchar);

  auto handle = jit.compile(graph);
  graph->destroy();

  DIAG(eventFinish, "Build")

  DIAG(eventStart, "Run")

  Memory::Config tapeConfig;
  Memory::Tape tape(tapeConfig);

  handle->entry(static_cast<char*>(tape.start));

  DIAG(eventFinish, "Run")

  delete graph;
  delete diag;
}