#include "../apps/vitte_ide_gtk/plugin_api.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace {

thread_local std::string g_out;

struct Location {
    std::string file;
    int line = 1;
    int col = 1;
    std::string text;
};

enum class RefKind {
    Read,
    Write,
    Call,
    Impl,
};

struct RefOccurrence {
    std::string symbol;
    RefKind kind = RefKind::Read;
    std::string file;
    int line = 1;
    int col = 1;
    std::string text;
};

struct SymbolInfo {
    std::string kind;
    std::string name;
    std::string file;
    int line = 1;
    int col = 1;
    std::string signature;
};

struct WorkspaceIndex {
    int graph_version = 2;
    std::vector<SymbolInfo> definitions;
    std::unordered_map<std::string, std::vector<Location>> references;
    std::unordered_map<std::string, std::vector<Location>> alias_defs;
    std::unordered_set<std::string> aliases;
    std::unordered_map<std::string, std::string> alias_to_path;
    std::unordered_map<std::string, std::vector<std::string>> alias_to_paths;
    std::vector<RefOccurrence> ref_occurrences;
    std::vector<std::string> diagnostics;
    std::unordered_map<std::string, std::string> file_hashes;
};

struct FileIndexChunk {
    std::string file;
    std::string hash;
    std::vector<SymbolInfo> definitions;
    std::vector<RefOccurrence> ref_occurrences;
    std::vector<Location> alias_locs;
    std::unordered_map<std::string, std::string> alias_to_path;
    std::vector<std::string> diagnostics;
};

struct IdentifierTok {
    std::string text;
    std::size_t begin = 0;
    std::size_t end = 0;
    int col = 1;
};

struct UseLineInfo {
    bool is_use = false;
    bool changed = false;
    std::string path;
    std::string alias;
    std::string rewritten;
};

struct TaskSpec {
    std::string name;
    std::string cmd;
    std::vector<std::string> deps;
};

std::string trim_copy(const std::string& s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])) != 0) {
        ++b;
    }
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) {
        --e;
    }
    return s.substr(b, e - b);
}

std::string lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::vector<std::string> split_csv_trimmed(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        tok = trim_copy(tok);
        if (!tok.empty()) {
            out.push_back(tok);
        }
    }
    return out;
}

std::string ref_kind_to_string(RefKind k) {
    switch (k) {
        case RefKind::Write:
            return "write";
        case RefKind::Call:
            return "call";
        case RefKind::Impl:
            return "impl";
        case RefKind::Read:
        default:
            return "read";
    }
}

RefKind ref_kind_from_string(const std::string& s) {
    if (s == "write") {
        return RefKind::Write;
    }
    if (s == "call") {
        return RefKind::Call;
    }
    if (s == "impl") {
        return RefKind::Impl;
    }
    return RefKind::Read;
}

std::string sanitize_field(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c == '\t' || c == '\n' || c == '\r') {
            c = ' ';
        }
    }
    return out;
}

std::uint64_t fnv1a64(const std::string& s) {
    std::uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) {
        h ^= static_cast<std::uint64_t>(c);
        h *= 1099511628211ull;
    }
    return h;
}

std::string file_hash_fnv(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in.is_open()) {
        return "0";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::ostringstream hex;
    hex << std::hex << fnv1a64(ss.str());
    return hex.str();
}

bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool is_keyword(const std::string& token) {
    static const std::unordered_set<std::string> kKeywords = {
        "use", "space", "pick", "form", "proc", "entry", "trait", "give", "let",
        "if", "else", "loop", "return", "as", "at", "true", "false"};
    return kKeywords.find(token) != kKeywords.end();
}

std::vector<std::string> read_lines(const fs::path& p) {
    std::ifstream in(p);
    if (!in.is_open()) {
        return {};
    }
    std::vector<std::string> out;
    std::string line;
    while (std::getline(in, line)) {
        out.push_back(line);
    }
    return out;
}

bool write_lines(const fs::path& p, const std::vector<std::string>& lines) {
    std::ofstream out(p);
    if (!out.is_open()) {
        return false;
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out << lines[i];
        if (i + 1 < lines.size()) {
            out << '\n';
        }
    }
    out << '\n';
    return true;
}

bool is_source(const fs::path& p) {
    const std::string ext = p.extension().string();
    return ext == ".vit" || ext == ".cpp" || ext == ".hpp" || ext == ".h";
}

std::unordered_map<std::string, std::string> parse_payload(const std::string& payload) {
    std::unordered_map<std::string, std::string> out;
    std::stringstream ss(payload);
    std::string tok;
    while (std::getline(ss, tok, ';')) {
        const std::size_t eq = tok.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        out[trim_copy(tok.substr(0, eq))] = trim_copy(tok.substr(eq + 1));
    }
    return out;
}

std::vector<IdentifierTok> scan_identifiers_precise(const std::string& line) {
    std::vector<IdentifierTok> out;
    bool in_string = false;
    char quote = '\0';
    bool escape = false;

    for (std::size_t i = 0; i < line.size();) {
        const char c = line[i];
        if (!in_string && c == '#') {
            break;
        }
        if (in_string) {
            if (escape) {
                escape = false;
                ++i;
                continue;
            }
            if (c == '\\') {
                escape = true;
                ++i;
                continue;
            }
            if (c == quote) {
                in_string = false;
            }
            ++i;
            continue;
        }
        if (c == '"' || c == '\'') {
            in_string = true;
            quote = c;
            ++i;
            continue;
        }
        if (is_ident_start(c)) {
            const std::size_t b = i;
            ++i;
            while (i < line.size() && is_ident_char(line[i])) {
                ++i;
            }
            IdentifierTok t;
            t.text = line.substr(b, i - b);
            t.begin = b;
            t.end = i;
            t.col = static_cast<int>(b) + 1;
            out.push_back(t);
            continue;
        }
        ++i;
    }
    return out;
}

std::string symbol_at(const std::vector<std::string>& lines, int one_line, int one_col) {
    if (one_line <= 0 || static_cast<std::size_t>(one_line) > lines.size()) {
        return {};
    }
    const std::string& ln = lines[static_cast<std::size_t>(one_line - 1)];
    if (ln.empty()) {
        return {};
    }
    int i = std::max(0, std::min(static_cast<int>(ln.size()) - 1, one_col - 1));
    while (i > 0 && !is_ident_char(ln[static_cast<std::size_t>(i)])) {
        --i;
    }
    if (!is_ident_char(ln[static_cast<std::size_t>(i)])) {
        return {};
    }
    int b = i;
    int e = i;
    while (b > 0 && is_ident_char(ln[static_cast<std::size_t>(b - 1)])) {
        --b;
    }
    while (e + 1 < static_cast<int>(ln.size()) && is_ident_char(ln[static_cast<std::size_t>(e + 1)])) {
        ++e;
    }
    return ln.substr(static_cast<std::size_t>(b), static_cast<std::size_t>(e - b + 1));
}

std::string payload_symbol_or_cursor(const std::unordered_map<std::string, std::string>& p,
                                     const std::vector<std::string>& lines) {
    const int line = p.count("line") ? std::atoi(p.at("line").c_str()) : 1;
    const int col = p.count("col") ? std::atoi(p.at("col").c_str()) : 1;
    if (p.count("symbol") && !p.at("symbol").empty()) {
        return p.at("symbol");
    }
    return symbol_at(lines, line, col);
}

fs::path guess_workspace_root(fs::path from_file) {
    if (!from_file.has_parent_path()) {
        return fs::current_path();
    }
    fs::path cur = from_file.parent_path();
    for (int i = 0; i < 12; ++i) {
        if (fs::exists(cur / ".git") || fs::exists(cur / "toolchain") || fs::exists(cur / "Makefile")) {
            return cur;
        }
        if (!cur.has_parent_path()) {
            break;
        }
        fs::path parent = cur.parent_path();
        if (parent == cur) {
            break;
        }
        cur = parent;
    }
    return from_file.parent_path();
}

[[maybe_unused]] void try_capture_definition(const std::string& rel_file, int line_no, const std::string& line, WorkspaceIndex* idx) {
    auto tokens = scan_identifiers_precise(line);
    if (tokens.size() < 2 || idx == nullptr) {
        return;
    }
    const std::string& head = tokens[0].text;
    if (head != "proc" && head != "form" && head != "entry" && head != "trait") {
        return;
    }
    SymbolInfo s;
    s.kind = head;
    s.name = tokens[1].text;
    s.file = rel_file;
    s.line = line_no;
    s.col = tokens[1].col;
    if (head == "proc") {
        s.signature = trim_copy(line.substr(tokens[0].begin));
    }
    idx->definitions.push_back(s);
}

std::string semantic_index_cache_path(const fs::path& root) {
    fs::path p = root / ".vitte-cache" / "vitte-ide-gtk" / "semantic_v2.index";
    fs::create_directories(p.parent_path());
    return p.string();
}

std::vector<std::string> split_tab(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, '\t')) {
        out.push_back(tok);
    }
    return out;
}

FileIndexChunk parse_file_chunk(const fs::path& root, const fs::path& abs_file) {
    FileIndexChunk c;
    c.file = fs::relative(abs_file, root).string();
    c.hash = file_hash_fnv(abs_file);
    const auto lines = read_lines(abs_file);

    std::unordered_map<int, std::unordered_set<std::string>> defs_by_scope;
    int brace_depth = 0;
    std::vector<std::string> module_stack;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const int line_no = static_cast<int>(i) + 1;
        const std::string& ln = lines[i];
        auto toks = scan_identifiers_precise(ln);
        if (!toks.empty() && toks[0].text == "space" && toks.size() >= 2) {
            module_stack.push_back(toks[1].text);
        }
        if (toks.size() >= 2 &&
            (toks[0].text == "proc" || toks[0].text == "form" || toks[0].text == "entry" || toks[0].text == "trait")) {
            SymbolInfo s;
            s.kind = toks[0].text;
            s.name = toks[1].text;
            s.file = c.file;
            s.line = line_no;
            s.col = toks[1].col;
            if (toks[0].text == "proc") {
                s.signature = trim_copy(ln.substr(toks[0].begin));
            }
            c.definitions.push_back(s);
            defs_by_scope[brace_depth].insert(s.name);
        }

        if (toks.size() >= 3 && toks[0].text == "let") {
            const std::string& name = toks[1].text;
            bool shadow = false;
            for (int d = brace_depth; d >= 0; --d) {
                if (defs_by_scope[d].find(name) != defs_by_scope[d].end()) {
                    shadow = true;
                    break;
                }
            }
            defs_by_scope[brace_depth].insert(name);
            if (shadow) {
                std::ostringstream d;
                d << c.file << ":" << line_no << ":" << toks[1].col << ":warning:VITTE-A1001:shadowing symbol '" << name << "'";
                c.diagnostics.push_back(d.str());
            }
        }

        if (toks.size() >= 2 && toks[0].text == "use") {
            std::string alias;
            std::string path = toks[1].text;
            for (std::size_t t = 2; t + 1 < toks.size(); ++t) {
                if (toks[t].text == "as") {
                    alias = toks[t + 1].text;
                    break;
                }
            }
            if (alias.empty()) {
                std::ostringstream d;
                d << c.file << ":" << line_no << ":" << toks[1].col << ":error:VITTE-A2001:import alias missing, expected 'use ... as ...'";
                c.diagnostics.push_back(d.str());
            } else {
                Location loc;
                loc.file = c.file;
                loc.line = line_no;
                loc.col = toks[1].col;
                loc.text = ln;
                c.alias_locs.push_back(loc);
                c.alias_to_path[alias] = path;
            }
        }

        for (std::size_t t = 0; t < toks.size(); ++t) {
            if (is_keyword(toks[t].text)) {
                continue;
            }
            RefOccurrence r;
            r.symbol = toks[t].text;
            r.file = c.file;
            r.line = line_no;
            r.col = toks[t].col;
            r.text = ln;
            const std::string prev = t > 0 ? toks[t - 1].text : "";
            const std::size_t next_non_space = toks[t].end < ln.size() ? ln.find_first_not_of(" \t", toks[t].end) : std::string::npos;
            const bool next_call = next_non_space != std::string::npos && ln[next_non_space] == '(';
            if (prev == "let") {
                r.kind = RefKind::Write;
            } else if (prev == "impl") {
                r.kind = RefKind::Impl;
            } else if (next_call) {
                r.kind = RefKind::Call;
            } else {
                r.kind = RefKind::Read;
            }
            c.ref_occurrences.push_back(r);
        }

        for (char ch : ln) {
            if (ch == '{') {
                ++brace_depth;
            } else if (ch == '}') {
                if (brace_depth > 0) {
                    defs_by_scope.erase(brace_depth);
                    --brace_depth;
                }
            }
        }
    }
    return c;
}

void write_index_cache(const fs::path& root, const std::unordered_map<std::string, FileIndexChunk>& chunks) {
    std::ofstream out(semantic_index_cache_path(root));
    if (!out.is_open()) {
        return;
    }
    out << "VERSION\t2\n";
    for (const auto& [file, c] : chunks) {
        out << "FILE\t" << sanitize_field(file) << "\t" << c.hash << "\n";
        for (const SymbolInfo& d : c.definitions) {
            out << "DEF\t" << sanitize_field(d.kind) << "\t" << sanitize_field(d.name) << "\t" << d.line << "\t" << d.col << "\t"
                << sanitize_field(d.signature) << "\n";
        }
        for (const auto& [alias, path] : c.alias_to_path) {
            out << "ALIAS\t" << sanitize_field(alias) << "\t" << sanitize_field(path) << "\n";
        }
        for (const RefOccurrence& r : c.ref_occurrences) {
            out << "REF\t" << sanitize_field(r.symbol) << "\t" << ref_kind_to_string(r.kind) << "\t" << r.line << "\t" << r.col
                << "\t" << sanitize_field(r.text) << "\n";
        }
        for (const std::string& d : c.diagnostics) {
            out << "DIAG\t" << sanitize_field(d) << "\n";
        }
        out << "END\n";
    }
}

std::unordered_map<std::string, FileIndexChunk> read_index_cache(const fs::path& root) {
    std::unordered_map<std::string, FileIndexChunk> out;
    std::ifstream in(semantic_index_cache_path(root));
    if (!in.is_open()) {
        return out;
    }
    std::string line;
    FileIndexChunk cur;
    bool have_cur = false;
    while (std::getline(in, line)) {
        auto parts = split_tab(line);
        if (parts.empty()) {
            continue;
        }
        if (parts[0] == "FILE" && parts.size() >= 3) {
            if (have_cur && !cur.file.empty()) {
                out[cur.file] = cur;
            }
            cur = FileIndexChunk();
            cur.file = parts[1];
            cur.hash = parts[2];
            have_cur = true;
        } else if (parts[0] == "DEF" && parts.size() >= 6 && have_cur) {
            SymbolInfo d;
            d.kind = parts[1];
            d.name = parts[2];
            d.file = cur.file;
            d.line = std::max(1, std::atoi(parts[3].c_str()));
            d.col = std::max(1, std::atoi(parts[4].c_str()));
            d.signature = parts[5];
            cur.definitions.push_back(d);
        } else if (parts[0] == "ALIAS" && parts.size() >= 3 && have_cur) {
            cur.alias_to_path[parts[1]] = parts[2];
        } else if (parts[0] == "REF" && parts.size() >= 6 && have_cur) {
            RefOccurrence r;
            r.symbol = parts[1];
            r.kind = ref_kind_from_string(parts[2]);
            r.file = cur.file;
            r.line = std::max(1, std::atoi(parts[3].c_str()));
            r.col = std::max(1, std::atoi(parts[4].c_str()));
            r.text = parts[5];
            cur.ref_occurrences.push_back(r);
        } else if (parts[0] == "DIAG" && parts.size() >= 2 && have_cur) {
            cur.diagnostics.push_back(parts[1]);
        } else if (parts[0] == "END" && have_cur && !cur.file.empty()) {
            out[cur.file] = cur;
            cur = FileIndexChunk();
            have_cur = false;
        }
    }
    if (have_cur && !cur.file.empty()) {
        out[cur.file] = cur;
    }
    return out;
}

WorkspaceIndex build_workspace_index(const fs::path& root) {
    WorkspaceIndex idx;
    auto cache = read_index_cache(root);
    std::unordered_map<std::string, FileIndexChunk> out_chunks;

    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || !is_source(e.path())) {
            continue;
        }
        const std::string rel = fs::relative(e.path(), root).string();
        const std::string hash = file_hash_fnv(e.path());
        auto it = cache.find(rel);
        if (it != cache.end() && it->second.hash == hash) {
            out_chunks[rel] = it->second;
        } else {
            out_chunks[rel] = parse_file_chunk(root, e.path());
        }
    }

    write_index_cache(root, out_chunks);

    for (const auto& [file, c] : out_chunks) {
        idx.file_hashes[file] = c.hash;
        for (const SymbolInfo& d : c.definitions) {
            idx.definitions.push_back(d);
        }
        for (const RefOccurrence& r : c.ref_occurrences) {
            idx.ref_occurrences.push_back(r);
            Location loc;
            loc.file = r.file;
            loc.line = r.line;
            loc.col = r.col;
            loc.text = r.text;
            idx.references[r.symbol].push_back(loc);
        }
        for (const auto& [alias, path] : c.alias_to_path) {
            idx.aliases.insert(alias);
            idx.alias_to_paths[alias].push_back(path);
            idx.alias_to_path[alias] = path;
        }
        for (const auto& d : c.diagnostics) {
            idx.diagnostics.push_back(d);
        }
    }

    for (const auto& [alias, paths] : idx.alias_to_paths) {
        std::unordered_set<std::string> uniq(paths.begin(), paths.end());
        if (uniq.size() > 1) {
            std::ostringstream d;
            d << "workspace:1:1:error:VITTE-A2002:ambiguous alias '" << alias << "' => ";
            bool first = true;
            for (const std::string& p : uniq) {
                if (!first) {
                    d << ",";
                }
                first = false;
                d << p;
            }
            idx.diagnostics.push_back(d.str());
        }
    }

    return idx;
}

const SymbolInfo* find_definition_prefer_file(const WorkspaceIndex& idx,
                                              const std::string& symbol,
                                              const std::string& rel_file) {
    const SymbolInfo* best = nullptr;
    int best_score = -1;
    const std::string rel_dir = fs::path(rel_file).parent_path().string();
    for (const SymbolInfo& s : idx.definitions) {
        if (s.name != symbol) {
            continue;
        }
        int score = 10;
        if (!rel_file.empty() && s.file == rel_file) {
            score += 300;
        }
        if (!rel_dir.empty() && fs::path(s.file).parent_path().string() == rel_dir) {
            score += 120;
        }
        if (s.kind == "proc" || s.kind == "form") {
            score += 20;
        }
        if (score > best_score) {
            best_score = score;
            best = &s;
        }
    }
    return best;
}

[[maybe_unused]] std::vector<const SymbolInfo*> find_definition_candidates(const WorkspaceIndex& idx,
                                                                           const std::string& symbol,
                                                                           const std::string& rel_file) {
    std::vector<const SymbolInfo*> out;
    for (const SymbolInfo& s : idx.definitions) {
        if (s.name == symbol) {
            out.push_back(&s);
        }
    }
    std::sort(out.begin(), out.end(), [&](const SymbolInfo* a, const SymbolInfo* b) {
        auto score = [&](const SymbolInfo* d) {
            int sc = 0;
            if (d->file == rel_file) {
                sc += 1000;
            }
            if (fs::path(d->file).parent_path() == fs::path(rel_file).parent_path()) {
                sc += 200;
            }
            if (d->kind == "proc") {
                sc += 30;
            }
            return sc;
        };
        return score(a) > score(b);
    });
    return out;
}

std::string ref_kind_summary(const WorkspaceIndex& idx, const std::string& sym) {
    int reads = 0;
    int writes = 0;
    int calls = 0;
    int impls = 0;
    for (const RefOccurrence& r : idx.ref_occurrences) {
        if (r.symbol != sym) {
            continue;
        }
        if (r.kind == RefKind::Write) {
            ++writes;
        } else if (r.kind == RefKind::Call) {
            ++calls;
        } else if (r.kind == RefKind::Impl) {
            ++impls;
        } else {
            ++reads;
        }
    }
    std::ostringstream o;
    o << "reads=" << reads << ",writes=" << writes << ",calls=" << calls << ",impl=" << impls;
    return o.str();
}

std::string cmd_hover(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const auto lines = read_lines(file);
    const std::string sym = payload_symbol_or_cursor(p, lines);
    if (sym.empty()) {
        return "hover: no symbol";
    }
    const fs::path root = guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    const std::string rel_file = fs::relative(file, root).string();
    const SymbolInfo* def = find_definition_prefer_file(idx, sym, rel_file);
    if (def == nullptr) {
        return "hover: symbol=" + sym;
    }
    std::ostringstream out;
    out << "symbol: " << def->name << "\n";
    out << "kind: " << def->kind << "\n";
    if (!def->signature.empty()) {
        out << "signature: " << def->signature << "\n";
    }
    auto ait = idx.alias_to_path.find(sym);
    if (ait != idx.alias_to_path.end()) {
        out << "import: use " << ait->second << " as " << sym << "\n";
    }
    out << "usages: " << ref_kind_summary(idx, sym) << "\n";
    out << "location: " << def->file << ":" << def->line << ":" << def->col;
    if (p.count("explain") && p.at("explain") == "1") {
        out << "\nexplain: selected by ranking (same file > same module > kind priority)";
    }
    return out.str();
}

std::string cmd_signature(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const auto lines = read_lines(file);
    const std::string sym = payload_symbol_or_cursor(p, lines);
    if (sym.empty()) {
        return "signature: no symbol";
    }
    const fs::path root = guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    const std::string rel_file = fs::relative(file, root).string();
    const SymbolInfo* def = find_definition_prefer_file(idx, sym, rel_file);
    if (def == nullptr || def->kind != "proc") {
        return "signature: not found for " + sym;
    }
    int active_arg = 0;
    const int line = p.count("line") ? std::atoi(p.at("line").c_str()) : 1;
    const int col = p.count("col") ? std::atoi(p.at("col").c_str()) : 1;
    if (line > 0 && static_cast<std::size_t>(line) <= lines.size()) {
        const std::string& ln = lines[static_cast<std::size_t>(line - 1)];
        const int lim = std::max(0, std::min(static_cast<int>(ln.size()), col - 1));
        for (int i = 0; i < lim; ++i) {
            if (ln[static_cast<std::size_t>(i)] == ',') {
                ++active_arg;
            }
        }
    }
    std::ostringstream out;
    out << (def->signature.empty() ? ("proc " + sym) : def->signature) << "\n";
    out << "active_arg=" << active_arg;
    return out.str();
}

std::string cmd_refs(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const auto lines = read_lines(file);
    const std::string sym = payload_symbol_or_cursor(p, lines);
    if (sym.empty()) {
        return "refs: no symbol";
    }
    const fs::path root = guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    int hits = 0;
    std::ostringstream out;
    std::string kind_filter = p.count("kind") ? trim_copy(p.at("kind")) : "";
    for (const RefOccurrence& r : idx.ref_occurrences) {
        if (r.symbol != sym) {
            continue;
        }
        const std::string k = ref_kind_to_string(r.kind);
        if (!kind_filter.empty() && kind_filter != k) {
            continue;
        }
        out << r.file << ":" << r.line << ":" << r.col << ":info:[" << k << "] " << r.text << "\n";
        ++hits;
    }
    if (hits == 0) {
        return "refs: none";
    }
    if (p.count("explain") && p.at("explain") == "1") {
        out << "explain: filtered by AST-like token context, kind=" << (kind_filter.empty() ? "all" : kind_filter) << "\n";
    }
    return out.str();
}

std::string cmd_goto(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const auto lines = read_lines(file);
    const std::string sym = payload_symbol_or_cursor(p, lines);
    if (sym.empty()) {
        return "goto: no-symbol";
    }
    const fs::path root = guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    const std::string rel_file = fs::relative(file, root).string();
    const SymbolInfo* best = find_definition_prefer_file(idx, sym, rel_file);
    const auto candidates = find_definition_candidates(idx, sym, rel_file);
    if (best != nullptr) {
        double confidence = 0.55;
        if (best->file == rel_file) {
            confidence += 0.25;
        }
        if (best->kind == "proc" || best->kind == "form" || best->kind == "pick" || best->kind == "trait") {
            confidence += 0.10;
        }
        if (candidates.size() <= 1) {
            confidence += 0.10;
        }
        if (confidence > 0.99) {
            confidence = 0.99;
        }
        std::ostringstream out;
        out << "def:" << best->file << ":" << best->line << ":" << best->col
            << ":confidence=" << confidence
            << ":candidates=" << candidates.size()
            << ":symbol=" << sym;
        return out.str();
    }

    for (const RefOccurrence& r : idx.ref_occurrences) {
        if (r.symbol != sym) {
            continue;
        }
        std::ostringstream out;
        out << "fallback:" << r.file << ":" << r.line << ":" << r.col
            << ":confidence=0.20:candidates=0:symbol=" << sym
            << ":reason=refs-fallback";
        return out.str();
    }
    return "goto: not-found";
}

std::string cmd_completion(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);

    std::set<std::string> out = {
        "use", "space", "pick", "form", "proc", "entry", "trait", "give",
        "let", "if", "else", "loop", "return", "as", "at"};

    for (const SymbolInfo& s : idx.definitions) {
        out.insert(s.name);
    }
    for (const auto& alias : idx.aliases) {
        out.insert(alias);
    }

    std::ostringstream csv;
    bool first = true;
    for (const std::string& it : out) {
        if (!first) {
            csv << ",";
        }
        first = false;
        csv << it;
    }
    return csv.str();
}

std::string derive_alias_from_path(const std::string& path) {
    std::string base = path;
    const std::size_t slash = path.find_last_of('/');
    if (slash != std::string::npos && slash + 1 < path.size()) {
        base = path.substr(slash + 1);
    }
    std::string out;
    for (char c : base) {
        if (is_ident_char(c)) {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty() || std::isdigit(static_cast<unsigned char>(out[0])) != 0) {
        out = "pkg_" + out;
    }
    if (out.size() < 4 || out.substr(out.size() - 4) != "_pkg") {
        out += "_pkg";
    }
    return out;
}

UseLineInfo parse_use_line(const std::string& line) {
    UseLineInfo out;
    std::size_t non_ws = 0;
    while (non_ws < line.size() && std::isspace(static_cast<unsigned char>(line[non_ws])) != 0) {
        ++non_ws;
    }
    const std::string indent = line.substr(0, non_ws);
    std::string body = line.substr(non_ws);
    const std::size_t hash = body.find('#');
    const std::string comment = hash == std::string::npos ? "" : body.substr(hash);
    if (hash != std::string::npos) {
        body = body.substr(0, hash);
    }
    body = trim_copy(body);
    if (body.rfind("use ", 0) != 0) {
        return out;
    }
    out.is_use = true;

    std::stringstream ss(body);
    std::vector<std::string> parts;
    std::string tok;
    while (ss >> tok) {
        parts.push_back(tok);
    }
    if (parts.size() < 2) {
        return out;
    }
    out.path = parts[1];
    if (parts.size() >= 4 && parts[2] == "as") {
        out.alias = parts[3];
    } else if (parts.size() >= 3) {
        out.alias = parts[2];
        out.changed = true;
    } else {
        out.alias = derive_alias_from_path(out.path);
        out.changed = true;
    }
    out.rewritten = indent + "use " + out.path + " as " + out.alias;
    if (!comment.empty()) {
        out.rewritten += " " + comment;
    }
    return out;
}

struct ImportFixPlan {
    std::vector<std::string> rewritten_lines;
    std::vector<std::string> inserts;
    std::vector<std::string> notes;
};

ImportFixPlan build_import_fix_plan(const std::vector<std::string>& lines) {
    ImportFixPlan plan;
    std::unordered_map<std::string, std::string> alias_to_path;

    for (const std::string& line : lines) {
        UseLineInfo u = parse_use_line(line);
        if (!u.is_use) {
            plan.rewritten_lines.push_back(line);
            continue;
        }
        plan.rewritten_lines.push_back(u.changed ? u.rewritten : line);
        alias_to_path[u.alias] = u.path;
        if (u.changed) {
            plan.notes.push_back("normalize use: " + trim_copy(line) + " -> " + trim_copy(u.rewritten));
        }
    }

    std::unordered_set<std::string> used_aliases;
    for (const std::string& line : lines) {
        for (const IdentifierTok& tok : scan_identifiers_precise(line)) {
            if (tok.text.size() > 4 && tok.text.substr(tok.text.size() - 4) == "_pkg") {
                used_aliases.insert(tok.text);
            }
        }
    }

    static const std::unordered_map<std::string, std::string> kKnownAliasPath = {
        {"core_pkg", "vitte/core"},
        {"log_pkg", "vitte/log"},
        {"array_pkg", "vitte/array"},
        {"ast_pkg", "vitte/ast"},
        {"alloc_pkg", "vitte/alloc"},
        {"alerts_pkg", "vitte/alerts"},
    };

    for (const auto& [alias, path] : kKnownAliasPath) {
        if (used_aliases.find(alias) == used_aliases.end()) {
            continue;
        }
        auto it = alias_to_path.find(alias);
        if (it == alias_to_path.end()) {
            plan.inserts.push_back("use " + path + " as " + alias);
            plan.notes.push_back("add missing import: use " + path + " as " + alias);
            alias_to_path[alias] = path;
            continue;
        }
        if (it->second != path) {
            plan.inserts.push_back("use " + path + " as " + alias);
            plan.notes.push_back("fix alias mapping for " + alias + ": " + it->second + " -> " + path);
            it->second = path;
        }
    }

    return plan;
}

std::string cmd_import_fix(const std::unordered_map<std::string, std::string>& p, bool apply) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "import-fix: file empty/unreadable";
    }
    ImportFixPlan plan = build_import_fix_plan(lines);
    if (plan.notes.empty()) {
        return "import-fix: nothing to fix";
    }
    std::ostringstream preview;
    preview << "import-fix (" << plan.notes.size() << " change(s))\n";
    for (const std::string& n : plan.notes) {
        preview << "- " << n << "\n";
    }
    if (!apply) {
        return preview.str();
    }
    std::vector<std::string> out_lines;
    out_lines.insert(out_lines.end(), plan.inserts.begin(), plan.inserts.end());
    out_lines.insert(out_lines.end(), plan.rewritten_lines.begin(), plan.rewritten_lines.end());
    if (!write_lines(file, out_lines)) {
        return "import-fix: write failed";
    }
    return "import-fix: applied " + std::to_string(plan.notes.size()) + " change(s)";
}

std::size_t find_prev_non_space(const std::string& line, std::size_t pos) {
    while (pos > 0) {
        --pos;
        if (std::isspace(static_cast<unsigned char>(line[pos])) == 0) {
            return pos;
        }
    }
    return std::string::npos;
}

std::size_t find_next_non_space(const std::string& line, std::size_t pos) {
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
        ++pos;
    }
    return pos < line.size() ? pos : std::string::npos;
}

bool should_rename_occurrence(const std::string& line,
                              const std::vector<IdentifierTok>& ids,
                              std::size_t tok_idx,
                              bool target_is_definition) {
    const IdentifierTok& tok = ids[tok_idx];
    std::string prev_ident;
    if (tok_idx > 0) {
        prev_ident = ids[tok_idx - 1].text;
    }
    if (prev_ident == "let") {
        return false;
    }
    if (prev_ident == "proc" || prev_ident == "form" || prev_ident == "entry" || prev_ident == "trait" || prev_ident == "as") {
        return true;
    }
    if (!target_is_definition) {
        return true;
    }
    const std::size_t prev = find_prev_non_space(line, tok.begin);
    const std::size_t next = find_next_non_space(line, tok.end);
    if (next != std::string::npos && line[next] == '(') {
        return true;
    }
    if (prev != std::string::npos && (line[prev] == '.' || line[prev] == ':')) {
        return true;
    }
    return true;
}

std::string apply_replacements(const std::string& line,
                               const std::vector<std::pair<std::size_t, std::size_t>>& ranges,
                               const std::string& replacement) {
    if (ranges.empty()) {
        return line;
    }
    std::string out;
    std::size_t cursor = 0;
    for (const auto& [b, e] : ranges) {
        if (b > cursor) {
            out.append(line.substr(cursor, b - cursor));
        }
        out.append(replacement);
        cursor = e;
    }
    if (cursor < line.size()) {
        out.append(line.substr(cursor));
    }
    return out;
}

std::string cmd_refactor_rename(const std::unordered_map<std::string, std::string>& p, bool apply) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);

    std::string old_name = p.count("old") ? p.at("old") : "";
    const std::string new_name = p.count("new") ? p.at("new") : "";
    if (old_name.empty() && !file.empty()) {
        const auto lines = read_lines(file);
        const int line = p.count("line") ? std::atoi(p.at("line").c_str()) : 1;
        const int col = p.count("col") ? std::atoi(p.at("col").c_str()) : 1;
        old_name = symbol_at(lines, line, col);
    }
    if (old_name.empty() || new_name.empty()) {
        return "refactor.rename: payload requires old/new (or file+line+col)";
    }

    WorkspaceIndex idx = build_workspace_index(root);
    bool target_is_definition = false;
    for (const SymbolInfo& d : idx.definitions) {
        if (d.name == old_name) {
            target_is_definition = true;
            break;
        }
    }
    if (!target_is_definition && idx.aliases.find(old_name) != idx.aliases.end()) {
        target_is_definition = true;
    }

    int files_changed = 0;
    int total_hits = 0;
    std::ostringstream preview;

    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || !is_source(e.path())) {
            continue;
        }
        auto lines = read_lines(e.path());
        bool touched = false;
        const std::string rel = fs::relative(e.path(), root).string();
        for (std::size_t i = 0; i < lines.size(); ++i) {
            auto ids = scan_identifiers_precise(lines[i]);
            std::vector<std::pair<std::size_t, std::size_t>> ranges;
            for (std::size_t t = 0; t < ids.size(); ++t) {
                if (ids[t].text != old_name) {
                    continue;
                }
                if (!should_rename_occurrence(lines[i], ids, t, target_is_definition)) {
                    continue;
                }
                ranges.push_back({ids[t].begin, ids[t].end});
            }
            if (ranges.empty()) {
                continue;
            }
            touched = true;
            total_hits += static_cast<int>(ranges.size());
            const std::string replaced = apply_replacements(lines[i], ranges, new_name);
            preview << rel << ":" << (i + 1) << "\n- " << lines[i] << "\n+ " << replaced << "\n";
            lines[i] = replaced;
        }
        if (touched) {
            ++files_changed;
            if (apply) {
                (void)write_lines(e.path(), lines);
            }
        }
    }

    std::ostringstream msg;
    msg << "refactor.rename " << old_name << " -> " << new_name
        << " files=" << files_changed << " hits=" << total_hits << "\n";
    if (!apply) {
        msg << preview.str();
    }
    return msg.str();
}

std::string cmd_quickfix(const std::unordered_map<std::string, std::string>& p, bool apply) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "quickfix: empty/unreadable";
    }

    std::vector<std::string> notes;
    ImportFixPlan plan = build_import_fix_plan(lines);
    if (!plan.notes.empty()) {
        notes.insert(notes.end(), plan.notes.begin(), plan.notes.end());
        if (apply) {
            std::vector<std::string> merged;
            merged.insert(merged.end(), plan.inserts.begin(), plan.inserts.end());
            merged.insert(merged.end(), plan.rewritten_lines.begin(), plan.rewritten_lines.end());
            lines.swap(merged);
        }
    }

    static const std::unordered_map<std::string, std::string> kTypos = {
        {"retrun", "return"},
        {"entyr", "entry"},
        {"prco", "proc"},
        {"trati", "trait"},
    };

    for (std::string& line : lines) {
        auto ids = scan_identifiers_precise(line);
        std::vector<std::pair<std::size_t, std::size_t>> ranges;
        std::vector<std::string> repls;
        for (const auto& id : ids) {
            auto it = kTypos.find(id.text);
            if (it == kTypos.end()) {
                continue;
            }
            ranges.push_back({id.begin, id.end});
            repls.push_back(it->second);
            notes.push_back("fix typo: " + id.text + " -> " + it->second);
        }
        if (ranges.empty() || !apply) {
            continue;
        }
        std::string out;
        std::size_t cursor = 0;
        for (std::size_t i = 0; i < ranges.size(); ++i) {
            const auto [b, e] = ranges[i];
            if (b > cursor) {
                out.append(line.substr(cursor, b - cursor));
            }
            out.append(repls[i]);
            cursor = e;
        }
        if (cursor < line.size()) {
            out.append(line.substr(cursor));
        }
        line = out;
    }

    bool has_entry_main = false;
    for (const std::string& line : lines) {
        auto ids = scan_identifiers_precise(line);
        if (ids.size() >= 2 && ids[0].text == "entry" && ids[1].text == "main") {
            has_entry_main = true;
            break;
        }
    }
    if (!has_entry_main) {
        notes.push_back("add missing entry main");
        if (apply) {
            lines.push_back("");
            lines.push_back("entry main at core/app {");
            lines.push_back("  give 0");
            lines.push_back("}");
        }
    }

    if (notes.empty()) {
        return "quickfix: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "quickfix: write failed";
        }
        return "quickfix: applied " + std::to_string(notes.size()) + " fix(es)";
    }

    std::ostringstream out;
    out << "quickfix preview:\n";
    for (const std::string& n : notes) {
        out << "- " << n << "\n";
    }
    return out.str();
}

std::string find_json_object_block(const std::string& txt, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t k = txt.find(needle);
    if (k == std::string::npos) {
        return {};
    }
    std::size_t i = txt.find(':', k + needle.size());
    if (i == std::string::npos) {
        return {};
    }
    i = txt.find('{', i + 1);
    if (i == std::string::npos) {
        return {};
    }
    int depth = 0;
    bool in_string = false;
    bool esc = false;
    for (std::size_t j = i; j < txt.size(); ++j) {
        const char c = txt[j];
        if (in_string) {
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }
        if (c == '"') {
            in_string = true;
            continue;
        }
        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                return txt.substr(i, j - i + 1);
            }
        }
    }
    return {};
}

std::vector<std::string> find_quoted_strings(const std::string& txt) {
    std::vector<std::string> out;
    bool in = false;
    bool esc = false;
    std::string cur;
    for (char c : txt) {
        if (!in) {
            if (c == '"') {
                in = true;
                cur.clear();
            }
            continue;
        }
        if (esc) {
            cur.push_back(c);
            esc = false;
            continue;
        }
        if (c == '\\') {
            esc = true;
            continue;
        }
        if (c == '"') {
            out.push_back(cur);
            in = false;
            continue;
        }
        cur.push_back(c);
    }
    return out;
}

std::string shell_quote_single(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out += "'";
    return out;
}

std::string exec_capture(const std::string& cmd, int* rc_out = nullptr) {
    std::array<char, 4096> buf{};
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (p == nullptr) {
        if (rc_out != nullptr) {
            *rc_out = -1;
        }
        return out;
    }
    while (fgets(buf.data(), static_cast<int>(buf.size()), p) != nullptr) {
        out.append(buf.data());
    }
    const int rc = pclose(p);
    if (rc_out != nullptr) {
        *rc_out = rc;
    }
    return out;
}

std::unordered_map<std::string, TaskSpec> parse_tasks_file(const fs::path& file) {
    std::unordered_map<std::string, TaskSpec> out;
    std::ifstream in(file);
    if (!in.is_open()) {
        return out;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string txt = ss.str();
    std::string tasks_block = find_json_object_block(txt, "tasks");
    if (tasks_block.empty()) {
        return out;
    }

    std::size_t i = 1;
    while (i + 1 < tasks_block.size()) {
        while (i < tasks_block.size() && (std::isspace(static_cast<unsigned char>(tasks_block[i])) != 0 || tasks_block[i] == ',')) {
            ++i;
        }
        if (i >= tasks_block.size() || tasks_block[i] == '}') {
            break;
        }
        if (tasks_block[i] != '"') {
            ++i;
            continue;
        }
        const std::size_t key_b = i + 1;
        const std::size_t key_e = tasks_block.find('"', key_b);
        if (key_e == std::string::npos) {
            break;
        }
        const std::string name = tasks_block.substr(key_b, key_e - key_b);
        i = key_e + 1;
        i = tasks_block.find(':', i);
        if (i == std::string::npos) {
            break;
        }
        ++i;
        while (i < tasks_block.size() && std::isspace(static_cast<unsigned char>(tasks_block[i])) != 0) {
            ++i;
        }
        TaskSpec task;
        task.name = name;
        if (i >= tasks_block.size()) {
            break;
        }
        if (tasks_block[i] == '"') {
            const std::size_t v_b = i + 1;
            const std::size_t v_e = tasks_block.find('"', v_b);
            if (v_e == std::string::npos) {
                break;
            }
            task.cmd = tasks_block.substr(v_b, v_e - v_b);
            i = v_e + 1;
        } else if (tasks_block[i] == '{') {
            const std::size_t obj_b = i;
            int depth = 0;
            bool in_string = false;
            bool esc = false;
            std::size_t obj_e = std::string::npos;
            for (; i < tasks_block.size(); ++i) {
                const char c = tasks_block[i];
                if (in_string) {
                    if (esc) {
                        esc = false;
                    } else if (c == '\\') {
                        esc = true;
                    } else if (c == '"') {
                        in_string = false;
                    }
                    continue;
                }
                if (c == '"') {
                    in_string = true;
                    continue;
                }
                if (c == '{') {
                    ++depth;
                } else if (c == '}') {
                    --depth;
                    if (depth == 0) {
                        obj_e = i;
                        ++i;
                        break;
                    }
                }
            }
            if (obj_e == std::string::npos) {
                break;
            }
            const std::string obj = tasks_block.substr(obj_b, obj_e - obj_b + 1);
            const std::string cmd_block = find_json_object_block(obj, "cmd");
            if (!cmd_block.empty()) {
                auto ss2 = find_quoted_strings(cmd_block);
                if (!ss2.empty()) {
                    task.cmd = ss2.back();
                }
            } else {
                auto ss3 = find_quoted_strings(obj);
                for (std::size_t q = 0; q + 1 < ss3.size(); q += 2) {
                    if (ss3[q] == "cmd" || ss3[q] == "command") {
                        task.cmd = ss3[q + 1];
                    }
                }
            }
            std::size_t deps_key = obj.find("\"deps\"");
            if (deps_key == std::string::npos) {
                deps_key = obj.find("\"depends_on\"");
            }
            if (deps_key != std::string::npos) {
                std::size_t lb = obj.find('[', deps_key);
                std::size_t rb = lb == std::string::npos ? std::string::npos : obj.find(']', lb + 1);
                if (lb != std::string::npos && rb != std::string::npos) {
                    auto deps = find_quoted_strings(obj.substr(lb, rb - lb + 1));
                    task.deps.insert(task.deps.end(), deps.begin(), deps.end());
                }
            }
        } else {
            while (i < tasks_block.size() && tasks_block[i] != ',') {
                ++i;
            }
        }
        if (!task.cmd.empty()) {
            out[task.name] = task;
        }
    }
    return out;
}

bool resolve_task_order_dfs(const std::unordered_map<std::string, TaskSpec>& tasks,
                            const std::string& task_name,
                            std::unordered_map<std::string, int>* state,
                            std::vector<std::string>* order,
                            std::string* err) {
    auto it = tasks.find(task_name);
    if (it == tasks.end()) {
        if (err != nullptr) {
            *err = "unknown task: " + task_name;
        }
        return false;
    }
    int s = (*state)[task_name];
    if (s == 2) {
        return true;
    }
    if (s == 1) {
        if (err != nullptr) {
            *err = "dependency cycle at task: " + task_name;
        }
        return false;
    }
    (*state)[task_name] = 1;
    for (const std::string& dep : it->second.deps) {
        if (!resolve_task_order_dfs(tasks, dep, state, order, err)) {
            return false;
        }
    }
    (*state)[task_name] = 2;
    order->push_back(task_name);
    return true;
}

std::vector<std::string> resolve_task_order(const std::unordered_map<std::string, TaskSpec>& tasks,
                                            const std::string& requested,
                                            std::string* err) {
    std::vector<std::string> order;
    std::unordered_map<std::string, int> state;
    if (!resolve_task_order_dfs(tasks, requested, &state, &order, err)) {
        order.clear();
    }
    return order;
}

std::string cmd_docgen_preview(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const auto lines = read_lines(file);
    if (lines.empty()) {
        return "docgen: empty/unreadable";
    }
    const fs::path root = guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    const std::string rel = fs::relative(file, root).string();
    const std::string sym = payload_symbol_or_cursor(p, lines);

    std::ostringstream md;
    md << "# Module: " << rel << "\n\n";
    if (!sym.empty()) {
        const SymbolInfo* def = find_definition_prefer_file(idx, sym, rel);
        if (def != nullptr) {
            md << "## Symbol: " << def->name << "\n";
            md << "- kind: " << def->kind << "\n";
            md << "- location: " << def->file << ":" << def->line << ":" << def->col << "\n";
            if (!def->signature.empty()) {
                md << "- signature: `" << def->signature << "`\n";
            }
            auto refs = idx.references.find(def->name);
            if (refs != idx.references.end()) {
                md << "- references: " << refs->second.size() << "\n";
            }
            md << "\n";
        }
    }

    md << "## Exports\n";
    for (const SymbolInfo& d : idx.definitions) {
        if (d.file != rel) {
            continue;
        }
        md << "- " << d.kind << " " << d.name << " (line " << d.line << ")\n";
    }
    return md.str();
}

std::string detect_test_mode(const fs::path& root, const fs::path& file_hint, const std::string& req_mode) {
    if (req_mode == "test" || req_mode == "fuzz" || req_mode == "bench") {
        return req_mode;
    }
    const std::string hint = file_hint.string();
    if (hint.find("fuzz") != std::string::npos) {
        return "fuzz";
    }
    if (hint.find("bench") != std::string::npos) {
        return "bench";
    }
    if (hint.find("test") != std::string::npos) {
        return "test";
    }
    if (fs::exists(root / "tests" / "fuzz")) {
        return "fuzz";
    }
    if (fs::exists(root / "tests" / "bench")) {
        return "bench";
    }
    return "test";
}

std::string cmd_test_runner(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::string mode = detect_test_mode(root, file, p.count("mode") ? p.at("mode") : "");

    std::string cmd;
    const std::string qroot = shell_quote_single(root.string());
    if (mode == "fuzz") {
        cmd = "cd " + qroot + " && (make fuzz || make test)";
    } else if (mode == "bench") {
        cmd = "cd " + qroot + " && (make bench || make test)";
    } else {
        cmd = "cd " + qroot + " && make test";
    }
    std::ostringstream out;
    out << "mode=" << mode << "\n";
    out << "command=" << cmd << "\n";
    return out.str();
}

std::string cmd_task_orchestrator_list(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const auto tasks = parse_tasks_file(root / "tasks.vitte.json");
    if (tasks.empty()) {
        return "tasks: none";
    }
    std::vector<std::string> names;
    names.reserve(tasks.size());
    for (const auto& [name, _] : tasks) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    std::ostringstream out;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << names[i];
    }
    return out.str();
}

std::string cmd_task_orchestrator_plan(const std::unordered_map<std::string, std::string>& p, bool only_command) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const auto tasks = parse_tasks_file(root / "tasks.vitte.json");
    if (tasks.empty()) {
        return "task-orchestrator: no tasks.vitte.json or no tasks";
    }
    std::string task = p.count("task") ? p.at("task") : "";
    if (task.empty()) {
        if (tasks.find("build") != tasks.end()) {
            task = "build";
        } else if (tasks.find("test") != tasks.end()) {
            task = "test";
        } else {
            task = tasks.begin()->first;
        }
    }

    std::string err;
    const auto order = resolve_task_order(tasks, task, &err);
    if (order.empty()) {
        return "task-orchestrator: " + err;
    }

    std::ostringstream cmd;
    cmd << "cd " << shell_quote_single(root.string()) << " && ";
    for (std::size_t i = 0; i < order.size(); ++i) {
        auto it = tasks.find(order[i]);
        if (it == tasks.end() || it->second.cmd.empty()) {
            continue;
        }
        if (i > 0) {
            cmd << " && ";
        }
        cmd << "(" << it->second.cmd << ")";
    }
    if (only_command) {
        return cmd.str();
    }
    std::ostringstream out;
    out << "task=" << task << "\n";
    out << "order=";
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << order[i];
    }
    out << "\ncommand=" << cmd.str() << "\n";
    return out.str();
}

std::unordered_map<std::string, std::string> parse_simple_kv_file(const fs::path& file) {
    std::unordered_map<std::string, std::string> out;
    std::ifstream in(file);
    if (!in.is_open()) {
        return out;
    }
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        out[trim_copy(line.substr(0, eq))] = trim_copy(line.substr(eq + 1));
    }
    return out;
}

std::string cmd_profiler_timeline(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const fs::path telemetry = root / ".vitte-cache" / "vitte-ide-gtk" / "telemetry.log";
    std::ifstream in(telemetry);
    if (!in.is_open()) {
        return "profiler.timeline: no telemetry log";
    }

    struct Event {
        long long ts = 0;
        std::string name;
        std::string detail;
    };
    std::vector<Event> events;
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t p1 = line.find('|');
        const std::size_t p2 = p1 == std::string::npos ? std::string::npos : line.find('|', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) {
            continue;
        }
        Event ev;
        ev.ts = std::atoll(line.substr(0, p1).c_str());
        ev.name = line.substr(p1 + 1, p2 - p1 - 1);
        ev.detail = line.substr(p2 + 1);
        events.push_back(ev);
    }
    if (events.empty()) {
        return "profiler.timeline: empty";
    }
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.ts < b.ts; });

    std::ostringstream out;
    out << "timeline events=" << events.size() << "\n";
    long long prev_ts = events.front().ts;
    for (const Event& ev : events) {
        const long long dt = ev.ts - prev_ts;
        out << ev.ts << " +" << (dt < 0 ? 0 : dt) << "s " << ev.name << " " << ev.detail << "\n";
        prev_ts = ev.ts;
    }
    return out.str();
}

std::string cmd_profiler_hotspots(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    struct Hot {
        std::string sym;
        std::size_t refs = 0;
    };
    std::vector<Hot> tops;
    tops.reserve(idx.references.size());
    for (const auto& [sym, refs] : idx.references) {
        if (sym.size() < 3) {
            continue;
        }
        tops.push_back({sym, refs.size()});
    }
    std::sort(tops.begin(), tops.end(), [](const Hot& a, const Hot& b) { return a.refs > b.refs; });
    if (tops.empty()) {
        return "profiler.hotspots: none";
    }
    std::ostringstream out;
    out << "hotspots (top 30 by references)\n";
    const std::size_t n = std::min<std::size_t>(30, tops.size());
    for (std::size_t i = 0; i < n; ++i) {
        out << (i + 1) << ". " << tops[i].sym << " refs=" << tops[i].refs << "\n";
    }
    return out.str();
}

std::string cmd_memory_watch_snapshot(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const fs::path prev_file = root / ".vitte-cache" / "vitte-ide-gtk" / "memory_watch.prev";
    fs::create_directories(prev_file.parent_path());

    struct rusage usage {};
    getrusage(RUSAGE_SELF, &usage);
    const long maxrss_kb = usage.ru_maxrss;

    std::ifstream proc("/proc/self/status");
    long vm_rss_kb = -1;
    long vm_size_kb = -1;
    std::string line;
    while (std::getline(proc, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            vm_rss_kb = std::atol(line.substr(6).c_str());
        } else if (line.rfind("VmSize:", 0) == 0) {
            vm_size_kb = std::atol(line.substr(7).c_str());
        }
    }

    std::ofstream out_prev(prev_file);
    if (out_prev.is_open()) {
        out_prev << "maxrss_kb=" << maxrss_kb << "\n";
        out_prev << "vm_rss_kb=" << vm_rss_kb << "\n";
        out_prev << "vm_size_kb=" << vm_size_kb << "\n";
    }

    std::ostringstream out;
    out << "memory.snapshot\n";
    out << "maxrss_kb=" << maxrss_kb << "\n";
    out << "vm_rss_kb=" << vm_rss_kb << "\n";
    out << "vm_size_kb=" << vm_size_kb << "\n";
    out << "saved=" << prev_file.string() << "\n";
    return out.str();
}

std::string cmd_memory_watch_diff(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const fs::path prev_file = root / ".vitte-cache" / "vitte-ide-gtk" / "memory_watch.prev";
    const auto prev = parse_simple_kv_file(prev_file);
    if (prev.empty()) {
        return "memory.diff: no snapshot found";
    }
    struct rusage usage {};
    getrusage(RUSAGE_SELF, &usage);
    const long cur_maxrss = usage.ru_maxrss;

    std::ifstream proc("/proc/self/status");
    long cur_vm_rss = -1;
    long cur_vm_size = -1;
    std::string line;
    while (std::getline(proc, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            cur_vm_rss = std::atol(line.substr(6).c_str());
        } else if (line.rfind("VmSize:", 0) == 0) {
            cur_vm_size = std::atol(line.substr(7).c_str());
        }
    }
    const long prev_maxrss = prev.count("maxrss_kb") ? std::atol(prev.at("maxrss_kb").c_str()) : 0;
    const long prev_vm_rss = prev.count("vm_rss_kb") ? std::atol(prev.at("vm_rss_kb").c_str()) : 0;
    const long prev_vm_size = prev.count("vm_size_kb") ? std::atol(prev.at("vm_size_kb").c_str()) : 0;

    std::ostringstream out;
    out << "memory.diff\n";
    out << "maxrss_kb=" << cur_maxrss << " delta=" << (cur_maxrss - prev_maxrss) << "\n";
    out << "vm_rss_kb=" << cur_vm_rss << " delta=" << (cur_vm_rss - prev_vm_rss) << "\n";
    out << "vm_size_kb=" << cur_vm_size << " delta=" << (cur_vm_size - prev_vm_size) << "\n";
    return out.str();
}

std::string cmd_security_lint(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const fs::path policy_file = root / ".vitte-security-policy.json";
    std::string policy_txt;
    if (fs::exists(policy_file)) {
        std::ifstream in(policy_file);
        std::stringstream ss;
        ss << in.rdbuf();
        policy_txt = ss.str();
    }

    std::vector<std::string> deny = {"vitte/ffi", "vitte/process", "vitte/net", "vitte/http_client", "vitte/crypto"};
    const std::vector<std::string> allow_from_policy = find_quoted_strings(find_json_object_block(policy_txt, "allow"));
    const std::vector<std::string> deny_from_policy = find_quoted_strings(find_json_object_block(policy_txt, "deny"));
    if (!deny_from_policy.empty()) {
        deny = deny_from_policy;
    }
    std::unordered_set<std::string> allow(allow_from_policy.begin(), allow_from_policy.end());
    std::unordered_set<std::string> deny_set(deny.begin(), deny.end());

    std::ostringstream out;
    int issues = 0;
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || e.path().extension() != ".vit") {
            continue;
        }
        const std::string rel = fs::relative(e.path(), root).string();
        auto lines = read_lines(e.path());
        for (std::size_t i = 0; i < lines.size(); ++i) {
            UseLineInfo u = parse_use_line(lines[i]);
            if (!u.is_use) {
                continue;
            }
            if (deny_set.find(u.path) != deny_set.end() && allow.find(u.path) == allow.end()) {
                out << rel << ":" << (i + 1) << ":1:error:sensitive import denied by policy: " << u.path << "\n";
                ++issues;
            }
        }
    }
    if (issues == 0) {
        return "security-lint: clean";
    }
    return out.str();
}

bool is_snake_case(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    if (std::isdigit(static_cast<unsigned char>(s[0])) != 0) {
        return false;
    }
    for (char c : s) {
        if (!(std::islower(static_cast<unsigned char>(c)) != 0 || std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '_')) {
            return false;
        }
    }
    return true;
}

std::string basename_no_ext(const fs::path& p) {
    return p.stem().string();
}

std::string cmd_style_enforcer(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    std::ostringstream out;
    int issues = 0;

    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || e.path().extension() != ".vit") {
            continue;
        }
        const std::string base = basename_no_ext(e.path());
        if (!is_snake_case(base)) {
            out << fs::relative(e.path(), root).string() << ":1:1:warning:file name should be snake_case\n";
            ++issues;
        }
    }

    for (const SymbolInfo& s : idx.definitions) {
        if (s.kind == "entry" && s.name == "main") {
            continue;
        }
        if (!is_snake_case(s.name)) {
            out << s.file << ":" << s.line << ":" << s.col << ":warning:symbol should be snake_case: " << s.name << "\n";
            ++issues;
        }
    }

    if (issues == 0) {
        return "style-enforcer: clean";
    }
    return out.str();
}

std::string cmd_task_orchestrator_validate(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const auto tasks = parse_tasks_file(root / "tasks.vitte.json");
    if (tasks.empty()) {
        return "tasks: none";
    }
    std::ostringstream out;
    int issues = 0;

    for (const auto& [name, task] : tasks) {
        for (const std::string& dep : task.deps) {
            if (tasks.find(dep) == tasks.end()) {
                out << "tasks.vitte.json:1:1:error:task '" << name << "' depends on unknown task '" << dep << "'\n";
                ++issues;
            }
        }
    }
    for (const auto& [name, _] : tasks) {
        std::string err;
        (void)resolve_task_order(tasks, name, &err);
        if (!err.empty() && err.find("cycle") != std::string::npos) {
            out << "tasks.vitte.json:1:1:error:" << err << "\n";
            ++issues;
            break;
        }
    }

    if (issues == 0) {
        return "tasks: valid";
    }
    return out.str();
}

std::string semantic_index_path(const fs::path& root) {
    fs::path p = root / ".vitte-cache" / "vitte-ide-gtk" / "semantic.index";
    fs::create_directories(p.parent_path());
    return p.string();
}

std::string cmd_semantic_search_index(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const WorkspaceIndex idx = build_workspace_index(root);
    std::ofstream out(semantic_index_path(root));
    if (!out.is_open()) {
        return "semantic-search.index: write failed";
    }
    int rows = 0;
    for (const SymbolInfo& d : idx.definitions) {
        out << d.name << "|" << d.kind << "|" << d.file << "|" << d.line << "|" << d.col << "\n";
        ++rows;
    }
    return "semantic-search.index: " + std::to_string(rows) + " symbol(s)";
}

std::string cmd_semantic_search_query(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::string query = p.count("q") ? trim_copy(p.at("q")) : "";
    if (query.empty()) {
        return "semantic-search.query: empty query";
    }
    const fs::path idx_path = semantic_index_path(root);
    if (!fs::exists(idx_path)) {
        (void)cmd_semantic_search_index(p);
    }
    std::ifstream in(idx_path);
    if (!in.is_open()) {
        return "semantic-search.query: index unreadable";
    }
    std::string ql = query;
    std::transform(ql.begin(), ql.end(), ql.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::ostringstream out;
    std::string line;
    int hits = 0;
    while (std::getline(in, line)) {
        std::string ll = line;
        std::transform(ll.begin(), ll.end(), ll.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ll.find(ql) == std::string::npos) {
            continue;
        }
        const std::size_t p1 = line.find('|');
        const std::size_t p2 = p1 == std::string::npos ? std::string::npos : line.find('|', p1 + 1);
        const std::size_t p3 = p2 == std::string::npos ? std::string::npos : line.find('|', p2 + 1);
        const std::size_t p4 = p3 == std::string::npos ? std::string::npos : line.find('|', p3 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos || p4 == std::string::npos) {
            continue;
        }
        const std::string sym = line.substr(0, p1);
        const std::string kind = line.substr(p1 + 1, p2 - p1 - 1);
        const std::string src = line.substr(p2 + 1, p3 - p2 - 1);
        const int ln = std::max(1, std::atoi(line.substr(p3 + 1, p4 - p3 - 1).c_str()));
        const int col = std::max(1, std::atoi(line.substr(p4 + 1).c_str()));
        out << src << ":" << ln << ":" << col << ":info:" << kind << " " << sym << "\n";
        ++hits;
    }
    if (hits == 0) {
        return "semantic-search.query: no result";
    }
    return out.str();
}

std::string cmd_git_assistant_diff(const std::unordered_map<std::string, std::string>& p) {
    if (!p.count("file")) {
        return "git.diff: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    const fs::path root = guess_workspace_root(file);
    const fs::path rel = fs::relative(file, root);
    const std::string cmd = "git -C " + shell_quote_single(root.string()) + " diff -- " + shell_quote_single(rel.string());
    int rc = 0;
    std::string out = exec_capture(cmd, &rc);
    if (rc != 0) {
        return "git.diff: command failed";
    }
    return out.empty() ? "git.diff: clean" : out;
}

std::string cmd_git_assistant_blame(const std::unordered_map<std::string, std::string>& p) {
    if (!p.count("file")) {
        return "git.blame: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    const fs::path root = guess_workspace_root(file);
    const fs::path rel = fs::relative(file, root);
    const int line = p.count("line") ? std::max(1, std::atoi(p.at("line").c_str())) : 1;
    const std::string cmd = "git -C " + shell_quote_single(root.string()) + " blame -L " + std::to_string(line) + "," +
                            std::to_string(line) + " -- " + shell_quote_single(rel.string());
    int rc = 0;
    const std::string out = exec_capture(cmd, &rc);
    if (rc != 0) {
        return "git.blame: command failed";
    }
    return out.empty() ? "git.blame: no output" : out;
}

bool parse_unified0_hunk_header(const std::string& header, int* new_start, int* new_count) {
    const std::size_t plus = header.find('+');
    if (plus == std::string::npos) {
        return false;
    }
    std::size_t i = plus + 1;
    std::size_t j = i;
    while (j < header.size() && std::isdigit(static_cast<unsigned char>(header[j])) != 0) {
        ++j;
    }
    if (j == i) {
        return false;
    }
    const int s = std::atoi(header.substr(i, j - i).c_str());
    int c = 1;
    if (j < header.size() && header[j] == ',') {
        std::size_t k = j + 1;
        std::size_t l = k;
        while (l < header.size() && std::isdigit(static_cast<unsigned char>(header[l])) != 0) {
            ++l;
        }
        if (l > k) {
            c = std::atoi(header.substr(k, l - k).c_str());
        }
    }
    if (new_start != nullptr) {
        *new_start = s;
    }
    if (new_count != nullptr) {
        *new_count = c;
    }
    return true;
}

std::string cmd_git_assistant_stage_hunk(const std::unordered_map<std::string, std::string>& p) {
    if (!p.count("file")) {
        return "git.stage-hunk: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    const fs::path root = guess_workspace_root(file);
    const fs::path rel = fs::relative(file, root);
    const int target_line = p.count("line") ? std::max(1, std::atoi(p.at("line").c_str())) : 1;

    const std::string diff_cmd = "git -C " + shell_quote_single(root.string()) + " diff --unified=0 -- " + shell_quote_single(rel.string());
    int rc = 0;
    const std::string diff = exec_capture(diff_cmd, &rc);
    if (rc != 0 || diff.empty()) {
        return "git.stage-hunk: no diff";
    }

    std::istringstream in(diff);
    std::string line;
    std::string header;
    std::string selected_hunk;
    bool in_hunk = false;
    std::ostringstream current_hunk;
    int hs = 0;
    int hc = 0;
    bool selected = false;

    while (std::getline(in, line)) {
        if (line.rfind("@@ ", 0) == 0) {
            if (in_hunk && selected && selected_hunk.empty()) {
                selected_hunk = current_hunk.str();
            }
            in_hunk = true;
            current_hunk.str("");
            current_hunk.clear();
            current_hunk << line << "\n";
            selected = parse_unified0_hunk_header(line, &hs, &hc) &&
                       target_line >= hs && target_line < hs + std::max(1, hc);
            continue;
        }
        if (in_hunk) {
            current_hunk << line << "\n";
        } else if (line.rfind("diff --git ", 0) == 0 || line.rfind("--- ", 0) == 0 || line.rfind("+++ ", 0) == 0) {
            header += line + "\n";
        }
    }
    if (in_hunk && selected && selected_hunk.empty()) {
        selected_hunk = current_hunk.str();
    }
    if (selected_hunk.empty()) {
        return "git.stage-hunk: no hunk at target line";
    }

    if (header.empty()) {
        header = "diff --git a/" + rel.string() + " b/" + rel.string() + "\n"
                 "--- a/" + rel.string() + "\n"
                 "+++ b/" + rel.string() + "\n";
    }
    const fs::path patch_file = root / ".vitte-cache" / "vitte-ide-gtk" / "stage_hunk.patch";
    fs::create_directories(patch_file.parent_path());
    {
        std::ofstream outp(patch_file);
        outp << header << selected_hunk;
    }
    const std::string apply_cmd = "git -C " + shell_quote_single(root.string()) + " apply --cached --unidiff-zero " +
                                  shell_quote_single(patch_file.string()) + " 2>&1";
    const std::string apply_out = exec_capture(apply_cmd, &rc);
    if (rc != 0) {
        return "git.stage-hunk: apply failed\n" + apply_out;
    }
    return "git.stage-hunk: staged at " + rel.string() + ":" + std::to_string(target_line);
}

std::string cmd_terminal_actions_extract(const std::unordered_map<std::string, std::string>& p) {
    if (!p.count("log_file")) {
        return "terminal-actions: payload requires log_file";
    }
    const fs::path log_file = fs::path(p.at("log_file"));
    auto lines = read_lines(log_file);
    if (lines.empty()) {
        return "terminal-actions: empty log";
    }
    std::ostringstream out;
    int actions = 0;
    for (const std::string& ln : lines) {
        const std::size_t p1 = ln.find(':');
        const std::size_t p2 = p1 == std::string::npos ? std::string::npos : ln.find(':', p1 + 1);
        if (p1 != std::string::npos && p2 != std::string::npos) {
            bool num1 = true;
            for (std::size_t i = p1 + 1; i < p2; ++i) {
                if (std::isdigit(static_cast<unsigned char>(ln[i])) == 0) {
                    num1 = false;
                    break;
                }
            }
            if (num1) {
                std::size_t p3 = ln.find(':', p2 + 1);
                int col = 1;
                std::size_t msg_pos = p2 + 1;
                if (p3 != std::string::npos) {
                    bool num2 = true;
                    for (std::size_t i = p2 + 1; i < p3; ++i) {
                        if (std::isdigit(static_cast<unsigned char>(ln[i])) == 0) {
                            num2 = false;
                            break;
                        }
                    }
                    if (num2) {
                        col = std::max(1, std::atoi(ln.substr(p2 + 1, p3 - p2 - 1).c_str()));
                        msg_pos = p3 + 1;
                    }
                }
                const std::string file = ln.substr(0, p1);
                const int lno = std::max(1, std::atoi(ln.substr(p1 + 1, p2 - p1 - 1).c_str()));
                const std::string msg = ln.substr(msg_pos);
                std::string sev = "info";
                const std::string lcl = lower_copy(msg);
                if (lcl.find("error") != std::string::npos) {
                    sev = "error";
                } else if (lcl.find("warning") != std::string::npos) {
                    sev = "warning";
                }
                out << file << ":" << lno << ":" << col << ":" << sev << ":terminal action: " << msg << "\n";
                ++actions;
            }
        }
    }
    if (actions == 0) {
        return "terminal-actions: no actionable entries";
    }
    return out.str();
}

std::string cmd_notebook_list(const std::unordered_map<std::string, std::string>& p) {
    const std::string file = p.count("file") ? p.at("file") : "";
    std::vector<std::string> names = {"entry main", "use module alias", "proc template", "trait template"};
    if (file.size() >= 4 && file.substr(file.size() - 4) == ".vit") {
        names.push_back("test template");
        names.push_back("bench template");
    } else if (file.size() >= 4 && file.substr(file.size() - 4) == ".cpp") {
        names.push_back("cpp class template");
    }
    std::ostringstream csv;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            csv << ",";
        }
        csv << names[i];
    }
    return csv.str();
}

std::string cmd_notebook_expand(const std::unordered_map<std::string, std::string>& p) {
    const std::string name = p.count("name") ? p.at("name") : "";
    if (name == "use module alias") {
        return "use vitte/core as core_pkg\n";
    }
    if (name == "proc template") {
        return "proc name(args: int) -> int {\n  give args\n}\n";
    }
    if (name == "trait template") {
        return "trait Name {\n  proc run(self: Name) -> int\n}\n";
    }
    if (name == "test template") {
        return "proc test_name() -> int {\n  give 0\n}\n";
    }
    if (name == "bench template") {
        return "proc bench_name() -> int {\n  give 0\n}\n";
    }
    if (name == "cpp class template") {
        return "class Name {\n public:\n  int run();\n};\n";
    }
    return "entry main at core/app {\n  give 0\n}\n";
}

std::string cmd_build_matrix_plan(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::string target = p.count("target") ? trim_copy(p.at("target")) : "all";
    std::vector<std::string> targets;
    if (target == "all") {
        targets = {"check", "build", "test"};
    } else {
        targets = split_csv_trimmed(target);
    }
    if (targets.empty()) {
        targets.push_back("build");
    }
    std::ostringstream cmd;
    cmd << "cd " << shell_quote_single(root.string()) << " && ";
    cmd << "printf '%s\\n' ";
    for (std::size_t i = 0; i < targets.size(); ++i) {
        if (i > 0) {
            cmd << " ";
        }
        cmd << shell_quote_single(targets[i]);
    }
    cmd << " | xargs -I{} -P 2 sh -lc 'VITTE_PROFILE=Debug vitte {} >/tmp/vitte_matrix_debug_{}.log 2>&1; "
           "echo debug:{}:$?; VITTE_PROFILE=Release vitte {} >/tmp/vitte_matrix_release_{}.log 2>&1; echo release:{}:$?'";
    return "command=" + cmd.str() + "\n";
}

std::string cmd_std_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"std-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_std_mod_contracts.py"},
        {"std-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_std_no_internal_exports.py"},
        {"std-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/std_contract_snapshots.sh"},
        {"std-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/std_facade_snapshot.sh"},
        {"std-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_std_compat_contracts.py"},
        {"std-security-gate", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_std_sensitive_imports.py"},
    };

    std::ostringstream out;
    out << "Std API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_log_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"log-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_log_mod_contracts.py"},
        {"log-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_log_no_internal_exports.py"},
        {"log-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/log_contract_snapshots.sh"},
        {"log-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/log_facade_snapshot.sh"},
        {"log-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_log_compat_contracts.py"},
        {"log-security-gate", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_log_sensitive_imports.py"},
    };

    std::ostringstream out;
    out << "Log API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_log_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "log.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "log.quickfix-config: empty/unreadable";
    }

    bool has_min_level = false;
    bool has_sink = false;
    bool has_format = false;
    int changes = 0;

    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("min_level=", 0) == 0) {
            has_min_level = true;
            const std::string v = lower_copy(trim_copy(t.substr(10)));
            if (!(v == "trace" || v == "debug" || v == "info" || v == "warn" || v == "error" || v == "fatal")) {
                line = "min_level=info";
                ++changes;
            }
        } else if (t.rfind("sink=", 0) == 0) {
            has_sink = true;
            const std::string v = lower_copy(trim_copy(t.substr(5)));
            if (!(v == "stdout" || v == "stderr" || v == "file" || v == "syslog" || v == "journald" || v == "memory_ring")) {
                line = "sink=stderr";
                ++changes;
            }
        } else if (t.rfind("format=", 0) == 0) {
            has_format = true;
            const std::string v = lower_copy(trim_copy(t.substr(7)));
            if (!(v == "text" || v == "jsonl" || v == "jsonlines")) {
                line = "format=text";
                ++changes;
            }
        }
    }
    if (!has_min_level) {
        lines.push_back("min_level=info");
        ++changes;
    }
    if (!has_sink) {
        lines.push_back("sink=stderr");
        ++changes;
    }
    if (!has_format) {
        lines.push_back("format=text");
        ++changes;
    }

    if (changes == 0) {
        return "log.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "log.quickfix-config: write failed";
        }
        return "log.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "log.quickfix-config preview:\n";
    out << "- normalize min_level/sink/format values\n";
    out << "- add missing required keys\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_fs_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"fs-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_fs_mod_contracts.py"},
        {"fs-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_fs_no_internal_exports.py"},
        {"fs-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/fs_contract_snapshots.sh"},
        {"fs-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/fs_facade_snapshot.sh"},
        {"fs-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_fs_compat_contracts.py"},
        {"fs-security-gate", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_fs_sensitive_imports.py"},
    };

    std::ostringstream out;
    out << "FS API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_db_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"db-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_db_mod_contracts.py"},
        {"db-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_db_no_internal_exports.py"},
        {"db-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/db_contract_snapshots.sh"},
        {"db-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/db_facade_snapshot.sh"},
        {"db-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_db_compat_contracts.py"},
        {"db-sql-injection-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_db_sql_injection.py"},
        {"db-migration-compat", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_db_migration_compat.py"},
    };

    std::ostringstream out;
    out << "DB API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_db_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "db.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "db.quickfix-config: empty/unreadable";
    }

    bool has_timeout = false;
    bool has_pool_min = false;
    bool has_pool_max = false;
    bool has_retry = false;
    int changes = 0;

    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("timeout_ms=", 0) == 0) {
            has_timeout = true;
            int v = std::atoi(trim_copy(t.substr(11)).c_str());
            if (v <= 0) {
                line = "timeout_ms=2000";
                ++changes;
            }
        } else if (t.rfind("pool_min=", 0) == 0) {
            has_pool_min = true;
            int v = std::atoi(trim_copy(t.substr(9)).c_str());
            if (v < 1) {
                line = "pool_min=1";
                ++changes;
            }
        } else if (t.rfind("pool_max=", 0) == 0) {
            has_pool_max = true;
            int v = std::atoi(trim_copy(t.substr(9)).c_str());
            if (v < 1) {
                line = "pool_max=8";
                ++changes;
            }
        } else if (t.rfind("retry_budget=", 0) == 0) {
            has_retry = true;
            int v = std::atoi(trim_copy(t.substr(13)).c_str());
            if (v < 0) {
                line = "retry_budget=3";
                ++changes;
            }
        }
    }
    if (!has_timeout) {
        lines.push_back("timeout_ms=2000");
        ++changes;
    }
    if (!has_pool_min) {
        lines.push_back("pool_min=1");
        ++changes;
    }
    if (!has_pool_max) {
        lines.push_back("pool_max=8");
        ++changes;
    }
    if (!has_retry) {
        lines.push_back("retry_budget=3");
        ++changes;
    }

    if (changes == 0) {
        return "db.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "db.quickfix-config: write failed";
        }
        return "db.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "db.quickfix-config preview:\n";
    out << "- normalize timeout/pool/retry values\n";
    out << "- add missing required keys\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_http_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"http-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_mod_contracts.py"},
        {"http-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_no_internal_exports.py"},
        {"http-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/http_contract_snapshots.sh"},
        {"http-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/http_facade_snapshot.sh"},
        {"http-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_compat_contracts.py"},
        {"http-sensitive-imports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_sensitive_imports.py"},
        {"http-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_security.py"},
    };

    std::ostringstream out;
    out << "HTTP API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_http_client_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"http-client-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_client_mod_contracts.py"},
        {"http-client-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_no_internal_exports.py"},
        {"http-client-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/http_client_contract_snapshots.sh"},
        {"http-client-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/http_client_facade_snapshot.sh"},
        {"http-client-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_compat_contracts.py"},
        {"http-client-sensitive-imports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_sensitive_imports.py"},
        {"http-client-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_http_security.py"},
    };

    std::ostringstream out;
    out << "HTTP Client API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_http_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "http.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "http.quickfix-config: empty/unreadable";
    }

    bool has_read = false;
    bool has_write = false;
    bool has_body = false;
    bool has_rate = false;
    bool has_cors = false;
    int changes = 0;

    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("read_timeout_ms=", 0) == 0) {
            has_read = true;
            int v = std::atoi(trim_copy(t.substr(16)).c_str());
            if (v <= 0) {
                line = "read_timeout_ms=2000";
                ++changes;
            }
        } else if (t.rfind("write_timeout_ms=", 0) == 0) {
            has_write = true;
            int v = std::atoi(trim_copy(t.substr(17)).c_str());
            if (v <= 0) {
                line = "write_timeout_ms=2000";
                ++changes;
            }
        } else if (t.rfind("body_limit=", 0) == 0) {
            has_body = true;
            int v = std::atoi(trim_copy(t.substr(11)).c_str());
            if (v <= 0) {
                line = "body_limit=1048576";
                ++changes;
            }
        } else if (t.rfind("rate_limit=", 0) == 0) {
            has_rate = true;
            int v = std::atoi(trim_copy(t.substr(11)).c_str());
            if (v <= 0) {
                line = "rate_limit=1000";
                ++changes;
            }
        } else if (t.rfind("cors_allow_origin=", 0) == 0) {
            has_cors = true;
            if (trim_copy(t.substr(18)).empty()) {
                line = "cors_allow_origin=*";
                ++changes;
            }
        }
    }

    if (!has_read) {
        lines.push_back("read_timeout_ms=2000");
        ++changes;
    }
    if (!has_write) {
        lines.push_back("write_timeout_ms=2000");
        ++changes;
    }
    if (!has_body) {
        lines.push_back("body_limit=1048576");
        ++changes;
    }
    if (!has_rate) {
        lines.push_back("rate_limit=1000");
        ++changes;
    }
    if (!has_cors) {
        lines.push_back("cors_allow_origin=*");
        ++changes;
    }

    if (changes == 0) {
        return "http.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "http.quickfix-config: write failed";
        }
        return "http.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "http.quickfix-config preview:\n";
    out << "- normalize timeout/body/rate/cors values\n";
    out << "- add missing required keys\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_http_client_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "http-client.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "http-client.quickfix-config: empty/unreadable";
    }

    bool has_connect = false;
    bool has_read = false;
    bool has_retry = false;
    bool has_tls = false;
    bool has_ssrf = false;
    int changes = 0;

    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("connect_timeout_ms=", 0) == 0) {
            has_connect = true;
            int v = std::atoi(trim_copy(t.substr(19)).c_str());
            if (v <= 0) {
                line = "connect_timeout_ms=1000";
                ++changes;
            }
        } else if (t.rfind("read_timeout_ms=", 0) == 0) {
            has_read = true;
            int v = std::atoi(trim_copy(t.substr(16)).c_str());
            if (v <= 0) {
                line = "read_timeout_ms=2000";
                ++changes;
            }
        } else if (t.rfind("retry_budget=", 0) == 0) {
            has_retry = true;
            int v = std::atoi(trim_copy(t.substr(13)).c_str());
            if (v < 0) {
                line = "retry_budget=2";
                ++changes;
            }
        } else if (t.rfind("strict_tls=", 0) == 0) {
            has_tls = true;
            std::string v = lower_copy(trim_copy(t.substr(11)));
            if (!(v == "true" || v == "false")) {
                line = "strict_tls=true";
                ++changes;
            }
        } else if (t.rfind("allow_private_hosts=", 0) == 0) {
            has_ssrf = true;
            std::string v = lower_copy(trim_copy(t.substr(20)));
            if (!(v == "true" || v == "false")) {
                line = "allow_private_hosts=false";
                ++changes;
            }
        }
    }

    if (!has_connect) {
        lines.push_back("connect_timeout_ms=1000");
        ++changes;
    }
    if (!has_read) {
        lines.push_back("read_timeout_ms=2000");
        ++changes;
    }
    if (!has_retry) {
        lines.push_back("retry_budget=2");
        ++changes;
    }
    if (!has_tls) {
        lines.push_back("strict_tls=true");
        ++changes;
    }
    if (!has_ssrf) {
        lines.push_back("allow_private_hosts=false");
        ++changes;
    }

    if (changes == 0) {
        return "http-client.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "http-client.quickfix-config: write failed";
        }
        return "http-client.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "http-client.quickfix-config preview:\n";
    out << "- normalize timeout/retry/tls/ssrf values\n";
    out << "- add missing required keys\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_process_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"process-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_process_mod_contracts.py"},
        {"process-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_process_no_internal_exports.py"},
        {"process-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/process_contract_snapshots.sh"},
        {"process-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/process_facade_snapshot.sh"},
        {"process-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_process_compat_contracts.py"},
        {"process-sensitive-imports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_process_sensitive_imports.py"},
        {"process-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_process_security.py"},
    };

    std::ostringstream out;
    out << "Process API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_process_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "process.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "process.quickfix-config: empty/unreadable";
    }

    bool has_timeout = false;
    bool has_grace = false;
    bool has_capture = false;
    bool has_shell = false;
    bool has_cwd = false;
    int changes = 0;

    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("timeout_ms=", 0) == 0) {
            has_timeout = true;
            int v = std::atoi(trim_copy(t.substr(11)).c_str());
            if (v <= 0) {
                line = "timeout_ms=2000";
                ++changes;
            }
        } else if (t.rfind("grace_ms=", 0) == 0) {
            has_grace = true;
            int v = std::atoi(trim_copy(t.substr(9)).c_str());
            if (v <= 0) {
                line = "grace_ms=200";
                ++changes;
            }
        } else if (t.rfind("capture_limit=", 0) == 0) {
            has_capture = true;
            int v = std::atoi(trim_copy(t.substr(14)).c_str());
            if (v <= 0) {
                line = "capture_limit=4096";
                ++changes;
            }
        } else if (t.rfind("allow_shell=", 0) == 0) {
            has_shell = true;
            std::string v = lower_copy(trim_copy(t.substr(12)));
            if (!(v == "true" || v == "false")) {
                line = "allow_shell=false";
                ++changes;
            }
        } else if (t.rfind("cwd=", 0) == 0) {
            has_cwd = true;
            std::string v = trim_copy(t.substr(4));
            if (v.empty() || v[0] != '/') {
                line = "cwd=/workspace";
                ++changes;
            }
        }
    }

    if (!has_timeout) {
        lines.push_back("timeout_ms=2000");
        ++changes;
    }
    if (!has_grace) {
        lines.push_back("grace_ms=200");
        ++changes;
    }
    if (!has_capture) {
        lines.push_back("capture_limit=4096");
        ++changes;
    }
    if (!has_shell) {
        lines.push_back("allow_shell=false");
        ++changes;
    }
    if (!has_cwd) {
        lines.push_back("cwd=/workspace");
        ++changes;
    }

    if (changes == 0) {
        return "process.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "process.quickfix-config: write failed";
        }
        return "process.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "process.quickfix-config preview:\n";
    out << "- normalize timeout/grace/capture/shell/cwd values\n";
    out << "- add missing required keys\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_json_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"json-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_json_mod_contracts.py"},
        {"json-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_json_no_internal_exports.py"},
        {"json-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/json_contract_snapshots.sh"},
        {"json-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/json_facade_snapshot.sh"},
        {"json-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_json_compat_contracts.py"},
        {"json-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_json_security.py"},
    };

    std::ostringstream out;
    out << "JSON API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_yaml_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"yaml-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_yaml_mod_contracts.py"},
        {"yaml-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_yaml_no_internal_exports.py"},
        {"yaml-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/yaml_contract_snapshots.sh"},
        {"yaml-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/yaml_facade_snapshot.sh"},
        {"yaml-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_yaml_compat_contracts.py"},
        {"yaml-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_yaml_security.py"},
    };

    std::ostringstream out;
    out << "YAML API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_json_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "json.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "json.quickfix-config: empty/unreadable";
    }

    bool has_max = false;
    bool has_depth = false;
    bool has_utf8 = false;
    int changes = 0;
    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("max_bytes=", 0) == 0) {
            has_max = true;
            int v = std::atoi(trim_copy(t.substr(10)).c_str());
            if (v <= 0) {
                line = "max_bytes=1048576";
                ++changes;
            }
        } else if (t.rfind("max_depth=", 0) == 0) {
            has_depth = true;
            int v = std::atoi(trim_copy(t.substr(10)).c_str());
            if (v <= 0) {
                line = "max_depth=64";
                ++changes;
            }
        } else if (t.rfind("strict_utf8=", 0) == 0) {
            has_utf8 = true;
            std::string v = lower_copy(trim_copy(t.substr(12)));
            if (!(v == "true" || v == "false")) {
                line = "strict_utf8=true";
                ++changes;
            }
        }
    }
    if (!has_max) {
        lines.push_back("max_bytes=1048576");
        ++changes;
    }
    if (!has_depth) {
        lines.push_back("max_depth=64");
        ++changes;
    }
    if (!has_utf8) {
        lines.push_back("strict_utf8=true");
        ++changes;
    }

    if (changes == 0) {
        return "json.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "json.quickfix-config: write failed";
        }
        return "json.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "json.quickfix-config preview:\n";
    out << "- normalize parse limits and strict utf8\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_yaml_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "yaml.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "yaml.quickfix-config: empty/unreadable";
    }

    bool has_indent = false;
    bool has_safe = false;
    bool has_allow = false;
    int changes = 0;
    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.rfind("indent=", 0) == 0) {
            has_indent = true;
            int v = std::atoi(trim_copy(t.substr(7)).c_str());
            if (v <= 0) {
                line = "indent=2";
                ++changes;
            }
        } else if (t.rfind("safe_load=", 0) == 0) {
            has_safe = true;
            std::string v = lower_copy(trim_copy(t.substr(10)));
            if (!(v == "true" || v == "false")) {
                line = "safe_load=true";
                ++changes;
            }
        } else if (t.rfind("allow_tags=", 0) == 0) {
            has_allow = true;
        }
    }
    if (!has_indent) {
        lines.push_back("indent=2");
        ++changes;
    }
    if (!has_safe) {
        lines.push_back("safe_load=true");
        ++changes;
    }
    if (!has_allow) {
        lines.push_back("allow_tags=");
        ++changes;
    }

    if (changes == 0) {
        return "yaml.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "yaml.quickfix-config: write failed";
        }
        return "yaml.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "yaml.quickfix-config preview:\n";
    out << "- normalize indent and safe_load defaults\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_test_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"test-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_test_mod_contracts.py"},
        {"test-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_test_no_internal_exports.py"},
        {"test-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/test_contract_snapshots.sh"},
        {"test-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/test_facade_snapshot.sh"},
        {"test-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_test_compat_contracts.py"},
        {"test-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_test_security.py"},
    };

    std::ostringstream out;
    out << "Test API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_lint_doctor(const std::unordered_map<std::string, std::string>& p) {
    const fs::path file = p.count("file") ? fs::path(p.at("file")) : fs::path();
    const fs::path root = file.empty() ? fs::current_path() : guess_workspace_root(file);
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"lint-mod-lint", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_lint_mod_contracts.py"},
        {"lint-no-internal-exports", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_lint_no_internal_exports.py"},
        {"lint-contract-snapshots", "cd " + shell_quote_single(root.string()) + " && tools/lint_contract_snapshots_pkg.sh"},
        {"lint-facade-snapshot", "cd " + shell_quote_single(root.string()) + " && tools/lint_facade_snapshot_pkg.sh"},
        {"lint-compat-contracts", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_lint_compat_contracts.py"},
        {"lint-security", "cd " + shell_quote_single(root.string()) + " && python3 tools/lint_lint_security.py"},
    };

    std::ostringstream out;
    out << "Lint API Doctor\n\n";
    for (const auto& [name, cmd] : checks) {
        int rc = 0;
        out << "## " << name << "\n";
        const std::string cmd_out = exec_capture(cmd + " 2>&1", &rc);
        out << cmd_out;
        if (!cmd_out.empty() && cmd_out.back() != '\n') {
            out << "\n";
        }
        out << "[exit " << rc << "]\n\n";
    }
    return out.str();
}

std::string cmd_lint_quickfix_config(const std::unordered_map<std::string, std::string>& p, bool apply) {
    if (!p.count("file")) {
        return "lint.quickfix-config: payload requires file";
    }
    const fs::path file = fs::path(p.at("file"));
    auto lines = read_lines(file);
    if (lines.empty()) {
        return "lint.quickfix-config: empty/unreadable";
    }

    int changes = 0;
    for (std::string& line : lines) {
        std::string t = trim_copy(line);
        if (t.find(" as ") != std::string::npos && t.find("_pkg") == std::string::npos) {
            std::size_t pos = t.find(" as ");
            if (pos != std::string::npos) {
                line = t + "_pkg";
                ++changes;
            }
        }
        if (t.rfind("entry ", 0) == 0) {
            line = "# " + t;
            ++changes;
        }
    }

    if (changes == 0) {
        return "lint.quickfix-config: nothing";
    }
    if (apply) {
        if (!write_lines(file, lines)) {
            return "lint.quickfix-config: write failed";
        }
        return "lint.quickfix-config: applied " + std::to_string(changes) + " fix(es)";
    }
    std::ostringstream out;
    out << "lint.quickfix-config preview:\n";
    out << "- normalize alias *_pkg and comment import-time entry points\n";
    out << "- estimated changes: " << changes << "\n";
    return out.str();
}

std::string cmd_commands_by_domain(const std::unordered_map<std::string, std::string>&) {
    std::ostringstream out;
    out << "doctor=vitte-std.doctor,vitte-log.doctor,vitte-fs.doctor,vitte-db.doctor,vitte-http.doctor,vitte-http-client.doctor,vitte-process.doctor,vitte-json.doctor,vitte-yaml.doctor,vitte-test.doctor,vitte-lint.doctor\n";
    out << "quickfix=vitte-log.quickfix-config.preview,vitte-log.quickfix-config.apply,vitte-db.quickfix-config.preview,vitte-db.quickfix-config.apply,vitte-http.quickfix-config.preview,vitte-http.quickfix-config.apply,vitte-http-client.quickfix-config.preview,vitte-http-client.quickfix-config.apply,vitte-process.quickfix-config.preview,vitte-process.quickfix-config.apply,vitte-json.quickfix-config.preview,vitte-json.quickfix-config.apply,vitte-yaml.quickfix-config.preview,vitte-yaml.quickfix-config.apply,vitte-lint.quickfix-config.preview,vitte-lint.quickfix-config.apply\n";
    out << "contract=vitte-refactor.rename.preview,vitte-refactor.rename.apply,vitte-docgen.preview\n";
    out << "security=vitte-security-lint.run\n";
    out << "perf=vitte-profiler.timeline,vitte-profiler.hotspots,vitte-build-matrix.plan\n";
    return out.str();
}

const char* commands_csv() {
    return "vitte-analyzer.completion,vitte-analyzer.hover,vitte-analyzer.signature,vitte-analyzer.refs,vitte-analyzer.goto,"
           "vitte-import-fix.preview,vitte-import-fix.apply,"
           "vitte-refactor.rename.preview,vitte-refactor.rename.apply,"
           "vitte-quickfix-pack.preview,vitte-quickfix-pack.apply,"
           "vitte-docgen.preview,"
           "vitte-test-runner.detect,"
           "vitte-task-orchestrator.list,vitte-task-orchestrator.plan,vitte-task-orchestrator.command,vitte-task-orchestrator.validate,"
           "vitte-profiler.timeline,vitte-profiler.hotspots,"
           "vitte-memory-watch.snapshot,vitte-memory-watch.diff,"
           "vitte-security-lint.run,vitte-style-enforcer.run,"
           "vitte-semantic-search.index,vitte-semantic-search.query,"
           "vitte-git-assistant.diff,vitte-git-assistant.blame,vitte-git-assistant.stage-hunk,"
           "vitte-terminal-actions.extract,"
           "vitte-notebook.list,vitte-notebook.expand,"
           "vitte-build-matrix.plan,"
           "vitte-std.doctor,"
           "vitte-log.doctor,"
           "vitte-log.quickfix-config.preview,vitte-log.quickfix-config.apply,"
           "vitte-fs.doctor,"
           "vitte-db.doctor,"
           "vitte-db.quickfix-config.preview,vitte-db.quickfix-config.apply,"
           "vitte-http.doctor,"
           "vitte-http.quickfix-config.preview,vitte-http.quickfix-config.apply,"
           "vitte-http-client.doctor,"
           "vitte-http-client.quickfix-config.preview,vitte-http-client.quickfix-config.apply,"
           "vitte-process.doctor,"
           "vitte-process.quickfix-config.preview,vitte-process.quickfix-config.apply,"
           "vitte-json.doctor,"
           "vitte-json.quickfix-config.preview,vitte-json.quickfix-config.apply,"
           "vitte-yaml.doctor,"
           "vitte-yaml.quickfix-config.preview,vitte-yaml.quickfix-config.apply,"
           "vitte-test.doctor,"
           "vitte-lint.doctor,"
           "vitte-lint.quickfix-config.preview,vitte-lint.quickfix-config.apply,"
           "vitte-commands.by-domain";
}

const char* run_command(const char* command_c, const char* payload_c) {
    const std::string command = command_c != nullptr ? command_c : "";
    const std::string payload = payload_c != nullptr ? payload_c : "";
    const auto p = parse_payload(payload);

    if (command == "vitte-analyzer.hover") {
        g_out = cmd_hover(p);
    } else if (command == "vitte-analyzer.signature") {
        g_out = cmd_signature(p);
    } else if (command == "vitte-analyzer.refs") {
        g_out = cmd_refs(p);
    } else if (command == "vitte-analyzer.goto") {
        g_out = cmd_goto(p);
    } else if (command == "vitte-analyzer.completion") {
        g_out = cmd_completion(p);
    } else if (command == "vitte-import-fix.preview") {
        g_out = cmd_import_fix(p, false);
    } else if (command == "vitte-import-fix.apply") {
        g_out = cmd_import_fix(p, true);
    } else if (command == "vitte-refactor.rename.preview") {
        g_out = cmd_refactor_rename(p, false);
    } else if (command == "vitte-refactor.rename.apply") {
        g_out = cmd_refactor_rename(p, true);
    } else if (command == "vitte-quickfix-pack.preview") {
        g_out = cmd_quickfix(p, false);
    } else if (command == "vitte-quickfix-pack.apply") {
        g_out = cmd_quickfix(p, true);
    } else if (command == "vitte-docgen.preview") {
        g_out = cmd_docgen_preview(p);
    } else if (command == "vitte-test-runner.detect") {
        g_out = cmd_test_runner(p);
    } else if (command == "vitte-task-orchestrator.list") {
        g_out = cmd_task_orchestrator_list(p);
    } else if (command == "vitte-task-orchestrator.plan") {
        g_out = cmd_task_orchestrator_plan(p, false);
    } else if (command == "vitte-task-orchestrator.command") {
        g_out = cmd_task_orchestrator_plan(p, true);
    } else if (command == "vitte-task-orchestrator.validate") {
        g_out = cmd_task_orchestrator_validate(p);
    } else if (command == "vitte-profiler.timeline") {
        g_out = cmd_profiler_timeline(p);
    } else if (command == "vitte-profiler.hotspots") {
        g_out = cmd_profiler_hotspots(p);
    } else if (command == "vitte-memory-watch.snapshot") {
        g_out = cmd_memory_watch_snapshot(p);
    } else if (command == "vitte-memory-watch.diff") {
        g_out = cmd_memory_watch_diff(p);
    } else if (command == "vitte-security-lint.run") {
        g_out = cmd_security_lint(p);
    } else if (command == "vitte-style-enforcer.run") {
        g_out = cmd_style_enforcer(p);
    } else if (command == "vitte-semantic-search.index") {
        g_out = cmd_semantic_search_index(p);
    } else if (command == "vitte-semantic-search.query") {
        g_out = cmd_semantic_search_query(p);
    } else if (command == "vitte-git-assistant.diff") {
        g_out = cmd_git_assistant_diff(p);
    } else if (command == "vitte-git-assistant.blame") {
        g_out = cmd_git_assistant_blame(p);
    } else if (command == "vitte-git-assistant.stage-hunk") {
        g_out = cmd_git_assistant_stage_hunk(p);
    } else if (command == "vitte-terminal-actions.extract") {
        g_out = cmd_terminal_actions_extract(p);
    } else if (command == "vitte-notebook.list") {
        g_out = cmd_notebook_list(p);
    } else if (command == "vitte-notebook.expand") {
        g_out = cmd_notebook_expand(p);
    } else if (command == "vitte-build-matrix.plan") {
        g_out = cmd_build_matrix_plan(p);
    } else if (command == "vitte-std.doctor") {
        g_out = cmd_std_doctor(p);
    } else if (command == "vitte-log.doctor") {
        g_out = cmd_log_doctor(p);
    } else if (command == "vitte-log.quickfix-config.preview") {
        g_out = cmd_log_quickfix_config(p, false);
    } else if (command == "vitte-log.quickfix-config.apply") {
        g_out = cmd_log_quickfix_config(p, true);
    } else if (command == "vitte-fs.doctor") {
        g_out = cmd_fs_doctor(p);
    } else if (command == "vitte-db.doctor") {
        g_out = cmd_db_doctor(p);
    } else if (command == "vitte-db.quickfix-config.preview") {
        g_out = cmd_db_quickfix_config(p, false);
    } else if (command == "vitte-db.quickfix-config.apply") {
        g_out = cmd_db_quickfix_config(p, true);
    } else if (command == "vitte-http.doctor") {
        g_out = cmd_http_doctor(p);
    } else if (command == "vitte-http.quickfix-config.preview") {
        g_out = cmd_http_quickfix_config(p, false);
    } else if (command == "vitte-http.quickfix-config.apply") {
        g_out = cmd_http_quickfix_config(p, true);
    } else if (command == "vitte-http-client.doctor") {
        g_out = cmd_http_client_doctor(p);
    } else if (command == "vitte-http-client.quickfix-config.preview") {
        g_out = cmd_http_client_quickfix_config(p, false);
    } else if (command == "vitte-http-client.quickfix-config.apply") {
        g_out = cmd_http_client_quickfix_config(p, true);
    } else if (command == "vitte-process.doctor") {
        g_out = cmd_process_doctor(p);
    } else if (command == "vitte-process.quickfix-config.preview") {
        g_out = cmd_process_quickfix_config(p, false);
    } else if (command == "vitte-process.quickfix-config.apply") {
        g_out = cmd_process_quickfix_config(p, true);
    } else if (command == "vitte-json.doctor") {
        g_out = cmd_json_doctor(p);
    } else if (command == "vitte-json.quickfix-config.preview") {
        g_out = cmd_json_quickfix_config(p, false);
    } else if (command == "vitte-json.quickfix-config.apply") {
        g_out = cmd_json_quickfix_config(p, true);
    } else if (command == "vitte-yaml.doctor") {
        g_out = cmd_yaml_doctor(p);
    } else if (command == "vitte-yaml.quickfix-config.preview") {
        g_out = cmd_yaml_quickfix_config(p, false);
    } else if (command == "vitte-yaml.quickfix-config.apply") {
        g_out = cmd_yaml_quickfix_config(p, true);
    } else if (command == "vitte-test.doctor") {
        g_out = cmd_test_doctor(p);
    } else if (command == "vitte-lint.doctor") {
        g_out = cmd_lint_doctor(p);
    } else if (command == "vitte-lint.quickfix-config.preview") {
        g_out = cmd_lint_quickfix_config(p, false);
    } else if (command == "vitte-lint.quickfix-config.apply") {
        g_out = cmd_lint_quickfix_config(p, true);
    } else if (command == "vitte-commands.by-domain") {
        g_out = cmd_commands_by_domain(p);
    } else {
        g_out = "plugin: unknown command " + command;
    }
    return g_out.c_str();
}

const char* provide_completion(const char* file, int line, int col, const char*) {
    std::unordered_map<std::string, std::string> p;
    if (file != nullptr) {
        p["file"] = file;
    }
    p["line"] = std::to_string(line);
    p["col"] = std::to_string(col);
    g_out = cmd_completion(p);
    return g_out.c_str();
}

VitteIdePluginV1 kPlugin = {
    1,
    "vitte-analyzer-pack",
    commands_csv,
    run_command,
    provide_completion,
};

}  // namespace

extern "C" const VitteIdePluginV1* vitte_ide_plugin_v1() {
    return &kPlugin;
}
