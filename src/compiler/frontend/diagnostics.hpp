// ============================================================
// diagnostics.hpp — Vitte Compiler
// Frontend diagnostics interface
// ============================================================

#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "ast.hpp"

namespace vitte::frontend::diag {

// ------------------------------------------------------------
// Severity
// ------------------------------------------------------------

enum class Severity {
    Note,
    Warning,
    Error,
    Fatal,
};

const char* to_string(Severity severity);

// ------------------------------------------------------------
// Diagnostic
// ------------------------------------------------------------

struct Diagnostic {
    Severity severity;
    std::string message;
    SourceSpan span;
    std::vector<std::string> notes;

    Diagnostic(
        Severity severity,
        std::string message,
        SourceSpan span);

    void add_note(std::string message);
};

// ------------------------------------------------------------
// Diagnostic engine
// ------------------------------------------------------------

class DiagnosticEngine {
public:
    DiagnosticEngine();

    // emission
    void emit(Diagnostic diagnostic);

    void note(std::string message, SourceSpan span);
    void warning(std::string message, SourceSpan span);
    void error(std::string message, SourceSpan span);
    void fatal(std::string message, SourceSpan span);

    // state
    bool has_errors() const;
    std::size_t error_count() const;
    std::size_t warning_count() const;

    const std::vector<Diagnostic>& all() const;

private:
    std::vector<Diagnostic> diagnostics_;
    std::size_t error_count_;
    std::size_t warning_count_;
};

// ------------------------------------------------------------
// Rendering helpers
// ------------------------------------------------------------

void render(const Diagnostic& diagnostic, std::ostream& os);
void render_all(const DiagnosticEngine& engine, std::ostream& os);

} // namespace vitte::frontend::diag