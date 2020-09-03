#include <iostream>
#include <string>
#include <filesystem>
#include <clipp.h>
#include <fstream>

#include "src/bfvm.h"

using namespace clipp;

void printUsageAndExit(group &cli) {
  auto format = doc_formatting{}
    .split_alternatives(false)
    .indent_size(4)
    .first_column(4)
    .doc_column(27);

  std::cerr << "Usage:\n" << usage_lines(cli, "stackvm", format) << std::endl;
  std::cerr << "Parameters:\n" << documentation(cli, format) << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  BFVM::Config config;

  bool help = false;
  std::string program;
  std::vector<std::string> invalid;
  std::string memory;

  auto cli = (
    option("-h", "--help").set(help) % "print this help message",
    (option("-w", "--width") & value("bits", config.cellWidth)) % "width of cells in bits\ndefault = 8",
    (option("-e", "--eof") & value("value", config.cellWidth)) % "value of getchar when eof is reached\ndefault = 0",
    (option("-m", "--memory") & value("size", memory)) % "how much virtual memory to reserve to the left and right\ndefault = 128MiB,128MiB",
#ifndef NDIAG
    (option("-p", "--profile") & value("count", config.profile)) % "enable profiling of build and execution",
    option("-q", "--quiet").set(config.quiet) % "suppress printing profiling info to the console",
    (option("-d", "--dump") & value("dir", config.dump)) % "dumps intermediates into the specified folder",
#endif
    value("program").set(program)
  );

  auto res = parse(argc, argv, cli);

  if (!res.any_error() && program[0] == '-') {
    invalid.push_back(program);
  }

  if (help || res.any_error() || !invalid.empty()) {
    for (auto &arg : invalid) {
      std::cerr << "Error: Unrecognized argument \"" << arg << "\"" << std::endl;
    }
    printUsageAndExit(cli);
  }

  if (!memory.empty()) {
    auto delimiter = memory.find(',');
    if (delimiter == std::string::npos) {
      size_t size = Memory::parseSize(memory);
      config.memory.sizeLeft = size;
      config.memory.sizeRight = size;
    } else {
      config.memory.sizeLeft = Memory::parseSize(memory.substr(0, delimiter));
      config.memory.sizeRight = Memory::parseSize(memory.substr(delimiter + 1));
    }
  }

  std::ifstream input(program);
  if (!input.is_open()) {
    std::cerr << std::system_error(
      errno, std::system_category(), "Error: Failed to open \"" + program + "\""
    ).what() << std::endl;
    exit(1);
  }

  std::stringstream contents;
  contents << input.rdbuf();
  BFVM::run(contents.str(), config);
}
