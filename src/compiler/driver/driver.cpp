#include "driver.hpp"

#include "options.hpp"
#include "passes.hpp"
#include "pipeline.hpp"

#include "../frontend/diagnostics.hpp"
#include "../frontend/diagnostics_messages.hpp"
#include "../frontend/lexer.hpp"
#include "../frontend/module_loader.hpp"
#include "../frontend/parser.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
        "  vitte check <file>    Parse + resolve + IR (no backend)\n"
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

static bool apply_stage_override(Options& opts, std::string& error) {
    if (opts.stage.empty()) {
        return true;
    }

    opts.parse_only = false;
    opts.resolve_only = false;
    opts.hir_only = false;
    opts.mir_only = false;

    if (opts.stage == "parse") {
        opts.parse_only = true;
        return true;
    }
    if (opts.stage == "resolve") {
        opts.resolve_only = true;
        return true;
    }
    if (opts.stage == "ir") {
        opts.mir_only = true;
        return true;
    }
    if (opts.stage == "backend") {
        return true;
    }

    error = "invalid --stage value '" + opts.stage + "' (expected parse|resolve|ir|backend)";
    return false;
}

static bool run_with_options(Options run_opts) {
    run_opts.dump_ast = false;
    run_opts.dump_ir = false;
    run_opts.dump_resolve = false;
    run_opts.dump_hir = false;
    run_opts.dump_mir = false;
    run_opts.dump_hir_json = false;
    run_opts.dump_hir_compact = false;
    run_opts.dump_hir_format.clear();
    run_opts.emit_stdout = false;
    run_opts.parse_silent = true;

    if (run_opts.parse_only || run_opts.resolve_only || run_opts.hir_only || run_opts.mir_only) {
        return run_passes(run_opts).ok;
    }
    return run_pipeline(run_opts);
}

static void emit_driver_diags(const Options& opts, const frontend::diag::DiagnosticEngine& diagnostics, std::ostream& os = std::cerr) {
    if (opts.diag_code_only) {
        frontend::diag::render_all_code_only(diagnostics, os, opts.deterministic);
    } else if (opts.diag_json) {
        frontend::diag::render_all_json(diagnostics, os, opts.diag_json_pretty, opts.deterministic);
    } else {
        frontend::diag::render_all(diagnostics, os, opts.deterministic);
    }
}

static int run_clean_cache() {
    namespace fs = std::filesystem;
    const fs::path cache = ".vitte-cache";
    std::error_code ec;
    if (!fs::exists(cache)) {
        std::cout << "[clean-cache] nothing to clean\n";
        return 0;
    }
    fs::remove_all(cache, ec);
    if (ec) {
        std::cerr << "[clean-cache] error: failed to remove .vitte-cache (" << ec.message() << ")\n";
        return 1;
    }
    std::cout << "[clean-cache] removed .vitte-cache\n";
    return 0;
}

static int run_reduce(const Options& opts) {
    namespace fs = std::filesystem;

    std::ifstream in(opts.input);
    if (!in.is_open()) {
        std::cerr << "[reduce] error: cannot open input file: " << opts.input << "\n";
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    if (lines.empty()) {
        std::cerr << "[reduce] error: input file is empty\n";
        return 1;
    }

    fs::path input_path(opts.input);
    fs::path tmp_path = input_path;
    tmp_path += ".reduce.tmp.vit";
    fs::path out_path = input_path;
    out_path += ".reduced.vit";

    auto write_lines = [](const fs::path& path, const std::vector<std::string>& content) -> bool {
        std::ofstream out(path);
        if (!out.is_open()) {
            return false;
        }
        for (std::size_t i = 0; i < content.size(); ++i) {
            out << content[i];
            if (i + 1 < content.size()) {
                out << "\n";
            }
        }
        return true;
    };

    auto still_fails = [&](const std::vector<std::string>& candidate) -> bool {
        if (candidate.empty()) {
            return false;
        }
        if (!write_lines(tmp_path, candidate)) {
            return false;
        }
        Options run_opts = opts;
        run_opts.input = tmp_path.string();
        return !run_with_options(run_opts);
    };

    if (!still_fails(lines)) {
        std::cerr << "[reduce] error: input does not fail for the current stage/options\n";
        std::error_code ec;
        fs::remove(tmp_path, ec);
        return 1;
    }

    std::size_t granularity = 2;
    while (lines.size() > 1) {
        bool changed = false;
        const std::size_t chunk_size = (lines.size() + granularity - 1) / granularity;
        for (std::size_t start = 0; start < lines.size();) {
            const std::size_t end = std::min(lines.size(), start + chunk_size);
            std::vector<std::string> candidate;
            candidate.reserve(lines.size() - (end - start));
            candidate.insert(candidate.end(), lines.begin(), lines.begin() + start);
            candidate.insert(candidate.end(), lines.begin() + end, lines.end());

            if (still_fails(candidate)) {
                lines.swap(candidate);
                granularity = 2;
                changed = true;
                break;
            }
            start = end;
        }

        if (!changed) {
            if (granularity >= lines.size()) {
                break;
            }
            granularity = std::min(lines.size(), granularity * 2);
        }
    }

    if (!write_lines(out_path, lines)) {
        std::cerr << "[reduce] error: cannot write reduced output: " << out_path.string() << "\n";
        std::error_code ec;
        fs::remove(tmp_path, ec);
        return 1;
    }

    std::error_code ec;
    fs::remove(tmp_path, ec);
    std::cout << "[reduce] reduced from " << opts.input << " to " << out_path.string()
              << " (" << lines.size() << " lines)\n";
    return 0;
}

static bool build_module_index_for_tooling(const Options& opts,
                                           frontend::ast::AstContext& ast_ctx,
                                           frontend::diag::DiagnosticEngine& diagnostics,
                                           frontend::modules::ModuleIndex& index,
                                           frontend::ast::ModuleId& root_out) {
    std::ifstream in(opts.input);
    if (!in.is_open()) {
        std::cerr << "[mod] error: cannot open input file: " << opts.input << "\n";
        return false;
    }
    std::string source((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    frontend::Lexer lexer(source, opts.input);
    ast_ctx.sources.push_back(lexer.source_file());
    frontend::parser::Parser parser(lexer, diagnostics, ast_ctx, opts.strict_parse);
    auto root = parser.parse_module();
    root_out = root;

    frontend::modules::LoadOptions load_opts;
    load_opts.stdlib_profile = opts.stdlib_profile;
    load_opts.allow_experimental = opts.allow_experimental;
    load_opts.warn_experimental = opts.warn_experimental;
    load_opts.deny_internal = opts.deny_internal;
    frontend::modules::load_modules(ast_ctx, root, diagnostics, opts.input, index, load_opts);
    frontend::modules::rewrite_member_access(ast_ctx, root, index, &diagnostics);
    return !diagnostics.has_errors();
}

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static std::unordered_set<std::string> reachable_from(
    const std::unordered_map<std::string, std::unordered_set<std::string>>& graph,
    const std::string& from
) {
    std::unordered_set<std::string> out;
    if (from.empty()) {
        for (const auto& kv : graph) {
            out.insert(kv.first);
            out.insert(kv.second.begin(), kv.second.end());
        }
        return out;
    }
    std::vector<std::string> stack{from};
    while (!stack.empty()) {
        auto n = stack.back();
        stack.pop_back();
        if (!out.insert(n).second) {
            continue;
        }
        auto it = graph.find(n);
        if (it == graph.end()) {
            continue;
        }
        for (const auto& to : it->second) {
            stack.push_back(to);
        }
    }
    return out;
}

static std::vector<std::vector<std::string>> detect_cycles(
    const std::unordered_map<std::string, std::unordered_set<std::string>>& graph
) {
    enum class Mark { White, Gray, Black };
    std::unordered_map<std::string, Mark> marks;
    std::vector<std::string> stack;
    std::vector<std::vector<std::string>> cycles;

    std::function<void(const std::string&)> dfs = [&](const std::string& node) {
        marks[node] = Mark::Gray;
        stack.push_back(node);
        auto it = graph.find(node);
        if (it != graph.end()) {
            for (const auto& next : it->second) {
                auto m = marks.find(next);
                Mark state = (m == marks.end()) ? Mark::White : m->second;
                if (state == Mark::White) {
                    dfs(next);
                } else if (state == Mark::Gray) {
                    std::vector<std::string> cyc;
                    auto pos = std::find(stack.begin(), stack.end(), next);
                    if (pos != stack.end()) {
                        cyc.insert(cyc.end(), pos, stack.end());
                        cyc.push_back(next);
                        cycles.push_back(std::move(cyc));
                    }
                }
            }
        }
        stack.pop_back();
        marks[node] = Mark::Black;
    };

    for (const auto& kv : graph) {
        if (marks.find(kv.first) == marks.end()) {
            dfs(kv.first);
        }
    }
    return cycles;
}

static int run_mod_graph(const Options& opts) {
    frontend::ast::AstContext ast_ctx;
    frontend::diag::DiagnosticEngine diagnostics(opts.lang);
    frontend::modules::ModuleIndex index;
    frontend::ast::ModuleId root = frontend::ast::kInvalidAstId;
    if (!build_module_index_for_tooling(opts, ast_ctx, diagnostics, index, root)) {
        emit_driver_diags(opts, diagnostics, std::cerr);
        return 1;
    }

    std::set<std::string> modules;
    auto visible = reachable_from(index.imports, opts.mod_graph_from);
    if (opts.mod_graph_from.empty()) {
        for (const auto& kv : index.module_files) {
            modules.insert(kv.first);
        }
    } else if (visible.empty()) {
        std::cerr << "[mod] error: --from module not found: " << opts.mod_graph_from << "\n";
        return 1;
    }
    for (const auto& m : visible) {
        modules.insert(m);
    }

    std::vector<std::pair<std::string, std::string>> edges;
    for (const auto& kv : index.imports) {
        if (!modules.count(kv.first)) {
            continue;
        }
        for (const auto& to : kv.second) {
            if (!modules.count(to)) {
                continue;
            }
            edges.push_back({kv.first, to});
        }
    }
    std::sort(edges.begin(), edges.end());

    auto filtered_cycles = detect_cycles(index.imports);
    std::vector<std::vector<std::string>> cycles;
    for (const auto& cyc : filtered_cycles) {
        bool keep = true;
        for (const auto& n : cyc) {
            if (!modules.count(n)) {
                keep = false;
                break;
            }
        }
        if (keep) {
            cycles.push_back(cyc);
        }
    }

    if (opts.mod_graph_json) {
        std::cout << "{\n";
        std::cout << "  \"from\": \"" << json_escape(opts.mod_graph_from) << "\",\n";
        std::cout << "  \"nodes\": [\n";
        std::size_t i = 0;
        for (const auto& mod : modules) {
            std::size_t loc = 0;
            if (auto it = index.module_loc.find(mod); it != index.module_loc.end()) loc = it->second;
            std::size_t exports = 0;
            if (auto p = index.path_to_prefix.find(mod); p != index.path_to_prefix.end()) {
                if (auto e = index.exports.find(p->second); e != index.exports.end()) exports = e->second.size();
            }
            std::size_t imports = 0;
            if (auto ii = index.imports.find(mod); ii != index.imports.end()) imports = ii->second.size();
            std::cout << "    {\"id\":\"" << json_escape(mod) << "\",\"loc\":" << loc
                      << ",\"exports\":" << exports << ",\"imports\":" << imports << "}";
            if (++i < modules.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ],\n";
        std::cout << "  \"edges\": [\n";
        for (std::size_t j = 0; j < edges.size(); ++j) {
            std::cout << "    {\"from\":\"" << json_escape(edges[j].first)
                      << "\",\"to\":\"" << json_escape(edges[j].second) << "\"}";
            if (j + 1 < edges.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ],\n";
        std::cout << "  \"cycles\": [\n";
        for (std::size_t c = 0; c < cycles.size(); ++c) {
            std::cout << "    [";
            for (std::size_t k = 0; k < cycles[c].size(); ++k) {
                if (k) std::cout << ",";
                std::cout << "\"" << json_escape(cycles[c][k]) << "\"";
            }
            std::cout << "]";
            if (c + 1 < cycles.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ]\n";
        std::cout << "}\n";
        return 0;
    }

    std::cout << "module_graph:\n";
    for (const auto& mod : modules) {
        std::size_t loc = 0;
        if (auto it = index.module_loc.find(mod); it != index.module_loc.end()) {
            loc = it->second;
        }
        std::size_t exports = 0;
        if (auto p = index.path_to_prefix.find(mod); p != index.path_to_prefix.end()) {
            if (auto e = index.exports.find(p->second); e != index.exports.end()) {
                exports = e->second.size();
            }
        }
        std::size_t imports = 0;
        if (auto i = index.imports.find(mod); i != index.imports.end()) {
            imports = i->second.size();
        }
        std::cout << "  - " << mod << " (loc=" << loc
                  << ", exports=" << exports
                  << ", imports=" << imports << ")\n";
    }

    std::cout << "edges:\n";
    for (const auto& e : edges) {
        std::cout << "  " << e.first << " -> " << e.second << "\n";
    }

    if (!cycles.empty()) {
        std::cout << "cycles:\n";
        for (const auto& cyc : cycles) {
            std::cout << "  ";
            for (std::size_t i = 0; i < cyc.size(); ++i) {
                if (i) {
                    std::cout << " -> ";
                }
                std::cout << cyc[i];
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "cycles: none\n";
    }
    return 0;
}

static int run_mod_doctor(const Options& opts) {
    frontend::ast::AstContext ast_ctx;
    frontend::diag::DiagnosticEngine diagnostics(opts.lang);
    frontend::modules::ModuleIndex index;
    frontend::ast::ModuleId root_id = frontend::ast::kInvalidAstId;
    if (!build_module_index_for_tooling(opts, ast_ctx, diagnostics, index, root_id)) {
        emit_driver_diags(opts, diagnostics, std::cerr);
        return 1;
    }

    std::size_t issues = 0;
    auto& root = ast_ctx.get<frontend::ast::Module>(root_id);
    std::unordered_map<std::string, frontend::ast::SourceSpan> alias_span;
    std::unordered_set<std::string> aliases_used;

    for (auto decl_id : root.decls) {
        if (decl_id == frontend::ast::kInvalidAstId) {
            continue;
        }
        const auto& decl = ast_ctx.get<frontend::ast::Decl>(decl_id);
        if (decl.kind == frontend::ast::NodeKind::UseDecl) {
            const auto& u = static_cast<const frontend::ast::UseDecl&>(decl);
            if (u.path.relative_depth > 0) {
                ++issues;
                std::cout << "[doctor] non-canonical import path in use\n";
                if (opts.mod_doctor_fix) {
                    std::string canonical;
                    for (std::size_t i = 0; i < u.path.parts.size(); ++i) {
                        if (i) canonical += "/";
                        canonical += u.path.parts[i].name;
                    }
                    std::cout << "  fix: use " << canonical
                              << (u.alias ? " as " + u.alias->name : " as " + u.path.parts.back().name) << "\n";
                }
            }
            if (!u.alias.has_value()) {
                ++issues;
                std::cout << "[doctor] missing alias in use\n";
                if (opts.mod_doctor_fix && !u.path.parts.empty()) {
                    std::string canonical;
                    for (std::size_t i = 0; i < u.path.parts.size(); ++i) {
                        if (i) canonical += "/";
                        canonical += u.path.parts[i].name;
                    }
                    std::cout << "  fix: use " << canonical << " as " << u.path.parts.back().name << "\n";
                }
            } else {
                alias_span[u.alias->name] = u.alias->span;
            }
            if (u.is_glob && opts.mod_doctor_fix) {
                std::string key;
                for (std::size_t i = 0; i < u.path.parts.size(); ++i) {
                    if (i) key += "/";
                    key += u.path.parts[i].name;
                }
                if (auto pfx = index.path_to_prefix.find(key); pfx != index.path_to_prefix.end()) {
                    if (auto ex = index.exports.find(pfx->second); ex != index.exports.end()) {
                        std::vector<std::string> names(ex->second.begin(), ex->second.end());
                        std::sort(names.begin(), names.end());
                        std::cout << "  fix: replace glob with explicit imports:\n";
                        for (const auto& n : names) {
                            std::cout << "    use " << key << "/" << n << " as " << n << "\n";
                        }
                    }
                }
            }
        } else if (decl.kind == frontend::ast::NodeKind::PullDecl) {
            const auto& p = static_cast<const frontend::ast::PullDecl&>(decl);
            if (p.path.relative_depth > 0) {
                ++issues;
                std::cout << "[doctor] non-canonical import path in pull\n";
            }
            if (!p.alias.has_value()) {
                ++issues;
                std::cout << "[doctor] missing alias in pull\n";
                if (opts.mod_doctor_fix && !p.path.parts.empty()) {
                    std::string canonical;
                    for (std::size_t i = 0; i < p.path.parts.size(); ++i) {
                        if (i) canonical += "/";
                        canonical += p.path.parts[i].name;
                    }
                    std::cout << "  fix: pull " << canonical << " as " << p.path.parts.back().name << "\n";
                }
            } else {
                alias_span[p.alias->name] = p.alias->span;
            }
        }
    }

    auto note_ident_usage = [&](const frontend::ast::ExprId expr_id, auto&& self) -> void {
        if (expr_id == frontend::ast::kInvalidAstId) return;
        const auto& expr = ast_ctx.get<frontend::ast::Expr>(expr_id);
        if (expr.kind == frontend::ast::NodeKind::IdentExpr) {
            const auto& e = static_cast<const frontend::ast::IdentExpr&>(expr);
            if (alias_span.count(e.ident.name)) {
                aliases_used.insert(e.ident.name);
            }
        } else if (expr.kind == frontend::ast::NodeKind::MemberExpr) {
            const auto& e = static_cast<const frontend::ast::MemberExpr&>(expr);
            self(e.base, self);
        } else if (expr.kind == frontend::ast::NodeKind::CallNoParenExpr) {
            const auto& e = static_cast<const frontend::ast::CallNoParenExpr&>(expr);
            if (alias_span.count(e.callee.name)) {
                aliases_used.insert(e.callee.name);
            }
            self(e.arg, self);
        } else if (expr.kind == frontend::ast::NodeKind::InvokeExpr) {
            const auto& e = static_cast<const frontend::ast::InvokeExpr&>(expr);
            self(e.callee_expr, self);
            for (auto a : e.args) self(a, self);
        } else if (expr.kind == frontend::ast::NodeKind::UnaryExpr) {
            const auto& e = static_cast<const frontend::ast::UnaryExpr&>(expr);
            self(e.expr, self);
        } else if (expr.kind == frontend::ast::NodeKind::BinaryExpr) {
            const auto& e = static_cast<const frontend::ast::BinaryExpr&>(expr);
            self(e.lhs, self);
            self(e.rhs, self);
        } else if (expr.kind == frontend::ast::NodeKind::ListExpr) {
            const auto& e = static_cast<const frontend::ast::ListExpr&>(expr);
            for (auto it : e.items) self(it, self);
        }
    };
    auto walk_stmt = [&](const frontend::ast::StmtId stmt_id, auto&& self) -> void {
        if (stmt_id == frontend::ast::kInvalidAstId) return;
        const auto& stmt = ast_ctx.get<frontend::ast::Stmt>(stmt_id);
        using K = frontend::ast::NodeKind;
        if (stmt.kind == K::BlockStmt) {
            const auto& s = static_cast<const frontend::ast::BlockStmt&>(stmt);
            for (auto st : s.stmts) self(st, self);
        } else if (stmt.kind == K::LetStmt) {
            const auto& s = static_cast<const frontend::ast::LetStmt&>(stmt);
            note_ident_usage(s.initializer, note_ident_usage);
        } else if (stmt.kind == K::ExprStmt) {
            const auto& s = static_cast<const frontend::ast::ExprStmt&>(stmt);
            note_ident_usage(s.expr, note_ident_usage);
        } else if (stmt.kind == K::ReturnStmt) {
            const auto& s = static_cast<const frontend::ast::ReturnStmt&>(stmt);
            note_ident_usage(s.expr, note_ident_usage);
        } else if (stmt.kind == K::IfStmt) {
            const auto& s = static_cast<const frontend::ast::IfStmt&>(stmt);
            note_ident_usage(s.cond, note_ident_usage);
            self(s.then_block, self);
            self(s.else_block, self);
        }
    };
    for (auto decl_id : root.decls) {
        if (decl_id == frontend::ast::kInvalidAstId) continue;
        const auto& decl = ast_ctx.get<frontend::ast::Decl>(decl_id);
        if (decl.kind == frontend::ast::NodeKind::ProcDecl) {
            const auto& p = static_cast<const frontend::ast::ProcDecl&>(decl);
            walk_stmt(p.body, walk_stmt);
        } else if (decl.kind == frontend::ast::NodeKind::EntryDecl) {
            const auto& e = static_cast<const frontend::ast::EntryDecl&>(decl);
            walk_stmt(e.body, walk_stmt);
        }
    }
    for (const auto& kv : alias_span) {
        if (aliases_used.count(kv.first) == 0) {
            ++issues;
            std::cout << "[doctor] unused alias: " << kv.first << "\n";
        }
    }

    for (const auto& kv : index.imports) {
        if (opts.max_imports > 0 && static_cast<int>(kv.second.size()) > opts.max_imports) {
            ++issues;
            std::cout << "[doctor] fan-out too high in " << kv.first
                      << ": imports=" << kv.second.size()
                      << " > max=" << opts.max_imports << "\n";
        }
        std::unordered_map<std::string, std::string> owner;
        for (const auto& dep : kv.second) {
            auto pit = index.path_to_prefix.find(dep);
            if (pit == index.path_to_prefix.end()) continue;
            auto eit = index.exports.find(pit->second);
            if (eit == index.exports.end()) continue;
            for (const auto& sym : eit->second) {
                auto it = owner.find(sym);
                if (it != owner.end() && it->second != dep) {
                    ++issues;
                    std::cout << "[doctor] export collision in " << kv.first
                              << ": symbol '" << sym << "' from " << it->second
                              << " and " << dep << "\n";
                } else {
                    owner[sym] = dep;
                }
            }
        }
    }

    if (issues == 0) {
        std::cout << "[doctor] OK\n";
        return 0;
    }
    std::cout << "[doctor] issues: " << issues << "\n";
    return 1;
}

static int run_mod_api_diff(const Options& opts) {
    auto old_input = opts.api_diff_old.empty() ? opts.input : opts.api_diff_old;
    auto new_input = opts.api_diff_new.empty() ? opts.input : opts.api_diff_new;
    if (old_input.empty() || new_input.empty()) {
        std::cerr << "[contract-diff] error: provide --old and --new inputs\n";
        return 1;
    }

    Options left_opts = opts;
    left_opts.input = old_input;
    Options right_opts = opts;
    right_opts.input = new_input;

    frontend::ast::AstContext left_ctx;
    frontend::diag::DiagnosticEngine left_diag(opts.lang);
    frontend::modules::ModuleIndex left_idx;
    frontend::ast::ModuleId left_root = frontend::ast::kInvalidAstId;
    if (!build_module_index_for_tooling(left_opts, left_ctx, left_diag, left_idx, left_root)) {
        emit_driver_diags(opts, left_diag, std::cerr);
        return 1;
    }

    frontend::ast::AstContext right_ctx;
    frontend::diag::DiagnosticEngine right_diag(opts.lang);
    frontend::modules::ModuleIndex right_idx;
    frontend::ast::ModuleId right_root = frontend::ast::kInvalidAstId;
    if (!build_module_index_for_tooling(right_opts, right_ctx, right_diag, right_idx, right_root)) {
        emit_driver_diags(opts, right_diag, std::cerr);
        return 1;
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> l_exp;
    for (const auto& kv : left_idx.path_to_prefix) {
        auto eit = left_idx.exports.find(kv.second);
        if (eit != left_idx.exports.end()) {
            l_exp[kv.first] = eit->second;
        }
    }
    std::unordered_map<std::string, std::unordered_set<std::string>> r_exp;
    for (const auto& kv : right_idx.path_to_prefix) {
        auto eit = right_idx.exports.find(kv.second);
        if (eit != right_idx.exports.end()) {
            r_exp[kv.first] = eit->second;
        }
    }

    std::vector<std::string> removed_items;
    std::vector<std::string> added_items;
    bool breaking = false;
    for (const auto& kv : l_exp) {
        auto rit = r_exp.find(kv.first);
        if (rit == r_exp.end()) {
            removed_items.push_back("module:" + kv.first);
            breaking = true;
            continue;
        }
        for (const auto& sym : kv.second) {
            if (!rit->second.count(sym)) {
                removed_items.push_back("symbol:" + kv.first + "::" + sym);
                breaking = true;
            }
        }
    }
    for (const auto& kv : r_exp) {
        auto lit = l_exp.find(kv.first);
        if (lit == l_exp.end()) {
            added_items.push_back("module:" + kv.first);
            continue;
        }
        for (const auto& sym : kv.second) {
            if (!lit->second.count(sym)) {
                added_items.push_back("symbol:" + kv.first + "::" + sym);
            }
        }
    }
    std::sort(removed_items.begin(), removed_items.end());
    std::sort(added_items.begin(), added_items.end());
    if (opts.mod_graph_json) {
        std::cout << "{\n";
        std::cout << "  \"old\": \"" << json_escape(old_input) << "\",\n";
        std::cout << "  \"new\": \"" << json_escape(new_input) << "\",\n";
        std::cout << "  \"status\": \"" << (breaking ? "breaking" : "compatible") << "\",\n";
        std::cout << "  \"removed\": [\n";
        for (std::size_t i = 0; i < removed_items.size(); ++i) {
            std::cout << "    \"" << json_escape(removed_items[i]) << "\"";
            if (i + 1 < removed_items.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ],\n";
        std::cout << "  \"added\": [\n";
        for (std::size_t i = 0; i < added_items.size(); ++i) {
            std::cout << "    \"" << json_escape(added_items[i]) << "\"";
            if (i + 1 < added_items.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ]\n";
        std::cout << "}\n";
        return breaking ? 1 : 0;
    }
    for (const auto& it : removed_items) {
        if (it.rfind("module:", 0) == 0) {
            std::cout << "[contract-diff] removed module: " << it.substr(7) << "\n";
        } else {
            std::cout << "[contract-diff] removed symbol: " << it.substr(7) << "\n";
        }
    }
    for (const auto& it : added_items) {
        if (it.rfind("module:", 0) == 0) {
            std::cout << "[contract-diff] added module: " << it.substr(7) << "\n";
        } else {
            std::cout << "[contract-diff] added symbol: " << it.substr(7) << "\n";
        }
    }
    if (breaking) {
        std::cout << "[contract-diff] BREAKING\n";
        return 1;
    }
    std::cout << "[contract-diff] OK\n";
    return 0;
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

    if (opts.clean_cache) {
        return run_clean_cache();
    }

    const bool api_diff_without_input =
        opts.mod_api_diff && !opts.api_diff_old.empty() && !opts.api_diff_new.empty();
    if (opts.input.empty() && !api_diff_without_input) {
        if (argc == 1) {
            print_onboarding_summary();
            return 0;
        }
        std::cerr << "[driver] error: no input file provided\n";
        print_help();
        return 1;
    }

    std::string stage_error;
    if (!apply_stage_override(opts, stage_error)) {
        std::cerr << "[driver] error: " << stage_error << "\n";
        return 1;
    }

    if (!frontend::modules::is_valid_stdlib_profile(opts.stdlib_profile)) {
        std::cerr << "[driver] error: invalid --runtime-profile/--stdlib-profile '" << opts.stdlib_profile
                  << "' (expected minimal|full|kernel|arduino)\n";
        return 1;
    }

    if (opts.reduce_reproducer) {
        return run_reduce(opts);
    }

    if (opts.mod_graph) {
        return run_mod_graph(opts);
    }
    if (opts.mod_doctor) {
        return run_mod_doctor(opts);
    }
    if (opts.mod_api_diff) {
        return run_mod_api_diff(opts);
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
