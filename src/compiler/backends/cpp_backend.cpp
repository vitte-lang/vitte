#include "cpp_backend.hpp"

#include "lower/lower_mir.hpp"
#include "emit/emit.hpp"
#include "context/cpp_context.hpp"
#include "toolchain/clang.hpp"

#include <filesystem>
#include <iostream>
#include <cstdlib>

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

    clang_opts.libraries.push_back("stdc++");

    std::filesystem::path work_dir_path = options.work_dir;
    if (std::filesystem::exists(work_dir_path / "vitte_runtime.hpp")) {
        clang_opts.include_dirs.push_back(work_dir_path.string());
    }

    if (!options.runtime_include.empty()) {
        std::filesystem::path p = options.runtime_include;
        if (std::filesystem::exists(p / "vitte_runtime.hpp")) {
            clang_opts.include_dirs.push_back(p.string());
        }
    }

    const char* runtime_inc = std::getenv("VITTE_RUNTIME_INCLUDE");
    if (runtime_inc && *runtime_inc) {
        std::filesystem::path p = runtime_inc;
        if (std::filesystem::exists(p / "vitte_runtime.hpp")) {
            clang_opts.include_dirs.push_back(p.string());
        }
    } else {
        const char* root = std::getenv("VITTE_ROOT");
        std::filesystem::path base = root && *root
            ? std::filesystem::path(root)
            : std::filesystem::current_path();

        std::filesystem::path src_rt = base / "src/compiler/backends/runtime";
        if (std::filesystem::exists(src_rt / "vitte_runtime.hpp")) {
            clang_opts.include_dirs.push_back(src_rt.string());
        }

        std::filesystem::path tgt_inc = base / "target/include";
        if (std::filesystem::exists(tgt_inc / "vitte_runtime.hpp")) {
            clang_opts.include_dirs.push_back(tgt_inc.string());
        }
    }

    const char* root = std::getenv("VITTE_ROOT");
    std::filesystem::path base = root && *root
        ? std::filesystem::path(root)
        : std::filesystem::current_path();

    std::filesystem::path runtime_cpp = base / "src/compiler/backends/runtime/vitte_runtime.cpp";
    if (std::filesystem::exists(runtime_cpp)) {
        clang_opts.extra_sources.push_back(runtime_cpp.string());
    }

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
