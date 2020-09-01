#pragma once

#include <cstddef>

extern "C" {
  #include <sys/mman.h>
}

namespace Memory {
  const size_t b = 1;
  const size_t kib = b * 1024;
  const size_t mib = b * 1024;
  const size_t gib = b * 1024;

  struct Config {
    size_t sizeLeft = gib / 2;
    size_t sizeRight = gib / 2;
  };

  struct Tape {
    const Config &config;
    char* base = nullptr;
    char* start = nullptr;
    explicit Tape(const Config &config);
  };
}