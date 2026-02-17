// ============================================================
// diagnostics.cpp — Vitte Compiler
// Frontend diagnostics implementation
// ============================================================

#include "diagnostics.hpp"
#include "diagnostics_messages.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
      code(),
      message(std::move(msg)),
      span(sp) {}

Diagnostic::Diagnostic(
    Severity sev,
    std::string code,
    std::string msg,
    SourceSpan sp)
    : severity(sev),
      code(std::move(code)),
      message(std::move(msg)),
      span(sp) {}

void Diagnostic::add_note(std::string msg) {
    notes.emplace_back(std::move(msg));
}

// ------------------------------------------------------------
// Localization
// ------------------------------------------------------------

static std::string normalize_lang(std::string lang) {
    if (lang.empty()) {
        return lang;
    }
    std::string out;
    for (char c : lang) {
        if (c == '.' || c == '@') {
            break;
        }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

static std::string lang_primary(std::string lang) {
    auto pos = lang.find('_');
    if (pos == std::string::npos) {
        pos = lang.find('-');
    }
    if (pos == std::string::npos) {
        return lang;
    }
    return lang.substr(0, pos);
}

static std::string message_key(std::string_view message) {
    std::string key;
    bool prev_underscore = false;
    for (char c : message) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) {
            key.push_back(static_cast<char>(std::tolower(uc)));
            prev_underscore = false;
        } else if (!prev_underscore) {
            key.push_back('_');
            prev_underscore = true;
        }
    }
    while (!key.empty() && key.front() == '_') {
        key.erase(key.begin());
    }
    while (!key.empty() && key.back() == '_') {
        key.pop_back();
    }
    return key;
}

bool Localization::load(const std::string& base_dir, const std::string& lang, const std::string& filename) {
    namespace fs = std::filesystem;
    std::string norm = normalize_lang(lang);
    std::string primary = lang_primary(norm);
    std::vector<std::string> candidates;
    if (!norm.empty()) {
        candidates.push_back(norm);
    }
    if (primary != norm && !primary.empty()) {
        candidates.push_back(primary);
    }

    for (const auto& code : candidates) {
        fs::path path = fs::path(base_dir) / code / filename;
        std::ifstream in(path);
        if (!in.is_open()) {
            continue;
        }
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            if (line[0] == '#') {
                continue;
            }
            auto eq = line.find('=');
            if (eq == std::string::npos) {
                continue;
            }
            auto key = line.substr(0, eq);
            auto value = line.substr(eq + 1);
            auto ltrim = [](std::string& s) {
                auto it = std::find_if_not(s.begin(), s.end(), ::isspace);
                s.erase(s.begin(), it);
            };
            auto rtrim = [](std::string& s) {
                auto it = std::find_if_not(s.rbegin(), s.rend(), ::isspace);
                s.erase(it.base(), s.end());
            };
            ltrim(key);
            rtrim(key);
            ltrim(value);
            rtrim(value);
            if (!key.empty() && !value.empty()) {
                table_[key] = value;
            }
        }
        return true;
    }
    return false;
}

std::string Localization::translate(std::string_view code, std::string_view message) const {
    if (table_.empty()) {
        return std::string(message);
    }
    if (!code.empty()) {
        auto it_code = table_.find(std::string(code));
        if (it_code != table_.end()) {
            return it_code->second;
        }
    }
    std::string key = message_key(message);
    auto it = table_.find(key);
    if (it == table_.end()) {
        return std::string(message);
    }
    return it->second;
}

std::string Localization::lookup(std::string_view key) const {
    if (table_.empty()) {
        return {};
    }
    auto it = table_.find(std::string(key));
    if (it == table_.end()) {
        return {};
    }
    return it->second;
}

// ------------------------------------------------------------
// DiagnosticEngine
// ------------------------------------------------------------

DiagnosticEngine::DiagnosticEngine(std::string lang)
    : error_count_(0),
      warning_count_(0) {
    if (lang.empty()) {
        const char* env = std::getenv("LANG");
        if (!env || !*env) {
            env = std::getenv("LC_ALL");
        }
        if (env && *env) {
            lang = env;
        }
    }
    if (lang.empty()) {
        lang = "en";
    }
    localization_.load("locales", lang);
}

void DiagnosticEngine::emit(Diagnostic d) {
    d.message = localization_.translate(d.code, d.message);
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

    const std::string& text = span.file->content;
    std::size_t line = 1;
    std::size_t col = 1;
    for (std::size_t i = 0; i < span.start && i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }

    std::filesystem::path path(span.file->path);
    os << path.filename().string()
       << ":" << line
       << ":" << col;
}

static std::string location_string(const SourceSpan& span) {
    if (!span.is_valid()) {
        return "<unknown>:0:0";
    }
    const std::string& text = span.file->content;
    std::size_t line = 1;
    std::size_t col = 1;
    for (std::size_t i = 0; i < span.start && i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }
    std::filesystem::path path(span.file->path);
    std::ostringstream oss;
    oss << path.filename().string() << ":" << line << ":" << col;
    return oss.str();
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

    std::size_t line = 1;
    std::size_t col = 1;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i < span.start && i < text.size(); ++i) {
        if (text[i] == '\n') {
            ++line;
            col = 1;
            line_start = i + 1;
        } else {
            ++col;
        }
    }
    std::size_t line_end = line_start;
    while (line_end < text.size() && text[line_end] != '\n' && text[line_end] != '\r') {
        ++line_end;
    }
    std::string line_text = text.substr(line_start, line_end - line_start);

    std::size_t caret_start = col > 0 ? col - 1 : 0;
    std::size_t caret_len = span.end > span.start ? (span.end - span.start) : 1;
    if (line_start + caret_start + caret_len > line_end) {
        if (line_end > line_start + caret_start) {
            caret_len = line_end - (line_start + caret_start);
        } else {
            caret_len = 1;
        }
    }

    auto digits = [](std::size_t n) {
        std::size_t d = 1;
        while (n >= 10) {
            n /= 10;
            ++d;
        }
        return d;
    };
    std::size_t gutter = digits(line);

    os << "\n  " << std::string(gutter, ' ') << " |";
    os << "\n  " << line << " | " << line_text;
    os << "\n  " << std::string(gutter, ' ') << " | "
       << std::string(caret_start, ' ')
       << std::string(caret_len, '^');
}

// ------------------------------------------------------------
// Diagnostic rendering
// ------------------------------------------------------------

void render(const Diagnostic& d, std::ostream& os) {
    os << to_string(d.severity);
    if (!d.code.empty()) {
        os << "[" << d.code << "]";
    }
    os << ": " << d.message;

    if (d.span.is_valid()) {
        os << "\n  --> ";
        render_location(os, d.span);
        render_snippet(os, d.span);
    }

    for (const auto& note : d.notes) {
        os << "\n  note: " << note;
    }

    if (!d.code.empty()) {
        DiagId id{};
        if (diag_id_from_code(d.code, &id)) {
            const char* anchor = diag_doc_anchor(id);
            if (anchor && *anchor) {
                os << "\n  note: see docs/errors.md#" << anchor;
            }
        }
    }

    os << "\n";
}

void render_all(
    const DiagnosticEngine& engine,
    std::ostream& os,
    bool deterministic)
{
    std::vector<std::reference_wrapper<const Diagnostic>> ordered;
    ordered.reserve(engine.all().size());
    for (const auto& d : engine.all()) {
        ordered.push_back(std::cref(d));
    }
    if (deterministic) {
        std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
            const auto& da = a.get();
            const auto& db = b.get();
            const std::string fa = da.span.is_valid() ? std::filesystem::path(da.span.file->path).filename().string() : "";
            const std::string fb = db.span.is_valid() ? std::filesystem::path(db.span.file->path).filename().string() : "";
            if (fa != fb) return fa < fb;
            if (da.span.start != db.span.start) return da.span.start < db.span.start;
            if (da.code != db.code) return da.code < db.code;
            return da.message < db.message;
        });
    }
    for (const auto& dref : ordered) {
        const auto& d = dref.get();
        render(d, os);
    }
}

static std::string json_escape(const std::string& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char c : in) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

void render_all_json(const DiagnosticEngine& engine, std::ostream& os, bool pretty, bool deterministic) {
    if (pretty) {
        os << "{\n  \"diag_schema\": 1,\n  \"diagnostics\": [\n";
    } else {
        os << "{\"diag_schema\":1,\"diagnostics\":[";
    }
    std::vector<std::reference_wrapper<const Diagnostic>> all;
    all.reserve(engine.all().size());
    for (const auto& d : engine.all()) {
        all.push_back(std::cref(d));
    }
    if (deterministic) {
        std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
            const auto& da = a.get();
            const auto& db = b.get();
            const std::string fa = da.span.is_valid() ? std::filesystem::path(da.span.file->path).filename().string() : "";
            const std::string fb = db.span.is_valid() ? std::filesystem::path(db.span.file->path).filename().string() : "";
            if (fa != fb) return fa < fb;
            if (da.span.start != db.span.start) return da.span.start < db.span.start;
            if (da.code != db.code) return da.code < db.code;
            return da.message < db.message;
        });
    }
    for (std::size_t i = 0; i < all.size(); ++i) {
        const auto& d = all[i].get();
        if (pretty) {
            os << "    {\n";
            os << "      \"severity\": \"" << json_escape(to_string(d.severity)) << "\",\n";
            os << "      \"code\": \"" << json_escape(d.code) << "\",\n";
            os << "      \"message\": \"" << json_escape(d.message) << "\",\n";
        } else {
            os << "{\"severity\":\"" << json_escape(to_string(d.severity))
               << "\",\"code\":\"" << json_escape(d.code)
               << "\",\"message\":\"" << json_escape(d.message) << "\",";
        }
        if (d.span.is_valid()) {
            const std::string file_name = std::filesystem::path(d.span.file->path).filename().string();
            if (pretty) {
                os << "      \"file\": \"" << json_escape(file_name) << "\",\n";
                os << "      \"start\": " << d.span.start << ",\n";
                os << "      \"end\": " << d.span.end << "\n";
            } else {
                os << "\"file\":\"" << json_escape(file_name) << "\","
                   << "\"start\":" << d.span.start << ","
                   << "\"end\":" << d.span.end;
            }
        } else {
            if (pretty) {
                os << "      \"file\": \"\",\n";
                os << "      \"start\": 0,\n";
                os << "      \"end\": 0\n";
            } else {
                os << "\"file\":\"\",\"start\":0,\"end\":0";
            }
        }
        if (pretty) {
            os << "    }";
        } else {
            os << "}";
        }
        if (i + 1 < all.size()) {
            os << ",";
        }
        if (pretty) {
            os << "\n";
        }
    }
    if (pretty) {
        os << "  ]\n}\n";
    } else {
        os << "]}\n";
    }
}

void render_all_code_only(const DiagnosticEngine& engine, std::ostream& os, bool deterministic) {
    std::vector<std::reference_wrapper<const Diagnostic>> ordered;
    ordered.reserve(engine.all().size());
    for (const auto& d : engine.all()) {
        ordered.push_back(std::cref(d));
    }
    if (deterministic) {
        std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
            const auto& da = a.get();
            const auto& db = b.get();
            const std::string fa = da.span.is_valid() ? std::filesystem::path(da.span.file->path).filename().string() : "";
            const std::string fb = db.span.is_valid() ? std::filesystem::path(db.span.file->path).filename().string() : "";
            if (fa != fb) return fa < fb;
            if (da.span.start != db.span.start) return da.span.start < db.span.start;
            if (da.code != db.code) return da.code < db.code;
            return da.message < db.message;
        });
    }
    for (const auto& dref : ordered) {
        const auto& d = dref.get();
        os << location_string(d.span) << " ";
        if (!d.code.empty()) {
            os << d.code;
        } else {
            os << "E0000";
        }
        os << "\n";
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

void DiagnosticEngine::note_code(
    std::string code,
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Note,
        std::move(code),
        std::move(msg),
        sp));
}

void DiagnosticEngine::warning_code(
    std::string code,
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Warning,
        std::move(code),
        std::move(msg),
        sp));
}

void DiagnosticEngine::error_code(
    std::string code,
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Error,
        std::move(code),
        std::move(msg),
        sp));
}

void DiagnosticEngine::fatal_code(
    std::string code,
    std::string msg,
    SourceSpan sp)
{
    emit(Diagnostic(
        Severity::Fatal,
        std::move(code),
        std::move(msg),
        sp));
}

} // namespace vitte::frontend::diag
