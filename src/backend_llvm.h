#pragma once

#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>

#include "ir.h"
#include "diagnostics.h"

namespace Backend::LLVM {
  template<typename T>
  std::string printRaw(T &value) {
    std::string out;
    llvm::raw_string_ostream stream(out);
    stream << value;
    stream.flush();
    return out;
  }

  struct ModuleCompiler {
    llvm::TargetMachine &machine;
    llvm::LLVMContext &context;
    llvm::IRBuilder<> builder;
    llvm::Module &module;
    llvm::legacy::PassManager passManager;
    llvm::legacy::FunctionPassManager functionPassManager;

    llvm::Type *addrType;
    llvm::Type *intType;
    llvm::Type *voidType;
    llvm::Type *voidPtrType;
    llvm::Type *wordType;
    llvm::Type *wordPtrType;
    llvm::Type *boolType;

    llvm::FunctionType *putcharType;
    llvm::Function *putcharFunction;

    llvm::FunctionType *getcharType;
    llvm::Function *getcharFunction;

    llvm::FunctionType *bfMainType;
    llvm::Function *bfMainFunction;

    std::vector<IR::Inst*> pendingPhis;

    DIAG_DECL()

    explicit ModuleCompiler(
      llvm::TargetMachine &machine,
      llvm::LLVMContext &context,
      llvm::Module &module
    );

    void optimize();

    llvm::Type *convertType(IR::TypeId typeId);

    void compileGraph(IR::Graph *graph);
    void compileBlock(IR::Block *block);
    llvm::Value *compileInst(IR::Inst *inst);
    llvm::Value *getValue(IR::Inst *inst, llvm::Type *type = nullptr);
  };
}