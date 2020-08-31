#pragma once

#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>

namespace Backend::LLVM {
  struct ModuleCompiler {
    llvm::TargetMachine &machine;
    llvm::LLVMContext &context;
    llvm::IRBuilder<> builder;
    llvm::Module &module;
    llvm::legacy::PassManager passManager;
    llvm::legacy::FunctionPassManager functionPassManager;

    llvm::Type *intType;
    llvm::Type *voidType;

    llvm::FunctionType *putcharType;
    llvm::Function *putcharFunction;

    llvm::FunctionType *bfMainType;
    llvm::Function *bfMainFunction;

    explicit ModuleCompiler(
      llvm::TargetMachine &machine,
      llvm::LLVMContext &context,
      llvm::Module &module
    );

    void addOptPasses();
    void optimize();
    void callPutchar(int c);
    void helloWorld();
  };
}