#pragma once

#include <string>
#include <memory>

#include "tape_memory.h"

namespace BFVM {
  struct Config {
    Memory::Config memory;
    std::string dump;
    int cellWidth = 8;
    uint32_t eofValue = 0;
#ifndef NDIAG
    int profile = -1;
    bool quiet = false;
    bool dontRun = false;
#endif
  };

  // A public handle to a callable brainfuck function, compiled or otherwise
  struct Handle {
    virtual char* operator()(void*, char*) = 0;
    virtual ~Handle() = default;
  };

  struct Interpreter {
    virtual std::unique_ptr<BFVM::Handle> compile(const std::string &code, const std::string &name) = 0;
    virtual void run(BFVM::Handle &handle) = 0;
  protected:
    explicit Interpreter();
  };

  void run(const std::string &code, const Config &config = {});
}