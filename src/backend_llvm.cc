#include <iostream>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/IndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
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
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/Host.h"

#include "backend.h"

struct LLVMBackend {
  llvm::TargetMachine *machine;
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  llvm::Module module;
  llvm::legacy::PassManager passManager;
  llvm::legacy::FunctionPassManager functionPassManager;

  llvm::Type *intType;
  llvm::Type *voidType;

  llvm::FunctionType *putcharType;
  llvm::Function *putcharFunction;

  llvm::FunctionType *bfMainType;
  llvm::Function *bfMainFunction;

  explicit LLVMBackend(llvm::TargetMachine *machine) :
    machine(machine),
    builder(context),
    module("bf", context),
    functionPassManager(&module) {
    intType = llvm::Type::getInt32Ty(context);
    voidType = llvm::Type::getVoidTy(context);

    putcharType = llvm::FunctionType::get(
      intType,
      {intType},
      false
    );

    putcharFunction = llvm::Function::Create(
      putcharType,
      llvm::Function::ExternalLinkage,
      "putchar",
      module
    );

    bfMainType = llvm::FunctionType::get(
      voidType,
      {},
      false
    );

    bfMainFunction = llvm::Function::Create(
      bfMainType,
      llvm::Function::ExternalLinkage,
      "bf_main",
      module
    );
  }

  void callPutchar(int c) {
    builder.CreateCall(putcharFunction, {
      llvm::ConstantInt::get(intType, c)
    });
  }

  void addOptPasses() {
    llvm::PassManagerBuilder managerBuilder;
    managerBuilder.OptLevel = 2;
    managerBuilder.SizeLevel = 0;
    managerBuilder.Inliner = llvm::createFunctionInliningPass(3, 0, false);
    managerBuilder.LoopVectorize = true;
    managerBuilder.SLPVectorize = true;
    machine->adjustPassManager(managerBuilder);
    managerBuilder.populateFunctionPassManager(functionPassManager);
    managerBuilder.populateModulePassManager(passManager);
  }

  void addLinkPasses() {
    llvm::PassManagerBuilder managerBuilder;
    managerBuilder.VerifyInput = true;
    managerBuilder.Inliner = llvm::createFunctionInliningPass(3, 0, false);
    managerBuilder.populateLTOPassManager(passManager);
  }

  void optimize() {
    if (verifyModule(module, &llvm::errs())) abort();

    module.setTargetTriple(machine->getTargetTriple().str());
    module.setDataLayout(machine->createDataLayout());

    passManager.add(new llvm::TargetLibraryInfoWrapperPass(machine->getTargetTriple()));
    passManager.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));
    functionPassManager.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));

    addOptPasses();
    addLinkPasses();

    functionPassManager.doInitialization();
    for (llvm::Function &func : module) {
      if (verifyFunction(func, &llvm::errs())) abort();
      functionPassManager.run(func);
    }
    functionPassManager.doFinalization();

    passManager.add(llvm::createVerifierPass());
    passManager.run(module);
  }

  void helloWorld() {
    llvm::BasicBlock *block = llvm::BasicBlock::Create(context, "entry", bfMainFunction);
    builder.SetInsertPoint(block);

    const char *hello = "Hello, World!\n";
    while (*hello) {
      callPutchar(*hello);
      hello++;
    }

    builder.CreateRetVoid();

    optimize();

    module.print(llvm::errs(), nullptr);
  }
};

void Backend::LLVM::compile(IR::Graph *graph) {
  LLVMInitializeNativeTarget();
  LLVMBackend backend(llvm::EngineBuilder().selectTarget());
  backend.helloWorld();
}
