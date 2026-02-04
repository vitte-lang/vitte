// ============================================================
// diagnostics.cpp — Vitte Compiler
// Frontend diagnostics implementation
// ============================================================

#include "diagnostics.hpp"

#include <cassert>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace vitte::frontend::diag {

// ------------------------------------------------------------
// Severity
// ------------------------------------------------------------

const char* to_string(Severity s) {
    switch (s) {
        case Severity::Note:    return "note";
        case Severity::Warning: return "warning";
        case Severity::Error:   return "error";
        case Severity::Fatal:   return "fatal";
        default:                return "unknown";
    }
}

// ------------------------------------------------------------
// Diagnostic
// ------------------------------------------------------------

Diagnostic::Diagnostic(
    Severity sev,
    std::string msg,
    SourceSpan sp)
    : severity(sev),
      message(std::move(msg)),
      span(sp) {}

void Diagnostic::add_note(std::string msg) {
    notes.emplace_back(std::move(msg));
}

// ------------------------------------------------------------
// DiagnosticEngine
// ------------------------------------------------------------

DiagnosticEngine::DiagnosticEngine()
    : error_count_(0),
      warning_count_(0) {}

void DiagnosticEngine::emit(Diagnostic d) {
    switch (d.severity) {
        case Severity::Warning:
            ++warning_count_;
            break;
        case Severity::Error:
        case Severity::Fatal:
            ++error_count_;
            break;
        default:
            break;
    }
    diagnostics_.push_back(std::move(d));
}

bool DiagnosticEngine::has_errors() const {
    return error_count_ > 0;
}

std::size_t DiagnosticEngine::error_count() const {
    return error_count_;
}

std::size_t DiagnosticEngine::warning_count() const {
    return warning_count_;
}

const std::vector<Diagnostic>& DiagnosticEngine::all() const {
    return diagnostics_;
}

// ------------------------------------------------------------
// Rendering helpers
// ------------------------------------------------------------

static void render_location(
    std::ostream& os,
    const SourceSpan& span)
{
    if (!span.is_valid()) {
        return;
    }

    os << span.file->path
       << ":" << span.start
       << ":" << span.end;
}

static void render_snippet(
    std::ostream& os,
    const SourceSpan& span)
{
    if (!span.is_valid()) {
        return;
    }

    const std::string& text = span.file->content;
    if (span.start >= text.size() || span.end > text.size()) {
        return;
    }

    os << "\n  --> ";
    os << text.substr(span.start, span.end - span.start);
}

// ------------------------------------------------------------
// Diagnostic rendering
// ------------------------------------------------------------

void render(const Diagnostic& d, std::ostream& os) {
    os << to_string(d.severity) << ": " << d.message;

    if (d.span.is_valid()) {
        os << "\n  --> ";
        render_location(os, d.span);
        render_snippet(os, d.span);
    }

    for (const auto& note : d.notes) {
        os << "\n  note: " << note;
    }

    os << "\n";
}

void render_all(
    const DiagnosticEngine& engine,
    std::ostream& os)
{
    for (const auto& d : engine.all()) {
        render(d, os);
    }
}

// ------------------------------------------------------------
// Convenience emitters
// ------------------------------------------------------------

void DiagnosticEngine::note(
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Note,
        std::move(msg),
        sp));
}

void DiagnosticEngine::warning(
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Warning,
        std::move(msg),
        sp));
}

void DiagnosticEngine::error(
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Error,
        std::move(msg),
        sp));
}

void DiagnosticEngine::fatal(
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Fatal,
        std::move(msg),
        sp));
}

} // namespace vitte::frontend::diag