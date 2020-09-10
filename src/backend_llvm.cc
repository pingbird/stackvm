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
#include "opt.h"

Backend::LLVM::ModuleCompiler::ModuleCompiler(
  const BFVM::Config &config,
  llvm::TargetMachine &machine,
  llvm::LLVMContext &context,
  llvm::Module &module
) :
  config(config),
  machine(machine),
  builder(context),
  context(context),
  module(module),
  functionPassManager(&module),
  regValues{nullptr}
{
  intType = llvm::Type::getInt32Ty(context);
  voidType = llvm::Type::getVoidTy(context);

  auto contextType = llvm::StructType::get(context);
  contextType->setName("Context");
  contextPtrType = contextType->getPointerTo();

  sizeType = llvm::IntegerType::getInt64Ty(context);

  cellType = convertType(IR::typeForWidth(config.cellWidth));
  cellPtrType = cellType->getPointerTo();

  putcharType = llvm::FunctionType::get(
    voidType,
    {contextPtrType, intType},
    false
  );

  putcharFunction = llvm::Function::Create(
    putcharType,
    llvm::Function::ExternalLinkage,
    "bf_putchar",
    module
  );

  getcharType = llvm::FunctionType::get(
    intType,
    {contextPtrType},
    false
  );

  getcharFunction = llvm::Function::Create(
    getcharType,
    llvm::Function::ExternalLinkage,
    "bf_getchar",
    module
  );

  fragmentType = llvm::FunctionType::get(
    cellPtrType,
    {contextPtrType, cellPtrType},
    false
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

void Backend::LLVM::ModuleCompiler::compileGraph(IR::Graph &graph, const std::string &name) {
  graph.clearPassData();

  DIAG(eventStart, "Translate")

  auto fragmentFunction = llvm::Function::Create(
    fragmentType,
    llvm::Function::ExternalLinkage,
    name,
    module
  );
  fragmentFunction->addAttribute(2, llvm::Attribute::NoAlias);

  int numBlocks = graph.blocks.size();
  for (int b = 0; b < numBlocks; b++) {
    IR::Block *block = graph.blocks[b];
    llvm::BasicBlock *newBlock = llvm::BasicBlock::Create(
      context,
      block->getLabel(),
      fragmentFunction
    );
    block->passData = static_cast<void*>(newBlock);

    if (b == 0) {
      builder.SetInsertPoint(newBlock);
      for (int reg = 0; reg < IR::NUM_REGS; reg++) {
        switch ((IR::RegKind) reg) {
          case IR::R_PTR:
            builder.CreateStore(
              fragmentFunction->getArg(1),
              regValues[reg] = builder.CreateAlloca(cellPtrType)
            );
            break;
        }
      }
    }
  }

  for (int b = 0; b < numBlocks; b++) {
    compileBlock(*graph.blocks[b]);
  }

  for (IR::Inst *inst : pendingPhis) {
    auto phi = static_cast<llvm::PHINode*>(inst->passData);
    for (int i = 0; i < inst->inputs.size(); i++) {
      auto block = static_cast<llvm::BasicBlock*>(inst->block->predecessors[i]->passData);
      auto value = static_cast<llvm::Value*>(inst->inputs[i]->passData);
      if (value->getType() != phi->getType()) {
        auto inputInst = inst->inputs[i];
        IR::TypeId phiType = Opt::resolveType(inst);
        IR::TypeId inputType = Opt::resolveType(inputInst);
#ifndef NDEBUG
        if (phiType != IR::maxType(phiType, inputType)) {
          std::cerr << "Phi input has wrong type." << std::endl;
          std::cerr << "input: %" + std::to_string(inst->inputs[i]->id) << std::endl;
          std::cerr << "type: " << printRaw(*value->getType()) << std::endl;
          std::cerr << "expected: " << printRaw(*phi->getType()) << std::endl;
          abort();
        }
#endif
        auto terminator = block->getTerminator();
        assert(terminator);
        builder.SetInsertPoint(terminator);
        value = builder.CreateIntCast(value, phi->getType(), false);
      }
      phi->addIncoming(value, block);
    }
  }

  DIAG(eventFinish, "Translate")

  DIAG_ARTIFACT("llvm_ir_unopt.ll", printRaw(module))
  DIAG(eventStart, "Optimize LLVM")

  // optimize();

  DIAG(eventFinish, "Optimize LLVM")
  DIAG_ARTIFACT("llvm_ir_opt.ll", printRaw(module))
}

void Backend::LLVM::ModuleCompiler::compileBlock(IR::Block &block) {
  builder.SetInsertPoint(static_cast<llvm::BasicBlock*>(block.passData));
  IR::Inst *inst = block.first;
  while (inst) {
    auto value = compileInst(inst);
    inst->passData = value;
    inst = inst->next;
  }
}

llvm::Value *Backend::LLVM::ModuleCompiler::compileInst(IR::Inst *inst) {
  switch (inst->kind) {
    case IR::I_REG:
      return builder.CreateLoad(regValues[inst->immReg]);
    case IR::I_SETREG:
      return builder.CreateStore(
        getValue(inst->inputs[0]),
        regValues[inst->immReg]
      );
    case IR::I_NOP:
    case IR::I_IMM: {
      auto type = Opt::resolveType(inst);
      auto out = llvm::ConstantInt::get(
        convertType(type),
        inst->immValue,
        type == IR::T_SIZE
      );
      return out;
    } case IR::I_ADD: {
      auto type = convertType(Opt::resolveType(inst));
      return builder.CreateAdd(
        getValue(inst->inputs[0], type),
        getValue(inst->inputs[1], type)
      );
    } case IR::I_SUB: {
      auto type = convertType(Opt::resolveType(inst));
      return builder.CreateSub(
        getValue(inst->inputs[0], type),
        getValue(inst->inputs[1], type)
      );
    } case IR::I_GEP:
      return builder.CreateGEP(
        getValue(inst->inputs[0]),
        getValue(inst->inputs[1])
      );
    case IR::I_LD:
      return builder.CreateLoad(
        cellType,
        getValue(inst->inputs[0])
      );
    case IR::I_STR:
      return builder.CreateStore(
        getValue(inst->inputs[1]),
        getValue(inst->inputs[0])
      );
    case IR::I_PUTCHAR:
      return builder.CreateCall(putcharFunction, {
        builder.GetInsertBlock()->getParent()->args().begin(),
        getValue(inst->inputs[0], intType)
      });
    case IR::I_GETCHAR:
      return builder.CreateIntCast(
        builder.CreateCall(getcharFunction, {
          builder.GetInsertBlock()->getParent()->args().begin()
        }),
        cellType,
        false
      );
    case IR::I_PHI: {
      pendingPhis.push_back(inst);
      auto typeId = Opt::resolveType(inst);
      assert(typeId != IR::T_NONE);
      auto block = builder.GetInsertBlock();
      if (!block->empty()) {
        builder.SetInsertPoint(&block->getInstList().front());
      }
      auto phi = builder.CreatePHI(
        convertType(typeId),
        inst->inputs.size()
      );
      builder.SetInsertPoint(block);
      return phi;
    } case IR::I_IF:
      return builder.CreateCondBr(
        builder.CreateICmpNE(
          getValue(inst->inputs[0], cellType),
          llvm::ConstantInt::get(cellType, 0)
        ),
        static_cast<llvm::BasicBlock*>(inst->block->successors[0]->passData),
        static_cast<llvm::BasicBlock*>(inst->block->successors[1]->passData)
      );
    case IR::I_GOTO:
      return builder.CreateBr(
        static_cast<llvm::BasicBlock*>(inst->block->successors[0]->passData)
      );
    case IR::I_RET:
      return builder.CreateRet(getValue(inst->inputs[0], cellPtrType));
    case IR::I_TAG:
      abort();
  }
  abort();
}

llvm::Value *Backend::LLVM::ModuleCompiler::getValue(IR::Inst *inst, llvm::Type *type) {
  auto value = static_cast<llvm::Value*>(inst->passData);
  if (value == nullptr) {
    compileBlock(*inst->block);
    value = static_cast<llvm::Value*>(inst->passData);
    assert(value != nullptr);
  }
  if (type != nullptr && value->getType() != type) {
    value = builder.CreateIntCast(value, type, false);
  }
  return value;
}

llvm::Type *Backend::LLVM::ModuleCompiler::convertType(IR::TypeId typeId) {
  switch (typeId) {
    case IR::T_NONE:
      return voidType;
    case IR::T_PTR:
      return cellPtrType;
    case IR::T_SIZE:
      return sizeType;
    case IR::T_I8:
      return llvm::Type::getInt8Ty(context);
    case IR::T_I16:
      return llvm::Type::getInt16Ty(context);
    case IR::T_I32:
      return llvm::Type::getInt32Ty(context);
    case IR::T_I64:
      return llvm::Type::getInt64Ty(context);
    default:
      abort();
  }
}
