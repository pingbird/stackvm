#pragma once

#include <string>
#include <functional>
#include <fstream>

#ifndef NDIAG

using DiagCollector = std::function<std::string()>;

struct Diag {
  virtual void log(const std::string& string) {}
  virtual void event(const std::string& name) {}
  virtual void eventStart(const std::string& name) {}
  virtual void eventFinish(const std::string& name) {}
  virtual void artifact(const std::string& name, const DiagCollector &contents) {}
};

#define DIAG(m, ...) if (diag) diag->m(__VA_ARGS__);
#define DIAG_FWD(x) if (diag) (x).diag = diag;
#define DIAG_DECL() Diag* diag = nullptr;
#define DIAG_ARTIFACT(n, c) DIAG(artifact, n, [&]() { return c; })

#else

#define DIAG(m, ...)
#define DIAG_FWD(x)
#define DIAG_DECL()
#define DIAG_ARTIFACT(n, c)

#endif

namespace Util {
  namespace Time {
    const uint64_t nanosecond = 1;
    const uint64_t microsecond = nanosecond * 1000;
    const uint64_t millisecond = microsecond * 1000;
    const uint64_t second = millisecond * 1000;

    int64_t getTime();
    std::string printTime(int64_t ns, int64_t precision = second);
  }

  std::ofstream openFile(const std::string &path);

  std::string escapeCsvRow(std::string str);
}