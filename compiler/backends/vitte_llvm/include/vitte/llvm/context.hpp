#pragma once
#include <memory>
#include <string>
#include <vector>

namespace llvm {
    class LLVMContext;
    class Module;
    class IRBuilderBase;
    class TargetMachine;
    class Function;
    class Value;
}

namespace vitte::llvm {

struct TargetDesc {
    std::string triple;
    std::string cpu;
    std::string features;
    bool pic = true;
};

class Session;

class Context {
public:
    Context(Session&, const TargetDesc&);
    ~Context();

    llvm::LLVMContext& ctx();
    llvm::Module& mod();
    llvm::IRBuilderBase& ir();
    llvm::TargetMachine& tm();

private:
    Session& session;
    TargetDesc target;
    std::unique_ptr<llvm::LLVMContext> _ctx;
    std::unique_ptr<llvm::Module> _mod;
    std::unique_ptr<llvm::IRBuilderBase> _ir;
    std::unique_ptr<llvm::TargetMachine> _tm;
};

}
