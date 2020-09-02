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
    .doc_column(29);

  std::cerr << "Usage:\n" << usage_lines(cli, "stackvm", format) << std::endl;
  std::cerr << "Parameters:\n" << documentation(cli, format) << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  BFVM::Config config;

  bool help = false;
  std::string program;
  std::vector<std::string> invalid;
  std::string tapeLeft;
  std::string tapeRight;

  auto cli = (
    option("-h", "--help").set(help) % "print this help message",
    (option("-w", "--width") & value("bits", config.cellWidth)) % "width of cells in bits, default = 8",
    (option("-e", "--eof") & value("value", config.cellWidth)) % "value of getchar when eof is reached, default = 0",
    (option("-l", "--tape-left") & value("size", tapeLeft)) % "how much virtual memory to reserve to the left, default = 128MiB",
    (option("-r", "--tape-right") & value("size", tapeRight)) % "how much virtual memory to reserve to the right, default = 128MiB",
#ifndef NDIAG
    option("-p", "--profile").set(config.profile) % "enable profiling of build and execution",
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

  if (!tapeLeft.empty()) {
    config.memory.sizeLeft = Memory::parseSize(tapeLeft);
  }

  if (!tapeRight.empty()) {
    config.memory.sizeRight = Memory::parseSize(tapeRight);
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
