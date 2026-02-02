#pragma once
#include <memory>
#include <string>

namespace llvm {
    class LLVMContext;
    class Module;
    class IRBuilderBase;
    class TargetMachine;
}

namespace vitte::codegen::llvm_backend {

struct TargetDesc {
    std::string triple;
    std::string cpu;
    std::string features;
    bool pic = true;
};

class Session;

class CodegenContext {
public:
    CodegenContext(Session& session, const TargetDesc& target);
    ~CodegenContext();

    llvm::LLVMContext& llvmContext();
    llvm::Module& module();
    llvm::IRBuilderBase& builder();
    llvm::TargetMachine& targetMachine();

    void beginModule(const std::string& name);
    void endModule();

private:
    Session& session;
    TargetDesc target;

    std::unique_ptr<llvm::LLVMContext> ctx;
    std::unique_ptr<llvm::Module> mod;
    std::unique_ptr<llvm::IRBuilderBase> irb;
    std::unique_ptr<llvm::TargetMachine> tm;
};

} // namespace vitte::codegen::llvm_backend
