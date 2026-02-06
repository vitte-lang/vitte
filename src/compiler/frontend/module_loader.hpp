#pragma once

#include "ast.hpp"
#include "diagnostics.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace vitte::frontend::modules {

struct ModuleIndex {
    std::unordered_map<std::string, std::string> path_to_prefix;
    std::unordered_map<std::string, std::unordered_set<std::string>> exports;
};

std::string module_prefix(const ast::ModulePath& path);

bool load_modules(ast::AstContext& ctx,
                  ast::ModuleId root,
                  diag::DiagnosticEngine& diagnostics,
                  const std::string& entry_path,
                  ModuleIndex& index);

void rewrite_member_access(ast::AstContext& ctx,
                           ast::ModuleId root,
                           const ModuleIndex& index);

} // namespace vitte::frontend::modules
