#include <iostream>
#include <ctime>
#include <chrono>
#include <sstream>

#include "diagnostics.h"

using std::to_string;

int64_t Time::getTime() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::high_resolution_clock::now() - std::chrono::time_point<std::chrono::high_resolution_clock>()
  ).count();
}

static std::locale getLocale() {
  std::locale locale("");
  if (locale.name() == "C.UTF-8") {
    return std::locale("en_US.UTF-8");
  }
  return locale;
}

static std::string prettyPrint(int64_t value) {
  std::stringstream stream;
  stream.imbue(getLocale());
  stream << std::fixed << value;
  return stream.str();
}

std::string Time::printTime(int64_t time, int64_t precision) {
  if (time < microsecond * 10 || precision <= nanosecond) {
    return prettyPrint(time / nanosecond) + " ns";
  } else if (time < millisecond * 10 || precision <= microsecond) {
    return prettyPrint(time / microsecond) + " μs";
  } else if (time < second * 10 || precision <= millisecond) {
    return prettyPrint(time / millisecond) + " ms";
  } else {
    return prettyPrint(time / second) + " s";
  }
}
