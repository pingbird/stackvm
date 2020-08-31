#include <iostream>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/IndirectionUtils.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/TargetRegistry.h>

#include "backend_llvm.h"

Backend::LLVM::ModuleCompiler::ModuleCompiler(
  llvm::TargetMachine &machine,
  llvm::LLVMContext &context,
  llvm::Module &module
) :
  machine(machine),
  builder(context),
  context(context),
  module(module),
  functionPassManager(&module)
{
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
    "native_putchar",
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
    "code",
    module
  );
}

void Backend::LLVM::ModuleCompiler::optimize() {
  if (verifyModule(module, &llvm::errs())) abort();

  module.setTargetTriple(machine.getTargetTriple().str());
  module.setDataLayout(machine.createDataLayout());

  {
    llvm::PassManagerBuilder managerBuilder;
    managerBuilder.OptLevel = 2;
    managerBuilder.SizeLevel = 0;
    managerBuilder.Inliner = llvm::createFunctionInliningPass(2, 0, false);
    managerBuilder.LoopVectorize = true;
    managerBuilder.SLPVectorize = true;
    machine.adjustPassManager(managerBuilder);
    managerBuilder.populateFunctionPassManager(functionPassManager);
    managerBuilder.populateModulePassManager(passManager);
  }

  functionPassManager.doInitialization();
  for (llvm::Function &func : module) {
    if (verifyFunction(func, &llvm::errs())) abort();
    functionPassManager.run(func);
  }
  functionPassManager.doFinalization();

  passManager.add(llvm::createVerifierPass());
  passManager.run(module);
}

void Backend::LLVM::ModuleCompiler::callPutchar(int c) {
  builder.CreateCall(putcharFunction, {
    llvm::ConstantInt::get(intType, c)
  });
}

void Backend::LLVM::ModuleCompiler::compileGraph(IR::Graph *graph) {
  llvm::BasicBlock *block = llvm::BasicBlock::Create(context, "code", bfMainFunction);
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