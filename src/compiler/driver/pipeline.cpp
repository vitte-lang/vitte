#include "pipeline.hpp"

#include "options.hpp"

#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"
#include "../frontend/macro_expand.hpp"
#include "../frontend/module_loader.hpp"
#include "../frontend/validate.hpp"
#include "../frontend/diagnostics.hpp"
#include "../frontend/disambiguate.hpp"
#include "../frontend/resolve.hpp"
#include "../frontend/lower_hir.hpp"
#include "../ir/validate.hpp"
#include "../ir/hir.hpp"
#include "../ir/lower_mir.hpp"

#include "../backends/cpp_backend.hpp"

#include <fstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <sys/resource.h>

#include <openssl/sha.h>

namespace vitte::driver {

static std::string hash_content(const std::string& text) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(text.data()), text.size(), digest);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : digest) {
        oss << std::setw(2) << static_cast<unsigned int>(b);
    }
    return oss.str();
}

static std::string file_hash(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    std::string source(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );
    return hash_content(source);
}

static std::filesystem::path stage_cache_file(const Options& opts, const char* stage) {
    std::filesystem::path cache_dir = ".vitte-cache";
    std::error_code ec;
    std::filesystem::create_directories(cache_dir, ec);
    std::string key = opts.input + "|" + stage;
    std::string h = hash_content(key).substr(0, 16);
    std::ostringstream name;
    name << stage << "_" << h << ".cache";
    return cache_dir / name.str();
}

static bool read_stage_cache_deps(const std::filesystem::path& cache,
                                  std::vector<std::pair<std::string, std::string>>& deps) {
    std::ifstream in(cache);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("dep=", 0) != 0) {
            continue;
        }
        const std::string payload = line.substr(4);
        const auto sep = payload.rfind('|');
        if (sep == std::string::npos) {
            continue;
        }
        deps.push_back({payload.substr(0, sep), payload.substr(sep + 1)});
    }
    return true;
}

static bool is_stage_cache_valid(const std::filesystem::path& cache) {
    if (!std::filesystem::exists(cache)) {
        return false;
    }
    std::vector<std::pair<std::string, std::string>> deps;
    if (!read_stage_cache_deps(cache, deps)) {
        return false;
    }
    for (const auto& dep : deps) {
        const std::string got = file_hash(dep.first);
        if (got.empty() || got != dep.second) {
            return false;
        }
    }
    return true;
}

struct StageCacheReport {
    bool parse_hit = false;
    bool resolve_hit = false;
    bool ir_hit = false;
};

static StageCacheReport compute_stage_cache_report(const Options& opts) {
    StageCacheReport r;
    r.parse_hit = is_stage_cache_valid(stage_cache_file(opts, "parse"));
    r.resolve_hit = is_stage_cache_valid(stage_cache_file(opts, "resolve"));
    r.ir_hit = is_stage_cache_valid(stage_cache_file(opts, "ir"));
    return r;
}

static bool try_fast_incremental_skip(const Options& opts, std::ostream& log) {
    if (opts.emit_cpp || opts.emit_obj || opts.emit_stdout) {
        return false;
    }
    if (!opts.stage.empty() && opts.stage != "backend") {
        return false;
    }
    if (!std::filesystem::exists(opts.output)) {
        return false;
    }
    const auto report = compute_stage_cache_report(opts);
    if (!report.parse_hit || !report.resolve_hit || !report.ir_hit) {
        return false;
    }
    log << "[pipeline] incremental cache hit (no source/dependency change)\n";
    log << "[pipeline] stage cache hit: parse, resolve, ir\n";
    log << "[pipeline] done\n";
    return true;
}

static void write_stage_cache(const Options& opts,
                              const frontend::modules::ModuleIndex& index,
                              const char* stage) {
    if (opts.emit_cpp || opts.emit_obj || opts.emit_stdout) {
        return;
    }
    const auto cache = stage_cache_file(opts, stage);
    std::ofstream out(cache);
    if (!out.is_open()) {
        return;
    }
    out << "stage=" << stage << "\n";
    out << "input=" << opts.input << "\n";
    out << "output=" << opts.output << "\n";
    for (const auto& kv : index.module_files) {
        const std::string hash = file_hash(kv.second);
        if (hash.empty()) {
            continue;
        }
        out << "dep=" << kv.second << "|" << hash << "\n";
    }
}

/* -------------------------------------------------
 * Run full compilation pipeline
 * ------------------------------------------------- */
bool run_pipeline(const Options& opts) {
    using Clock = std::chrono::steady_clock;
    auto t_total_start = Clock::now();

    std::ostream& log = opts.emit_stdout ? std::cerr : std::cout;
    const auto cache_report = compute_stage_cache_report(opts);
    if (opts.cache_report) {
        log << "[cache] parse=" << (cache_report.parse_hit ? "hit" : "miss")
            << " resolve=" << (cache_report.resolve_hit ? "hit" : "miss")
            << " ir=" << (cache_report.ir_hit ? "hit" : "miss") << "\n";
    }

    if (try_fast_incremental_skip(opts, log)) {
        return true;
    }

    log << "[pipeline] input: " << opts.input << "\n";
    log << "[stage] parse\n";

    /* ---------------------------------------------
     * 1. Frontend: read source
     * --------------------------------------------- */
    std::ifstream in(opts.input);
    if (!in.is_open()) {
        std::cerr << "[pipeline] error[E3001]: cannot open input file\n";
        return false;
    }

    std::string source(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );

    auto t_parse_start = Clock::now();

    /* ---------------------------------------------
     * 2. Lexing + Parsing → AST
     * --------------------------------------------- */
    frontend::Lexer lexer(source, opts.input);
    frontend::diag::DiagnosticEngine diagnostics(opts.lang);
    auto emit_diags = [&](std::ostream& os = std::cerr) {
        if (opts.diag_code_only) {
            frontend::diag::render_all_code_only(diagnostics, os, opts.deterministic, opts.diag_filter_codes);
        } else if (opts.diag_json) {
            frontend::diag::render_all_json(
                diagnostics, os, opts.diag_json_pretty, opts.deterministic, opts.diag_filter_codes);
        } else {
            frontend::diag::render_all(diagnostics, os, opts.deterministic, opts.diag_filter_codes);
        }
    };
    frontend::ast::AstContext ast_ctx;
    ast_ctx.sources.push_back(lexer.source_file());
    frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, opts.strict_parse);
    auto ast = parser.parse_module();
    (void)ast;

    frontend::modules::ModuleIndex module_index;
    frontend::modules::LoadOptions module_opts;
    module_opts.stdlib_profile = opts.stdlib_profile;
    module_opts.allow_experimental = opts.allow_experimental;
    module_opts.warn_experimental = opts.warn_experimental;
    module_opts.deny_internal = opts.deny_internal;
    module_opts.allow_legacy_self_leaf = opts.allow_legacy_self_leaf;
    frontend::modules::load_modules(ast_ctx, ast, diagnostics, opts.input, module_index, module_opts);
    frontend::modules::rewrite_member_access(ast_ctx, ast, module_index, &diagnostics);
    if (opts.dump_stdlib_map) {
        frontend::modules::dump_stdlib_map(std::cout, module_index);
    }
    if (opts.dump_module_index) {
        frontend::modules::dump_module_index_json(
            std::cout, module_index, opts.stdlib_profile, opts.allow_experimental);
    }
    if (diagnostics.has_errors()) {
        emit_diags();
        return false;
    }

    frontend::passes::expand_macros(ast_ctx, ast, diagnostics);
    frontend::passes::disambiguate_invokes(ast_ctx, ast);
    frontend::validate::validate_module(ast_ctx, ast, diagnostics);
    if (diagnostics.has_errors()) {
        emit_diags();
        return false;
    }
    auto t_parse_end = Clock::now();
    write_stage_cache(opts, module_index, "parse");

    log << "[stage] resolve\n";
    auto t_resolve_start = Clock::now();
    frontend::resolve::Resolver resolver(diagnostics, opts.strict_types, opts.strict_imports || opts.strict_modules, opts.strict_modules);
    resolver.resolve_module(ast_ctx, ast);
    if (diagnostics.has_errors()) {
        emit_diags();
        std::cerr << "[pipeline] error[E1000]: resolve failed\n";
        return false;
    }
    auto t_resolve_end = Clock::now();
    write_stage_cache(opts, module_index, "resolve");

    log << "[stage] ir\n";
    auto t_ir_start = Clock::now();
    ir::HirContext hir_ctx;
    auto hir = frontend::lower::lower_to_hir(ast_ctx, ast, hir_ctx, diagnostics);
    ir::validate::validate_module(hir_ctx, hir, diagnostics);
    if (diagnostics.has_errors()) {
        emit_diags();
        std::cerr << "[pipeline] error[E2000]: hir lowering failed\n";
        return false;
    }
    auto t_ir_end = Clock::now();
    write_stage_cache(opts, module_index, "ir");

    auto mir = ir::lower::lower_to_mir(hir_ctx, hir, diagnostics);
    if (diagnostics.has_errors()) {
        emit_diags();
        std::cerr << "[pipeline] error[E2000]: mir lowering failed\n";
        return false;
    }
    if (opts.dump_mir) {
        std::cout << ir::dump_to_string(mir);
    }
    if (opts.mir_only) {
        std::cout << "[pipeline] mir ok\n";
        return true;
    }

    log << "[stage] backend\n";
    auto t_backend_start = Clock::now();

    /* ---------------------------------------------
     * 6. Backend: MIR → native
     * --------------------------------------------- */
    backend::CppBackendOptions be_opts;
    be_opts.debug = opts.debug;
    be_opts.optimize = opts.optimize;
    be_opts.opt_level = opts.opt_level;
    be_opts.work_dir = ".";
    be_opts.runtime_include = opts.runtime_include;
    be_opts.target = opts.target;
    be_opts.arduino_upload = opts.arduino_upload;
    be_opts.arduino_port = opts.arduino_port;
    be_opts.arduino_fqbn = opts.arduino_fqbn;
    be_opts.emit_obj = opts.emit_obj;
    be_opts.repro = opts.repro;
    be_opts.repro_strict = opts.repro_strict;
    be_opts.freestanding = opts.freestanding;

    std::string output = opts.output;
    if (opts.emit_obj && output == "a.out") {
        output = "a.o";
    }

    if (opts.emit_cpp && !opts.emit_obj) {
        if (opts.emit_stdout) {
            if (!backend::emit_cpp_backend(mir, std::cout, be_opts)) {
                std::cerr << "[pipeline] error[E3002]: emit-cpp failed\n";
                return false;
            }
            std::cout << "\n";
        } else {
            log << "[pipeline] emit-cpp only (skipping native compile)\n";
        }
    } else {
        if (!backend::compile_cpp_backend(
                mir,
                output,
                be_opts
            )) {
            std::cerr << "[pipeline] error[E3003]: backend compilation failed\n";
            return false;
        }
    }

    auto t_backend_end = Clock::now();
    auto t_total_end = Clock::now();

    auto ms = [](auto dur) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    };

    log << "[pipeline] stages:\n";
    log << "  parse: " << ms(t_parse_end - t_parse_start) << " ms\n";
    log << "  resolve: " << ms(t_resolve_end - t_resolve_start) << " ms\n";
    log << "  ir: " << ms(t_ir_end - t_ir_start) << " ms\n";
    log << "  backend: " << ms(t_backend_end - t_backend_start) << " ms\n";
    log << "  total: " << ms(t_total_end - t_total_start) << " ms\n";
    if (opts.profile_mode) {
        struct rusage ru {};
        if (getrusage(RUSAGE_SELF, &ru) == 0) {
#if defined(__APPLE__)
            const long max_rss_kb = ru.ru_maxrss / 1024;
#else
            const long max_rss_kb = ru.ru_maxrss;
#endif
            log << "[profile] max_rss_kb: " << max_rss_kb << "\n";
        }
        std::vector<std::pair<std::string, std::size_t>> modules;
        modules.reserve(module_index.module_loc.size());
        for (const auto& kv : module_index.module_loc) {
            modules.push_back(kv);
        }
        std::sort(modules.begin(), modules.end(), [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });
        log << "[profile] top_modules_by_loc:\n";
        for (std::size_t i = 0; i < modules.size() && i < 5; ++i) {
            log << "  " << modules[i].first << " loc=" << modules[i].second << "\n";
        }
    }
    log << "[pipeline] done\n";
    if (opts.fail_on_warning && diagnostics.warning_count() > 0) {
        emit_diags();
        std::cerr << "[pipeline] error: warnings are treated as errors (--fail-on-warning)\n";
        return false;
    }
    return true;
}

} // namespace vitte::driver
