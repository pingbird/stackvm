#include <iostream>
#include <ctime>
#include <chrono>
#include <sstream>

#include "diagnostics.h"

using std::to_string;

static int64_t startTime = 0;

int64_t Util::Time::getTime() {
  int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::high_resolution_clock::now() - std::chrono::time_point<std::chrono::high_resolution_clock>()
  ).count();
  if (startTime == 0) {
    startTime = time;
    return 0;
  } else {
    return time - startTime;
  }
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

std::string Util::Time::printTime(int64_t time, int64_t precision) {
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

std::ofstream Util::openFile(const std::string &path, bool binary) {
  std::ofstream file;
  auto mode = std::ios_base::out;
  if (binary) {
    mode |= std::ios_base::binary;
  }
  file.open(path, mode);
  if (!file.is_open()) {
    std::cerr << std::system_error(
      errno, std::system_category(), "Error: Failed to open \"" + path + "\""
    ).what() << std::endl;
    std::exit(1);
  }
  return file;
}

static void replaceAll(
  std::string &str,
  const std::string &from,
  const std::string &to
) {
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::string::npos) {
    str.replace(pos, from.size(), to);
    pos += from.size();
  }
}

std::string Util::escapeCsvRow(std::string str) {
  if (
    str.find(',') != std::string::npos ||
    str.find('\"') != std::string::npos ||
    str.find(' ') != std::string::npos
  ) {
    replaceAll(str, "\"", "\\\"");
    return "\"" + str + "\"";
  }
  return str;
}
