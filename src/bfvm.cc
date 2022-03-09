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
        << "\r\n";
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
        << "\r\n";
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
    events[name] = Util::Time::getTime();
    if (!config.dump.empty()) {
      timeline
        << std::to_string(events[name])
        << ",start,"
        << Util::escapeCsvRow(name)
        << "\r\n";
    }
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
        << std::to_string(endTime)
        << ",finish,"
        << Util::escapeCsvRow(name)
        << "\r\n";
    }
    events.erase(name);
  }

  void artifact(const std::string &name, const DiagCollector &contents) override {
    if (config.dump.empty()) return;
    std::ofstream file = Util::openFile(config.dump + "/" + name, true);
	auto contentsStr = contents();
	file.write(contentsStr.data(), contentsStr.size());
    file.close();
  }
};
#endif

enum InputState {
  IS_NONE,
  IS_RECORDING,
  IS_READING,
};

struct IO;
int bfGetchar(IO *io);
void bfPutchar(IO *context, int x);

struct IO {
#ifndef NDIAG
  size_t inputIndex = 0;
  std::string inputRecording;
  std::string outputRecording;
  InputState inputState = IS_NONE;
#endif
  int eofValue = 0;
};

struct CompileContext {
  const BFVM::Config &config;

#ifndef NDIAG
  CommandLineDiag *diag = nullptr;
#endif

  std::unique_ptr<JIT::Pipeline> jit;

  explicit CompileContext(const BFVM::Config &config) : config(config) {
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
        diag->timeline = Util::openFile(config.dump + "/timeline.txt", false);
        diag->timeline << "Time,Event,Label" << "\r\n";
      }
    }
    DIAG(eventStart, "No-op baseline")
    DIAG(eventFinish, "No-op baseline")
#endif
  }

  std::unique_ptr<IR::Graph> buildGraph(const std::string &code) {
    DIAG(eventStart, "Parse")
    auto program = BF::Program::parse(code);
    DIAG(eventFinish, "Parse")

    DIAG_ARTIFACT("bf.txt", program.print())

    DIAG(eventStart, "Lower")
    auto graph = Lowering::buildProgram(config, program);
    graph->buildDominators();
    Opt::validate(*graph);
    DIAG(eventFinish, "Lower")

    DIAG_ARTIFACT("ir_unopt.txt", IR::printGraph(*graph))

    Opt::validate(*graph);
    DIAG(eventStart, "Optimize")
    Opt::resolveRegs(*graph);
    Opt::fold(*graph, Opt::standardFoldRules());
    DIAG(eventFinish, "Optimize")

    DIAG_ARTIFACT("ir.txt", IR::printGraph(*graph))
    Opt::validate(*graph);

    return graph;
  }

  std::unique_ptr<BFVM::Handle> compile(IR::Graph &graph, const std::string &name) {
    DIAG(event, "JIT Initialization")

    JIT::init();
    if (!jit) {
      jit = std::make_unique<JIT::Pipeline>(config);
      DIAG_FWD(*jit)
    }
    jit->addSymbol("bf_putchar", bfPutchar);
    jit->addSymbol("bf_getchar", bfGetchar);
    return jit->compile(graph, name);
  }

  void run(BFVM::Handle &handle) {
    IO io;
    io.eofValue = config.eofValue;
#ifndef NDIAG
    if (config.profile >= 0) {
      DIAG(log, "Doing " + std::to_string(config.profile) + " profile runs")
      if (config.profile != 0) {
        Memory::Tape tape(config.memory);
        io.inputState = IS_RECORDING;
        DIAG(eventStart, "Dry run")
        handle(&io, tape.start);
        DIAG(eventFinish, "Dry run")
        DIAG_ARTIFACT("input.txt", io.inputRecording)
        DIAG_ARTIFACT("output.txt", io.outputRecording)

        io.inputState = IS_READING;
        DIAG(eventStart, "Batch")
        for (int i = 0; i < config.profile; i++) {
          tape.clear();
          io.inputIndex = 0;
          handle(&io, tape.start);
        }
        DIAG(eventFinish, "Batch")
      }
    } else {
#endif
      DIAG(eventStart, "Run")
      Memory::Tape tape(config.memory);
      handle(&io, tape.start);
      DIAG(eventFinish, "Run")
#ifndef NDIAG
    }
#endif

#ifndef NDIAG
    if (!config.dump.empty()) {
      diag->timeline.close();
    }
    delete diag;
#endif
  }
} __attribute__((aligned(32)));

int bfGetchar(IO *io) {
#ifndef NDIAG
  switch (io->inputState) {
    case IS_READING:
      if (io->inputIndex == io->inputRecording.length()) {
        return io->eofValue;
      } else {
        return io->inputRecording[io->inputIndex++];
      }
    case IS_RECORDING: {
      int c = getchar();
      if (c == -1) {
        return io->eofValue;
      } else {
        io->inputRecording.push_back(c);
        return c;
      }
    } default: break;
  }
#endif
  int c = getchar();
  if (c == -1) {
    return io->eofValue;
  } else {
    return c;
  }
}

void bfPutchar(IO *context, int x) {
#ifndef NDIAG
  if (context->inputState == IS_READING) {
    return;
  } else if (context->inputState == IS_RECORDING) {
    context->outputRecording.push_back(x);
  }
#endif
  putchar(x);
}

struct InterpreterImpl : public BFVM::Interpreter {
  CompileContext context;

  explicit InterpreterImpl(
    const BFVM::Config &config
  ) : context(config) {}

  std::unique_ptr<BFVM::Handle> compile(const std::string &code, const std::string &name) override {
    auto graph = context.buildGraph(code);
    auto handle = context.compile(*graph, name);
    graph->destroy();
    return handle;
  }

  void run(BFVM::Handle &handle) override {
    context.run(handle);
  }
};

BFVM::Interpreter::Interpreter() = default;

void BFVM::run(const std::string &code, const BFVM::Config &config) {
  InterpreterImpl interpreter(config);
  auto handle = interpreter.compile(code, "code");
  interpreter.run(*handle);
}
