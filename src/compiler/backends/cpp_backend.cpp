#include "cpp_backend.hpp"

#include "lower/lower_mir.hpp"
#include "emit/emit.hpp"
#include "context/cpp_context.hpp"
#include "toolchain/clang.hpp"

#include <filesystem>
#include <iostream>

namespace vitte::backend {

/* -------------------------------------------------
 * Compile MIR to native executable via C++
 * ------------------------------------------------- */
bool compile_cpp_backend(
    const std::vector<lower::MirFunction>& mir_functions,
    const std::string& output_exe,
    const CppBackendOptions& options
) {
    context::CppContext ctx;

    /* ---------------------------------------------
     * Configure context
     * --------------------------------------------- */
    ctx.set_debug(options.debug);
    ctx.set_optimize(options.optimize);

    ctx.add_include("<cstdint>");
    ctx.add_include("<cstdlib>");
    ctx.add_include("\"vitte_runtime.hpp\"");

    /* ---------------------------------------------
     * Lower MIR → C++ AST
     * --------------------------------------------- */
    ast::cpp::CppTranslationUnit tu =
        lower::lower_mir(mir_functions, ctx);

    /* ---------------------------------------------
     * Emit C++ file
     * --------------------------------------------- */
    std::filesystem::path cpp_path =
        std::filesystem::path(options.work_dir) / "vitte_out.cpp";

    if (!emit::emit_file(cpp_path.string(), tu, ctx)) {
        std::cerr << "[cpp-backend] failed to emit C++ file\n";
        return false;
    }

    /* ---------------------------------------------
     * Invoke clang++
     * --------------------------------------------- */
    toolchain::ClangOptions clang_opts;
    clang_opts.debug = options.debug;
    clang_opts.optimize = options.optimize;
    clang_opts.opt_level = options.opt_level;
    clang_opts.verbose = options.verbose;

    clang_opts.include_dirs.push_back(options.work_dir);
    clang_opts.libraries.push_back("stdc++");

    if (!toolchain::invoke_clang(
            cpp_path.string(),
            output_exe,
            clang_opts
        )) {
        std::cerr << "[cpp-backend] clang invocation failed\n";
        return false;
    }

    return true;
}

} // namespace vitte::backend
