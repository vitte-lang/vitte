#include "strict_core_guard.hpp"

#include "../frontend/diagnostics_messages.hpp"
#include "../frontend/lexer.hpp"
#include "../frontend/token_tables.hpp"

namespace vitte::driver {

bool apply_strict_core_guard(const Options& opts,
                             const std::string& source,
                             const std::string& path,
                             frontend::diag::DiagnosticEngine& diagnostics) {
    if (!opts.strict_core) {
        return true;
    }

    frontend::Lexer lexer(source, path);
    bool ok = true;
    frontend::Token prev{frontend::TokenKind::Eof, "", {}};
    while (true) {
        frontend::Token tok = lexer.next();
        if (tok.kind == frontend::TokenKind::Eof) {
            break;
        }
        if (frontend::tokens::is_forbidden_in_core(tok.kind)) {
            frontend::diag::error(diagnostics, frontend::diag::DiagId::CoreForbiddenTopLevelSyntax, tok.span);
            diagnostics.note("forbidden token in strict-core mode: '" + tok.text + "'", tok.span);
            ok = false;
        }
        if (prev.kind == frontend::TokenKind::Dot &&
            tok.kind == frontend::TokenKind::Ident &&
            tok.text == "end") {
            frontend::diag::error(diagnostics, frontend::diag::DiagId::CoreForbiddenTopLevelSyntax, tok.span);
            diagnostics.note("legacy '.end' blocks are forbidden in strict-core mode", tok.span);
            ok = false;
        }
        prev = tok;
    }
    return ok;
}

} // namespace vitte::driver

