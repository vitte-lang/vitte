#pragma once
#include <vector>

#include "lexer.hpp"
#include "ast.hpp"

namespace vitte::frontend {

/* -------------------------------------------------
 * Parser entry point
 * ------------------------------------------------- */
AstProgram parse(const std::vector<Token>& tokens);

} // namespace vitte::frontend
