#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "backend.h"


struct LLVMBackend {
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  llvm::Module module;

  LLVMBackend() :
    builder(context),
    module("bf", context) {}
};

void Backend::LLVM::compile(IR::Graph *graph) {
  LLVMBackend backend;
  backend.module.print(llvm::errs(), nullptr);
}
