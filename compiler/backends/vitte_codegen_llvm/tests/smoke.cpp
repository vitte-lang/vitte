// ============================================================
// vitte_codegen_llvm::tests::smoke
// Test de fumée du backend LLVM
// ============================================================

#include <cassert>
#include <iostream>
#include <filesystem>

#include "vitte/codegen/llvm/emit.hpp"
#include "vitte/codegen/llvm/passes.hpp"
#include "vitte/codegen/llvm/context.hpp"

// Headers internes lowering
// (supposé exposés par le backend)
namespace vitte::codegen::llvm_backend::lower {
    struct Program;
    bool compileModule(const Program& program,
                       const std::string& moduleName,
                       const std::string& outputPath,
                       emit::OutputKind outputKind,
                       passes::OptLevel optLevel,
                       const std::string& triple);
}

// ------------------------------------------------------------
// MIR factice minimal (mock)
// ------------------------------------------------------------

namespace vitte::mir {

enum class BinOp { Add };

struct Expr {
    enum class Kind { Const, Binary } kind;
    long value;
    Expr* lhs;
    Expr* rhs;
};

struct Block {
    unsigned id;
    std::vector<Expr*> stmts;
    struct {
        bool hasValue;
        Expr* value;
    } terminator;
};

struct Function {
    unsigned id;
    std::string name;
    std::vector<Block> blocks;
    std::vector<int> params;
    std::vector<int> locals;
    bool returnsVoid;
};

struct Program {
    std::vector<Function> functions;
};

} // namespace vitte::mir


// ------------------------------------------------------------
// Création d’un programme MIR minimal
// ------------------------------------------------------------

static vitte::mir::Program makeMinimalProgram()
{
    using namespace vitte::mir;

    // Expression constante
    auto* cst = new Expr{
        Expr::Kind::Const,
        42,
        nullptr,
        nullptr
    };

    Block block;
    block.id = 0;
    block.stmts = {};
    block.terminator.hasValue = true;
    block.terminator.value = cst;

    Function fn;
    fn.id = 0;
    fn.name = "main";
    fn.blocks = { block };
    fn.params = {};
    fn.locals = {};
    fn.returnsVoid = false;

    Program prog;
    prog.functions.push_back(fn);

    return prog;
}


// ------------------------------------------------------------
// Test principal
// ------------------------------------------------------------

int main()
{
    using namespace vitte::codegen::llvm_backend;

    std::cout << "[smoke] starting LLVM backend smoke test\n";

    auto program = makeMinimalProgram();

    std::filesystem::path out =
        std::filesystem::temp_directory_path() /
        "vitte_llvm_smoke.o";

    bool ok =
        lower::compileModule(
            program,
            "vitte_smoke_module",
            out.string(),
            emit::OutputKind::Object,
            passes::OptLevel::O0,
            llvm::sys::getDefaultTargetTriple()
        );

    assert(ok && "LLVM backend smoke test failed");

    assert(
        std::filesystem::exists(out) &&
        "output object file was not created"
    );

    std::cout << "[smoke] LLVM backend smoke test OK\n";
    return 0;
}
