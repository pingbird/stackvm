#include <unistd.h>
#include <zconf.h>
#include <cstring>
#include "tape_memory.h"

Memory::Tape::Tape(const Config &config) : config(config) {
  size_t pageSize = sysconf(_SC_PAGESIZE);
  small = config.sizeLeft + config.sizeRight <= mmapThreshold;
  if (small) {
    totalSize = config.sizeLeft + config.sizeRight;
    base = static_cast<char*>(calloc(totalSize, 1));
    if (base == nullptr) {
      std::cerr << "Error: calloc failed, out of memory?" << std::endl;
      exit(1);
    }
  } else {
    totalSize = ((config.sizeLeft + config.sizeRight) + (pageSize - 1)) & ~(pageSize - 1);

    base = static_cast<char*>(mmap(
      nullptr,
      totalSize,
      PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
      -1,
      0
    ));

    if (base == MAP_FAILED) {
      std::cerr << "Error: mmap failed (" << strerror(errno) << ")" << std::endl;
      exit(1);
    }
  }
  start = base + config.sizeLeft;
}

Memory::Tape::~Tape() {
  if (small) {
    free(base);
  } else {
    if (munmap(base, totalSize) != 0) {
      std::cerr << "Error: munmap failed (" << strerror(errno) << ")" << std::endl;
      exit(1);
    }
  }
}

void Memory::Tape::clear() {
  if (small) {
    memset(base, 0, totalSize);
  } else {
    auto result = static_cast<char*>(mmap(
      base,
      totalSize,
      PROT_READ | PROT_WRITE,
      MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
      -1,
      0
    ));

    if (result != base) {
      std::cerr << "Error: mmap failed (" << std::strerror(errno) << ")" << std::endl;
      exit(1);
    }
  }
}

bool strEquals(const std::string &x, const std::string &y) {
  unsigned int length = x.size();
  if (y.size() != length) {
    return false;
  }
  for (unsigned int i = 0; i < length; i++) {
    if (std::tolower(x[i]) != std::tolower(y[i])) {
      return false;
    }
  }
  return true;
}

bool Memory::tryParseSize(const std::string &str, size_t &size, std::string &err) {
  const char *start = str.c_str();
  char *end = nullptr;
  double value = std::strtod(start, &end);
  if (start == end) {
    err = "Could not parse size \"" + str + "\"";
    return false;
  } else if (value < 0) {
    err = "Size cannot be less than zero \"" + str + "\"";
    return false;
  }

  if (end == start + str.size()) {
    size = value;
    return true;
  }

  auto tail = std::string(end);
  auto suffix = &sizes[0];
  while (suffix->str != nullptr) {
    if (strEquals(suffix->str, tail)) {
      size = value * suffix->size;
      return true;
    }
    suffix++;
  }

  err = "Unknown suffix \"" + tail + "\"";
  return false;
}

size_t Memory::parseSize(const std::string &str) {
  size_t size;
  std::string err;
  if (tryParseSize(str, size, err)) {
    return size;
  } else {
    std::cerr << "Invalid argument: " << err << std::endl;
    exit(1);
  }
}