#include "driver.hpp"

#include "options.hpp"
#include "passes.hpp"
#include "pipeline.hpp"

#include "../frontend/diagnostics.hpp"
#include "../frontend/diagnostics_messages.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace vitte::driver {

static std::string resolve_lang(const std::string& lang) {
    if (!lang.empty()) {
        return lang;
    }
    const char* env = std::getenv("LANG");
    if (!env || !*env) {
        env = std::getenv("LC_ALL");
    }
    if (env && *env) {
        return env;
    }
    return "en";
}

static bool command_exists(const std::string& cmd) {
    std::string query = "command -v '" + cmd + "' >/dev/null 2>&1";
    return std::system(query.c_str()) == 0;
}

static bool has_lld(std::string* out_path = nullptr) {
    if (const char* env = std::getenv("LLD_PATH"); env && *env) {
        std::filesystem::path p(env);
        if (std::filesystem::exists(p)) {
            if (out_path) {
                *out_path = p.string();
            }
            return true;
        }
    }

    if (command_exists("ld.lld")) {
        if (out_path) {
            *out_path = "ld.lld";
        }
        return true;
    }

    return false;
}

static int run_doctor() {
    bool ok = true;

    bool has_clang = command_exists("clang");
    bool has_clangpp = command_exists("clang++");
    bool has_make = command_exists("make");
    bool has_steelconf = std::filesystem::exists("steelconf");
    bool has_runtime_header = std::filesystem::exists("src/compiler/backends/runtime/vitte_runtime.hpp");
    bool has_cpp_probe = false;
    std::string lld_path;
    bool has_lld_tool = has_lld(&lld_path);

    std::cout << "[doctor] clang: " << (has_clang ? "ok" : "missing") << "\n";
    std::cout << "[doctor] clang++: " << (has_clangpp ? "ok" : "missing") << "\n";
    std::cout << "[doctor] make: " << (has_make ? "ok" : "missing") << "\n";
    std::cout << "[doctor] lld: " << (has_lld_tool ? "ok" : "missing") << "\n";
    std::cout << "[doctor] steelconf: " << (has_steelconf ? "ok" : "missing") << "\n";
    std::cout << "[doctor] runtime header: " << (has_runtime_header ? "ok" : "missing") << "\n";

    if (!has_clang || !has_clangpp) {
        std::cout << "[doctor] fix: install clang/clang++ and ensure they are in PATH\n";
        ok = false;
    }
    if (!has_make) {
        std::cout << "[doctor] fix: install make and ensure it is in PATH\n";
        ok = false;
    }
    if (!has_lld_tool) {
        std::cout << "[doctor] fix: install llvm (ld.lld) or set LLD_PATH\n";
    }
    if (!has_steelconf) {
        std::cout << "[doctor] note: no steelconf found in current directory\n";
    }
    if (!has_runtime_header) {
        std::cout << "[doctor] note: vitte_runtime.hpp not found (expected repo layout)\n";
    }

    if (has_clangpp) {
        namespace fs = std::filesystem;
        fs::path tmp_src = fs::temp_directory_path() / "vitte_doctor.cpp";
        fs::path tmp_out = fs::temp_directory_path() / "vitte_doctor.o";
        {
            std::ofstream out(tmp_src);
            if (out.is_open()) {
                out <<
                    "#include <vector>\n"
                    "int main() { std::vector<int> v; return (int)v.size(); }\n";
            }
        }
        std::string cmd = "clang++ -std=c++20 -c '" + tmp_src.string() + "' -o '" + tmp_out.string() + "' >/dev/null 2>&1";
        has_cpp_probe = std::system(cmd.c_str()) == 0;
        std::cout << "[doctor] c++ stdlib: " << (has_cpp_probe ? "ok" : "missing") << "\n";
        if (!has_cpp_probe) {
            std::cout << "[doctor] fix: install C++ standard library headers/toolchain for clang++\n";
            ok = false;
        }
        std::error_code ec;
        fs::remove(tmp_src, ec);
        fs::remove(tmp_out, ec);
    }

    const char* lang = std::getenv("LANG");
    const char* lc_all = std::getenv("LC_ALL");
    if ((!lang || !*lang) && (!lc_all || !*lc_all)) {
        std::cout << "[doctor] note: LANG/LC_ALL not set; diagnostics will default to en\n";
    }

    return ok ? 0 : 1;
}

static void print_onboarding_summary() {
    std::cout <<
        "Welcome to Vitte.\n"
        "\n"
        "What you can do first:\n"
        "  vitte build <file>    Compile an executable\n"
        "  vitte check <file>    Typecheck and resolve only\n"
        "  vitte emit <file>     Emit C++ only (no native compile)\n"
        "\n"
        "Example:\n"
        "  vitte build src/main.vit\n"
        "\n"
        "More:\n"
        "  vitte help\n";
}

static int run_explain(const Options& opts) {
    using vitte::frontend::diag::DiagExplain;
    using vitte::frontend::diag::DiagId;
    using vitte::frontend::diag::Localization;
    using vitte::frontend::diag::diag_explain;
    using vitte::frontend::diag::diag_id_from_code;

    if (opts.explain_code.empty()) {
        std::cerr << "[driver] error: missing diagnostic code (e.g. E0001)\n";
        return 1;
    }

    DiagId id{};
    if (!diag_id_from_code(opts.explain_code, &id)) {
        std::cerr << "[driver] error: unknown diagnostic code '" << opts.explain_code << "'\n";
        return 1;
    }

    Localization localization;
    localization.load("locales", resolve_lang(opts.lang), "diagnostics_explain.ftl");

    DiagExplain explain = diag_explain(id);
    std::string summary_key = opts.explain_code + ".summary";
    std::string fix_key = opts.explain_code + ".fix";
    std::string example_key = opts.explain_code + ".example";

    std::string summary = localization.lookup(summary_key);
    std::string fix = localization.lookup(fix_key);
    std::string example = localization.lookup(example_key);

    if (summary.empty()) {
        summary = explain.summary;
    }
    if (fix.empty()) {
        fix = explain.fix;
    }
    if (example.empty()) {
        example = explain.example;
    }

    std::cout << opts.explain_code << "\n";
    if (!summary.empty()) {
        std::cout << "Summary: " << summary << "\n";
    }
    if (!fix.empty()) {
        std::cout << "Fix: " << fix << "\n";
    }
    if (!example.empty()) {
        std::cout << "Example:\n" << example << "\n";
    }
    return 0;
}

static bool dir_is_empty(const std::filesystem::path& dir) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) {
        return true;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        (void)entry;
        return false;
    }
    return true;
}

static bool looks_like_vitte_project(const std::filesystem::path& dir) {
    namespace fs = std::filesystem;
    return fs::exists(dir / "steelconf") || fs::exists(dir / "src");
}

static int run_init(const Options& opts) {
    namespace fs = std::filesystem;
    fs::path cwd = fs::current_path();
    fs::path base = cwd;
    bool auto_subdir = false;

    if (!opts.init_dir.empty()) {
        base = cwd / opts.init_dir;
    } else if (!dir_is_empty(cwd) && !looks_like_vitte_project(cwd)) {
        base = cwd / "app";
        auto_subdir = true;
    }

    if (!fs::exists(base)) {
        fs::create_directories(base);
        std::cout << "[init] created " << base.string() << "\n";
    } else if (!fs::is_directory(base)) {
        std::cerr << "[init] error: " << base.string() << " exists and is not a directory\n";
        return 1;
    }

    if (auto_subdir) {
        std::cout << "[init] non-empty directory detected; using " << base.string() << "\n";
    }

    fs::path src_dir = base / "src";
    fs::path main_path = src_dir / "main.vit";
    fs::path steelconf_path = base / "steelconf";

    bool ok = true;

    if (!fs::exists(src_dir)) {
        fs::create_directories(src_dir);
        std::cout << "[init] created " << src_dir.string() << "\n";
    }

    if (!fs::exists(main_path)) {
        std::ofstream out(main_path);
        if (!out.is_open()) {
            std::cerr << "[init] error: failed to write " << main_path.string() << "\n";
            ok = false;
        } else {
            out <<
                "proc main() -> i32 {\n"
                "  return 0\n"
                "}\n";
            std::cout << "[init] created " << main_path.string() << "\n";
        }
    } else {
        std::cout << "[init] exists " << main_path.string() << "\n";
    }

    if (!fs::exists(steelconf_path)) {
        std::ofstream out(steelconf_path);
        if (!out.is_open()) {
            std::cerr << "[init] error: failed to write " << steelconf_path.string() << "\n";
            ok = false;
        } else {
            out <<
                "!muf 4\n"
                "\n"
                "[workspace]\n"
                "  .set name \"app\"\n"
                "  .set root \".\"\n"
                "  .set target_dir \"build\"\n"
                "  .set profile \"debug\"\n"
                "..\n"
                "\n"
                "[tool sh]\n"
                "  .exec \"sh\"\n"
                "..\n"
                "\n"
                "[bake build]\n"
                "  [run sh]\n"
                "    .set \"-c\" \"vitte build src/main.vit\"\n"
                "  ..\n"
                "  .output marker \"build/.build.ok\"\n"
                "..\n";
            std::cout << "[init] created " << steelconf_path.string() << "\n";
        }
    } else {
        std::cout << "[init] exists " << steelconf_path.string() << "\n";
    }

    if (ok) {
        fs::path hint = base == cwd ? fs::path("src/main.vit") : (fs::path(base.filename()) / "src/main.vit");
        std::cout << "[init] done. Try: vitte build " << hint.string() << "\n";
        return 0;
    }
    return 1;
}

/* -------------------------------------------------
 * Compiler driver entry
 * ------------------------------------------------- */
int run(int argc, char** argv) {
    Options opts = parse_options(argc, argv);

    if (opts.show_help) {
        print_help();
        return 0;
    }

    if (opts.init_project) {
        return run_init(opts);
    }

    if (opts.explain_diagnostic) {
        return run_explain(opts);
    }

    if (opts.run_doctor) {
        return run_doctor();
    }

    if (opts.input.empty()) {
        if (argc == 1) {
            print_onboarding_summary();
            return 0;
        }
        std::cerr << "[driver] error: no input file provided\n";
        print_help();
        return 1;
    }

    if (opts.target.rfind("kernel-", 0) == 0) {
        if (!has_lld()) {
            std::cerr << "[driver] error: lld not found; install llvm (ld.lld) or set LLD_PATH\n";
            return 1;
        }
    }

    if (opts.parse_only || opts.resolve_only || opts.hir_only || opts.mir_only) {
        PassResult pass_result = run_passes(opts);
        return pass_result.ok ? 0 : 1;
    }

    /* ---------------------------------------------
     * Run compilation pipeline
     * --------------------------------------------- */
    bool ok = run_pipeline(opts);
    if (!ok) {
        std::cerr << "[driver] compilation failed\n";
        std::_Exit(EXIT_FAILURE);
    }

    return 0;
}

} // namespace vitte::driver
