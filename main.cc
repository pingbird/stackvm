#include <iostream>
#include <string>

#include "src/bfvm.h"

int main(int argv, char** argc) {
  BFVM::Config config;
  config.enableArtifacts = true;
  BFVM::run("++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.", config);
}
