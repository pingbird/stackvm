#include "memory.h"

Memory::Tape::Tape(const Config &config) : config(config) {
  base = static_cast<char*>(mmap(
    nullptr,
    config.sizeLeft + config.sizeRight,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
    -1,
    0
  ));
  start = base + config.sizeLeft;
}
