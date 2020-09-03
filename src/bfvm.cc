#include <iostream>
#include <filesystem>

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
  std::ofstream timeline;

  explicit CommandLineDiag(const BFVM::Config &config) : config(config) {}

  void log(const std::string &string) override {
    if (!config.profile) return;
    if (!config.quiet) {
      std::cerr << "[ log    ] " << string << std::endl;
    }
    if (!config.dump.empty()) {
      timeline
        << std::to_string(Util::Time::getTime())
        << ",log,"
        << Util::escapeCsvRow(string)
        << std::endl;
    }
  }

  void event(const std::string &name) override {
    if (!config.profile) return;
    if (!config.quiet) {
      std::cerr << "[ event  ] " << name << std::endl;
    }
    if (!config.dump.empty()) {
      timeline
        << std::to_string(Util::Time::getTime())
        << ",event,"
        << Util::escapeCsvRow(name)
        << std::endl;
    }
  }

  void eventStart(const std::string &name) override {
    if (!config.profile) return;
    if (events.count(name)) {
      abort();
    }
    if (!config.quiet) {
      std::cerr
        << "[ start  ] "
        << std::string(events.size() + 1, '>')
        << " " << name << std::endl;
    }
    if (!config.dump.empty()) {
      timeline
        << std::to_string(Util::Time::getTime())
        << ",start,"
        << Util::escapeCsvRow(name)
        << std::endl;
    }
    events[name] = Util::Time::getTime();
  }

  void eventFinish(const std::string &name) override {
    if (!config.profile) return;
    int64_t endTime = Util::Time::getTime();
    if (!events.count(name)) {
      abort();
    }
    if (!config.quiet) {
      std::cerr
        << "[ finish ] "
        << std::string(events.size(), '<')
        << " " << name << " - "
        << Util::Time::printTime(endTime - events[name])
        << std::endl;
    }
    if (!config.dump.empty()) {
      timeline
        << std::to_string(Util::Time::getTime())
        << ",finish,"
        << Util::escapeCsvRow(name)
        << std::endl;
    }
    events.erase(name);
  }

  void artifact(const std::string &name, const DiagCollector &contents) override {
    if (config.dump.empty()) return;
    std::ofstream file = Util::openFile(config.dump + "/" + name);
    file << contents();
    file.close();
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

struct Context {
#ifndef NDIAG
  CommandLineDiag *diag = nullptr;
#endif
  const BFVM::Config &config;

  explicit Context(
    const BFVM::Config &config
  ) : config(config) {}

  void run(const std::string &code) {
#ifndef NDIAG
    if (config.profile || !config.dump.empty()) {
      diag = new CommandLineDiag(config);
      if (!config.dump.empty()) {
        if (
          !std::filesystem::exists(config.dump) &&
          !std::filesystem::create_directory(config.dump)
        ) {
          std::cerr << "Error: Failed to create output directory \"" + config.dump + "\"";
          exit(1);
        }
        diag->timeline = Util::openFile(config.dump + "/timeline.txt");
        diag->timeline << "Time,Event,Label" << std::endl;
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

    Memory::Tape tape(config.memory);

    handle->entry(static_cast<char*>(tape.start));

    DIAG(eventFinish, "Run")

#ifndef NDIAG
    if (!config.dump.empty()) {
      diag->timeline.close();
    }
#endif

    delete graph;
    delete diag;
  }
};

void BFVM::run(const std::string &code, const Config &config) {
  Context(config).run(code);
}