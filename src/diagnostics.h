#pragma once

#include <string>
#include <functional>

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
#define DIAG_FWD(x) if (diag) (x)->diag = diag;
#define DIAG_DECL() Diag* diag = nullptr;
#define DIAG_ARTIFACT(n, c) DIAG(artifact, n, [&]() { return c; })

#else

#define DIAG(m, ...)
#define DIAG_FWD(x)
#define DIAG_DECL()

#endif