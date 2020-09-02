#pragma once

#include <cstdlib>
#include <string>
#include <stdexcept>
#include <iostream>

extern "C" {
  #include <sys/mman.h>
}

namespace Memory {
  const size_t b = 1;
  const size_t kb = b * 1000;
  const size_t mb = kb * 1000;
  const size_t gb = mb * 1000;
  const size_t kib = b * 1024;
  const size_t mib = kib * 1024;
  const size_t gib = mib * 1024;

  struct {
    const char *str;
    size_t size;
  } sizes[] = {
    {"b", b},
    {"kb", kb},
    {"mb", mb},
    {"gb", gb},
    {"kib", kib},
    {"mib", mib},
    {"gib", gib},
    {nullptr, 0}
  };

  bool tryParseSize(const std::string &str, size_t &size, std::string &err);

  size_t parseSize(const std::string &str);

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