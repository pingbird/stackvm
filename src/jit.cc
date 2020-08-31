#include "jit.h"

using std::unique_ptr;

void JIT::init() {
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();
}

JIT::Linker::Linker(llvm::TargetMachine &machine, llvm::LLVMContext &context) :
  machine(machine),
  dataLayout(machine.createDataLayout()),
  context(context),
  resolver(createLegacyLookupResolver(
    session,
    [this](llvm::StringRef name) -> llvm::JITSymbol {
      if (auto symbol = objectLayer.findSymbol(std::string(name), false)) {
        return symbol;
      } else if (auto error = symbol.takeError()) {
        return std::move(error);
      } if (auto address = llvm::RTDyldMemoryManager::getSymbolAddressInProcess(name)) {
        return llvm::JITSymbol(address, llvm::JITSymbolFlags::Exported);
      } else {
        return llvm::JITSymbol(nullptr);
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
  auto key = session.allocateVModule();
  cantFail(compileLayer.addModule(key, std::move(module)));
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

JIT::Pipeline::Pipeline() :
  machine(llvm::EngineBuilder().selectTarget()),
  backend(*machine, context) {}

std::unique_ptr<JIT::Handle> JIT::Pipeline::compile(IR::Graph *graph) {
  auto module = std::make_unique<llvm::Module>("bf", context);
  Backend::LLVM::ModuleCompiler moduleCompiler(*machine, context, *module);
  moduleCompiler.helloWorld();
  auto key = backend.addModule(std::move(module));
  return std::make_unique<JIT::Handle>(
    key,
    *this,
    backend.findEntry("code")
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

JIT::Handle::~Handle() {
  pipeline.backend.removeModule(key);
}
