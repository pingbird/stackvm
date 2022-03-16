#include "jit.h"

using std::unique_ptr;

void JIT::init() {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
}

JIT::Linker::Linker(
  const BFVM::Config &config,
  llvm::TargetMachine &machine,
  llvm::LLVMContext &context
) :
  config(config),
  machine(machine),
  dataLayout(machine.createDataLayout()),
  context(context),
  resolver(createLegacyLookupResolver(
    session,
    [this](llvm::StringRef nameRef) -> llvm::JITSymbol {
      auto name = nameRef.str();
      if (auto symbol = objectLayer.findSymbol(std::string(name), false)) {
        return symbol;
      } else if (auto error = symbol.takeError()) {
        return std::move(error);
      } if (auto address = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)) {
        return {address, llvm::JITSymbolFlags::Exported};
      } else if (symbols.count(name)) {
        return {symbols[name], llvm::JITSymbolFlags::Exported};
      } else {
        return {nullptr};
      }
    },
    [](llvm::Error error) {
      cantFail(std::move(error), "lookupFlags failed");
    })
  ),
  objectLayer(
    llvm::AcknowledgeORCv1Deprecation,
    session,
    [this](llvm::orc::VModuleKey key) {
      return llvm::orc::LegacyRTDyldObjectLinkingLayer::Resources{
        std::make_shared<llvm::SectionMemoryManager>(), resolver
      };
    }
  ),
  compileLayer(
    llvm::AcknowledgeORCv1Deprecation,
    objectLayer,
    machine
  ) {}

std::string JIT::Linker::mangle(const std::string &name) {
  std::string mangled;
  llvm::raw_string_ostream stream(mangled);
  llvm::Mangler::getNameWithPrefix(stream, name, dataLayout);
  return stream.str();
}

llvm::orc::VModuleKey JIT::Linker::addModule(unique_ptr<llvm::Module> module) {
  DIAG(eventStart, "Compile")

  auto key = session.allocateVModule();
  cantFail(compileLayer.addModule(key, std::move(module)));

  DIAG(eventFinish, "Compile")

  return key;
}

void JIT::Linker::removeModule(llvm::orc::VModuleKey key) {
  cantFail(compileLayer.removeModule(key));
  session.releaseVModule(key);
}

JIT::EntryFn JIT::Linker::findEntry(const std::string &name) {
  auto symbol = compileLayer.findSymbol(mangle(name), true);
  assert(symbol);
  auto entryAddr = symbol.getAddress();
  llvm::JITTargetAddress entryAddress = cantFail(std::move(entryAddr), "Could not get address");
  return llvm::jitTargetAddressToFunction<EntryFn>(entryAddress);
}

JIT::Pipeline::Pipeline(const BFVM::Config &config) :
  config(config),
  machine(llvm::EngineBuilder().selectTarget()),
  linker(config, *machine, context) {}

std::unique_ptr<BFVM::Handle> JIT::Pipeline::compile(IR::Graph &graph, const std::string &name) {
  auto module = std::make_unique<llvm::Module>("jit", context);
  Backend::LLVM::ModuleCompiler moduleCompiler(config, *machine, context, *module);
  DIAG_FWD(moduleCompiler)
  moduleCompiler.compileGraph(graph, name);
  DIAG_FWD(linker)

#ifndef NDIAG
  if (diag && diag->isDumping()) {
    llvm::legacy::PassManager pass;
    llvm::SmallString<0> jitModuleAsm;
    llvm::raw_svector_ostream jitModuleAsmStream(jitModuleAsm);
    if (machine->addPassesToEmitFile(pass, jitModuleAsmStream, nullptr, llvm::CGFT_AssemblyFile)) {
      std::cerr << "llvm::TargetMachine.addPassesToEmitFile failed";
      std::exit(1);
    }
    llvm::SmallString<0> jitModuleObject;
    llvm::raw_svector_ostream jitModuleObjectStream(jitModuleObject);
    if (machine->addPassesToEmitFile(pass, jitModuleObjectStream, nullptr, llvm::CGFT_ObjectFile)) {
      std::cerr << "llvm::TargetMachine.addPassesToEmitFile failed";
      std::exit(1);
    }
    pass.run(*module);
    diag->artifact("jit_module.asm", (std::string)jitModuleAsm);
    diag->artifact("jit_module.o", (std::string)jitModuleObject);
  }
#endif

  auto key = linker.addModule(std::move(module));
  return std::make_unique<Handle>(
    key,
    *this,
    linker.findEntry(name)
  );
}

JIT::Handle::Handle(
  llvm::orc::VModuleKey key,
  JIT::Pipeline &pipeline,
  EntryFn entry
) :
  key(key),
  pipeline(pipeline),
  entry(entry) {}

char *JIT::Handle::operator()(void *context, char *memory) {
  return entry(context, memory);
}

JIT::Handle::~Handle() {
  pipeline.linker.removeModule(key);
}
