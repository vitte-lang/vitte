#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

#include <string>
#include <ostream>
#include <unordered_map>
#include <unordered_set>

namespace vitte::frontend::modules {

struct ModuleIndex {
    std::unordered_map<std::string, std::string> path_to_prefix;
    std::unordered_map<std::string, std::unordered_set<std::string>> exports;
    std::unordered_map<std::string, std::unordered_set<std::string>> imports;
    std::unordered_map<std::string, std::string> module_files;
    std::unordered_map<std::string, std::size_t> module_loc;
};

struct LoadOptions {
    std::string stdlib_profile = "full";
    bool allow_experimental = false;
    bool warn_experimental = false;
    bool deny_internal = true;
};

std::string module_prefix(const ast::ModulePath& path);
bool is_valid_stdlib_profile(const std::string& profile);
bool is_stdlib_path_allowed(const ast::ModulePath& path, const std::string& profile);
std::string normalized_stdlib_path(const ast::ModulePath& path);

bool load_modules(ast::AstContext& ctx,
                  ast::ModuleId root,
                  diag::DiagnosticEngine& diagnostics,
                  const std::string& entry_path,
                  ModuleIndex& index,
                  const LoadOptions& options = {});

void rewrite_member_access(ast::AstContext& ctx,
                           ast::ModuleId root,
                           const ModuleIndex& index,
                           diag::DiagnosticEngine* diagnostics = nullptr);

void dump_stdlib_map(std::ostream& os, const ModuleIndex& index);
void dump_module_index_json(std::ostream& os,
                            const ModuleIndex& index,
                            const std::string& profile,
                            bool allow_experimental);

} // namespace vitte::frontend::modules
