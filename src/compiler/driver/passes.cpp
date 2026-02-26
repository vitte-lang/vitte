#include "passes.hpp"

#include "options.hpp"

#include "../frontend/diagnostics.hpp"
#include "../frontend/disambiguate.hpp"
#include "../frontend/lexer.hpp"
#include "../frontend/parser.hpp"
#include "../frontend/macro_expand.hpp"
#include "../frontend/module_loader.hpp"
#include "../frontend/validate.hpp"
#include "../frontend/resolve.hpp"
#include "../frontend/lower_hir.hpp"
#include "../ir/hir.hpp"
#include "../ir/lower_mir.hpp"
#include "../ir/validate.hpp"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

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

static bool is_stage_cache_valid(const Options& opts, const char* stage) {
    const auto cache = stage_cache_file(opts, stage);
    if (!std::filesystem::exists(cache)) {
        return false;
    }
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
        const std::string path = payload.substr(0, sep);
        const std::string expected = payload.substr(sep + 1);
        const std::string got = file_hash(path);
        if (got.empty() || got != expected) {
            return false;
        }
    }
    return true;
}

static bool read_file(const std::string& path, std::string& out) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "[driver] error: cannot open input file: " << path
                  << " (" << std::strerror(errno) << ")\n";
        return false;
    }
    out.assign(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );
    return true;
}

PassResult run_passes(const Options& opts) {
    PassResult result;

    std::string source;
    if (!read_file(opts.input, source)) {
        result.ok = false;
        return result;
    }

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
    auto module = parser.parse_module();

    if (opts.parse_only) {
        if (opts.dump_ast) {
            std::cout << frontend::ast::dump_to_string(ast_ctx.node(module));
        }
        if (opts.parse_with_modules) {
            frontend::modules::ModuleIndex module_index;
            frontend::modules::LoadOptions module_opts;
            module_opts.stdlib_profile = opts.stdlib_profile;
            module_opts.allow_experimental = opts.allow_experimental;
            module_opts.warn_experimental = opts.warn_experimental;
            module_opts.deny_internal = opts.deny_internal;
            module_opts.allow_legacy_self_leaf = opts.allow_legacy_self_leaf;
            frontend::modules::load_modules(ast_ctx, module, diagnostics, opts.input, module_index, module_opts);
            frontend::modules::rewrite_member_access(ast_ctx, module, module_index, &diagnostics);
            if (opts.dump_stdlib_map) {
                frontend::modules::dump_stdlib_map(std::cout, module_index);
            }
            if (opts.dump_module_index) {
                frontend::modules::dump_module_index_json(
                    std::cout, module_index, opts.stdlib_profile, opts.allow_experimental);
            }
            if (diagnostics.has_errors()) {
                emit_diags();
                result.ok = false;
                return result;
            }
            if (!opts.parse_silent) {
                std::cout << "[driver] parse-only: loaded modules\n";
            }
        } else {
            if (!opts.parse_silent) {
                std::cout << "[driver] parse-only: skipped module loading\n";
            }
        }
        frontend::validate::validate_module(ast_ctx, module, diagnostics);
        if (diagnostics.has_errors()) {
            emit_diags();
            result.ok = false;
            return result;
        }
        std::cout << "[driver] parse ok\n";
        if (opts.fail_on_warning && diagnostics.warning_count() > 0) {
            emit_diags();
            std::cerr << "[driver] error: warnings are treated as errors (--fail-on-warning)\n";
            result.ok = false;
            return result;
        }
        result.ok = true;
        return result;
    }

    frontend::modules::ModuleIndex module_index;
    frontend::modules::LoadOptions module_opts;
    module_opts.stdlib_profile = opts.stdlib_profile;
    module_opts.allow_experimental = opts.allow_experimental;
    module_opts.warn_experimental = opts.warn_experimental;
    module_opts.deny_internal = opts.deny_internal;
    module_opts.allow_legacy_self_leaf = opts.allow_legacy_self_leaf;
    frontend::modules::load_modules(ast_ctx, module, diagnostics, opts.input, module_index, module_opts);
    frontend::modules::rewrite_member_access(ast_ctx, module, module_index, &diagnostics);
    if (opts.cache_report) {
        std::cout << "[cache] parse=" << (is_stage_cache_valid(opts, "parse") ? "hit" : "miss")
                  << " resolve=" << (is_stage_cache_valid(opts, "resolve") ? "hit" : "miss")
                  << " ir=" << (is_stage_cache_valid(opts, "ir") ? "hit" : "miss") << "\n";
    }
    if (opts.dump_stdlib_map) {
        frontend::modules::dump_stdlib_map(std::cout, module_index);
    }
    if (opts.dump_module_index) {
        frontend::modules::dump_module_index_json(
            std::cout, module_index, opts.stdlib_profile, opts.allow_experimental);
    }
    if (diagnostics.has_errors()) {
        emit_diags();
        result.ok = false;
        return result;
    }

    frontend::passes::expand_macros(ast_ctx, module, diagnostics);
    frontend::passes::disambiguate_invokes(ast_ctx, module);

    if (opts.dump_ast) {
        std::cout << frontend::ast::dump_to_string(ast_ctx.node(module));
    }

    frontend::validate::validate_module(ast_ctx, module, diagnostics);

    if (diagnostics.has_errors()) {
        emit_diags();
        result.ok = false;
        return result;
    }

    frontend::resolve::Resolver resolver(diagnostics, opts.strict_types, opts.strict_imports || opts.strict_modules, opts.strict_modules);
    resolver.resolve_module(ast_ctx, module);

    if (opts.dump_resolve) {
        frontend::resolve::dump_symbols(resolver.symbols(), std::cout);
    }

    if (diagnostics.has_errors()) {
        emit_diags();
        std::cerr << "[driver] error[E1000]: resolve failed\n";
        result.ok = false;
        return result;
    }

    if (opts.resolve_only) {
        std::cout << "[driver] resolve ok\n";
        if (opts.fail_on_warning && diagnostics.warning_count() > 0) {
            emit_diags();
            std::cerr << "[driver] error: warnings are treated as errors (--fail-on-warning)\n";
            result.ok = false;
            return result;
        }
        result.ok = true;
        return result;
    }

    bool dump_hir_pretty = opts.dump_hir;
    bool dump_hir_json = opts.dump_hir_json;
    bool dump_hir_compact = opts.dump_hir_compact;

    if (!opts.dump_hir_format.empty()) {
        dump_hir_pretty = false;
        dump_hir_json = false;
        dump_hir_compact = false;
        if (opts.dump_hir_format == "pretty") {
            dump_hir_pretty = true;
        } else if (opts.dump_hir_format == "json") {
            dump_hir_json = true;
        } else if (opts.dump_hir_format == "compact") {
            dump_hir_compact = true;
        } else {
            std::cerr << "[driver] unknown dump-hir format: " << opts.dump_hir_format << "\n";
            result.ok = false;
            return result;
        }
    }

    if (opts.hir_only || dump_hir_pretty || dump_hir_json || dump_hir_compact) {
        ir::HirContext hir_ctx;
        auto hir = frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        if (dump_hir_pretty) {
            std::cout << ir::dump_to_string(hir_ctx, hir);
        }
        if (dump_hir_json) {
            std::cout << ir::dump_json_to_string(hir_ctx, hir);
        }
        if (dump_hir_compact) {
            std::cout << ir::dump_compact_to_string(hir_ctx, hir);
        }
        ir::validate::validate_module(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            emit_diags();
            std::cerr << "[driver] error[E2000]: hir lowering failed\n";
            result.ok = false;
            return result;
        }
        if (opts.hir_only) {
            std::cout << "[driver] hir ok\n";
            if (opts.fail_on_warning && diagnostics.warning_count() > 0) {
                emit_diags();
                std::cerr << "[driver] error: warnings are treated as errors (--fail-on-warning)\n";
                result.ok = false;
                return result;
            }
            result.ok = true;
            return result;
        }
    }

    if (opts.dump_mir) {
        ir::HirContext hir_ctx;
        auto hir = frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        ir::validate::validate_module(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            emit_diags();
            std::cerr << "[driver] error[E2000]: hir lowering failed\n";
            result.ok = false;
            return result;
        }
        auto mir = ir::lower::lower_to_mir(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            emit_diags();
            std::cerr << "[driver] error[E2000]: mir lowering failed\n";
            result.ok = false;
            return result;
        }
        std::cout << ir::dump_to_string(mir);
    }

    if (opts.mir_only) {
        ir::HirContext hir_ctx;
        auto hir = frontend::lower::lower_to_hir(ast_ctx, module, hir_ctx, diagnostics);
        ir::validate::validate_module(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            emit_diags();
            std::cerr << "[driver] error[E2000]: hir lowering failed\n";
            result.ok = false;
            return result;
        }
        auto mir = ir::lower::lower_to_mir(hir_ctx, hir, diagnostics);
        if (diagnostics.has_errors()) {
            emit_diags();
            std::cerr << "[driver] error[E2000]: mir lowering failed\n";
            result.ok = false;
            return result;
        }
        if (opts.dump_mir) {
            std::cout << ir::dump_to_string(mir);
        }
        std::cout << "[driver] mir ok\n";
        if (opts.fail_on_warning && diagnostics.warning_count() > 0) {
            emit_diags();
            std::cerr << "[driver] error: warnings are treated as errors (--fail-on-warning)\n";
            result.ok = false;
            return result;
        }
        result.ok = true;
        return result;
    }

    if (opts.fail_on_warning && diagnostics.warning_count() > 0) {
        emit_diags();
        std::cerr << "[driver] error: warnings are treated as errors (--fail-on-warning)\n";
        result.ok = false;
        return result;
    }
    result.ok = true;
    return result;
}

} // namespace vitte::driver
