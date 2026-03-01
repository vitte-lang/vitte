#include <ncurses.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr int kFilePaneWidth = 34;
constexpr int kBottomPaneHeight = 10;
constexpr int kMaxDiagLines = 800;
constexpr int kMaxSuggestions = 32;
constexpr int kAutosaveMs = 5000;

const std::array<std::string_view, 38> kVitteKeywords = {
    "use", "space", "pick", "form", "proc", "entry", "if", "else", "loop", "break",
    "continue", "return", "give", "let", "set", "match", "when", "case", "true", "false",
    "unsafe", "macro", "trait", "emit", "pull", "share", "as", "in", "and", "or", "not",
    "where", "with", "pub", "impl", "yield", "await", "at"};

enum class FocusPane {
    Files,
    Code,
    Diagnostics,
};

struct CommandResult {
    int exit_code = -1;
    std::vector<std::string> lines;
};

struct SymbolDef {
    std::string name;
    std::string kind;
    std::size_t line = 0;
};

struct DiagHit {
    fs::path file;
    std::size_t line = 0;
    std::size_t col = 0;
    std::string message;
};

struct SearchHit {
    fs::path file;
    std::size_t line = 0;
    std::size_t col = 0;
    std::string text;
};

struct JumpLoc {
    fs::path file;
    std::size_t line = 0;
    std::size_t col = 0;
};

struct BuildTarget {
    std::string name = "default";
    std::string check_cmd = "vitte check {file}";
    std::string build_cmd = "vitte build {file} -o {out}";
    std::string run_cmd = "{out}";
    std::string test_cmd = "";
    std::string profile = "debug";
    std::string args;
    std::string env_csv;
};

struct Buffer {
    fs::path path;
    fs::path rel;
    std::vector<std::string> lines;
    std::size_t cursor_line = 0;
    std::size_t cursor_col = 0;
    std::size_t top_line = 0;
    bool dirty = false;
    std::optional<fs::file_time_type> mtime;
    std::vector<std::size_t> breakpoints;
    std::vector<SymbolDef> outline;
    std::set<std::string> imports;
    std::uint64_t access_tick = 0;
};

bool has_prefix(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

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

std::string to_lower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string shell_quote(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        out.push_back(line);
    }
    if (out.empty()) {
        out.push_back({});
    }
    return out;
}

CommandResult run_capture(const std::string& command) {
    CommandResult res;
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (pipe == nullptr) {
        res.lines.push_back("[error] unable to run command");
        return res;
    }

    std::string data;
    std::array<char, 4096> buf{};
    while (true) {
        const std::size_t n = fread(buf.data(), 1, buf.size(), pipe);
        if (n == 0U) {
            break;
        }
        data.append(buf.data(), n);
    }

    const int st = pclose(pipe);
    res.exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
    res.lines = split_lines(data);
    return res;
}

std::optional<fs::file_time_type> safe_mtime(const fs::path& p) {
    std::error_code ec;
    auto t = fs::last_write_time(p, ec);
    if (ec) {
        return std::nullopt;
    }
    return t;
}

bool is_source_file(const fs::path& p) {
    const auto ext = p.extension().string();
    return ext == ".vit" || ext == ".cpp" || ext == ".hpp" || ext == ".h";
}

bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '/';
}

std::vector<std::string> parse_shell_words(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    bool in_single = false;
    bool in_double = false;
    bool escaped = false;

    for (char ch : text) {
        if (escaped) {
            cur.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\' && !in_single) {
            escaped = true;
            continue;
        }
        if (ch == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (ch == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (!in_single && !in_double && std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
            continue;
        }
        cur.push_back(ch);
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

std::vector<std::pair<std::string, std::string>> parse_env_pairs(const std::string& csv) {
    std::vector<std::pair<std::string, std::string>> out;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim_copy(item);
        if (item.empty()) {
            continue;
        }
        const auto eq = item.find('=');
        if (eq == std::string::npos || eq == 0) {
            continue;
        }
        const std::string key = trim_copy(item.substr(0, eq));
        const std::string value = trim_copy(item.substr(eq + 1));
        if (!key.empty()) {
            out.emplace_back(key, value);
        }
    }
    return out;
}

std::string basename_of(const fs::path& p) {
    return p.filename().string();
}

std::string replace_all_copy(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return s;
    }
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::string regex_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        switch (c) {
            case '.': case '^': case '$': case '|': case '(': case ')':
            case '[': case ']': case '{': case '}': case '*': case '+':
            case '?': case '\\':
                out.push_back('\\');
                out.push_back(c);
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

}  // namespace

class VitteIdeApp {
  public:
    explicit VitteIdeApp(fs::path root) : project_root_(fs::absolute(std::move(root))) {}

    int run() {
        load_targets();
        refresh_project_files();
        load_session();
        if (buffers_.empty() && !files_.empty()) {
            selected_file_index_ = 0;
            open_selected_file();
        }
        status_ = "Ready. Ctrl+Tab tabs | Tab pane | i insert | / find | % replace | F12 def";

        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        timeout(120);
        curs_set(0);

        auto last_tick = std::chrono::steady_clock::now();
        while (!quit_) {
            draw();
            const int ch = getch();
            if (ch != ERR) {
                handle_key(ch);
            }
            const auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count() >= 300) {
                last_tick = now;
                periodic_tasks();
            }
        }

        save_session();
        endwin();
        return 0;
    }

  private:
    enum class ProblemsFilter {
        All,
        CurrentFile,
        ErrorsOnly,
    };

    fs::path project_root_;
    std::vector<fs::path> files_;
    std::size_t selected_file_index_ = 0;

    std::vector<Buffer> buffers_;
    std::size_t active_buffer_ = 0;

    bool split_enabled_ = false;
    bool split_vertical_ = true;
    std::size_t secondary_buffer_ = 0;
    int active_editor_pane_ = 0;  // 0 left/main, 1 right/second

    FocusPane focus_ = FocusPane::Files;
    bool quit_ = false;
    bool insert_mode_ = false;
    bool auto_check_ = true;
    std::string status_;

    std::vector<std::string> diag_lines_;
    std::vector<DiagHit> diag_hits_;
    std::size_t selected_diag_ = 0;
    std::vector<DiagHit> all_problems_;
    ProblemsFilter problems_filter_ = ProblemsFilter::All;

    std::vector<SearchHit> search_hits_;
    std::vector<SymbolDef> outline_view_;

    std::set<std::string> project_symbols_;
    std::map<std::string, std::vector<DiagHit>> global_symbol_defs_;
    std::uint64_t access_tick_ = 0;

    std::vector<JumpLoc> jump_history_;
    std::size_t jump_pos_ = 0;

    BuildTarget target_;
    fs::path last_build_binary_;
    bool diag_is_problems_ = false;

    std::chrono::steady_clock::time_point last_autosave_ = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_autocheck_ = std::chrono::steady_clock::now();

    Buffer* active_buffer_ptr() {
        if (buffers_.empty() || active_buffer_ >= buffers_.size()) {
            return nullptr;
        }
        return &buffers_[active_buffer_];
    }

    Buffer* pane_buffer_ptr(int pane) {
        if (buffers_.empty()) {
            return nullptr;
        }
        if (pane == 1 && split_enabled_) {
            if (secondary_buffer_ < buffers_.size()) {
                return &buffers_[secondary_buffer_];
            }
        }
        if (active_buffer_ < buffers_.size()) {
            return &buffers_[active_buffer_];
        }
        return nullptr;
    }

    const Buffer* active_buffer_ptr() const {
        if (buffers_.empty() || active_buffer_ >= buffers_.size()) {
            return nullptr;
        }
        return &buffers_[active_buffer_];
    }

    void refresh_project_files() {
        files_.clear();
        std::error_code ec;
        for (fs::recursive_directory_iterator it(project_root_, ec), end; it != end && !ec; it.increment(ec)) {
            const fs::path p = it->path();
            if (it->is_directory(ec)) {
                const std::string n = p.filename().string();
                if (n == ".git" || n == "build" || n == "target" || n == ".vitte-cache" || n == ".debstage") {
                    it.disable_recursion_pending();
                }
                continue;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }
            if (!is_source_file(p)) {
                continue;
            }
            files_.push_back(fs::relative(p, project_root_, ec));
        }
        std::sort(files_.begin(), files_.end());
        if (selected_file_index_ >= files_.size()) {
            selected_file_index_ = files_.empty() ? 0 : files_.size() - 1;
        }
        rebuild_global_indices();
    }

    static void parse_outline(Buffer& b) {
        b.outline.clear();
        b.imports.clear();

        static const std::regex outline_re(R"(^\s*(proc|form|entry|trait)\s+([A-Za-z_][A-Za-z0-9_]*)?)");
        static const std::regex use_re(R"(^\s*use\s+([A-Za-z_][A-Za-z0-9_/]*))");

        for (std::size_t i = 0; i < b.lines.size(); ++i) {
            const std::string& line = b.lines[i];
            std::smatch m;
            if (std::regex_search(line, m, outline_re) && m.size() >= 2) {
                SymbolDef sd;
                sd.kind = m[1].str();
                sd.name = (m.size() >= 3 && !m[2].str().empty()) ? m[2].str() : "<anon>";
                sd.line = i;
                b.outline.push_back(sd);
            }
            if (std::regex_search(line, m, use_re) && m.size() >= 2) {
                b.imports.insert(m[1].str());
            }
        }
    }

    void rebuild_global_indices() {
        project_symbols_.clear();
        global_symbol_defs_.clear();

        static const std::regex token_re(R"([A-Za-z_][A-Za-z0-9_/]*)");
        static const std::regex def_re(R"(^\s*(proc|form|entry|trait)\s+([A-Za-z_][A-Za-z0-9_]*)?)");

        for (const auto& rel : files_) {
            std::ifstream in(project_root_ / rel);
            if (!in.is_open()) {
                continue;
            }
            std::string line;
            std::size_t ln = 0;
            while (std::getline(in, line)) {
                for (std::sregex_iterator it(line.begin(), line.end(), token_re), end; it != end; ++it) {
                    project_symbols_.insert(it->str());
                }
                std::smatch m;
                if (std::regex_search(line, m, def_re) && m.size() >= 3) {
                    const std::string name = m[2].str();
                    if (!name.empty()) {
                        global_symbol_defs_[name].push_back({rel, ln, 0, m[1].str()});
                    }
                }
                ++ln;
            }
        }
    }

    std::optional<std::size_t> find_buffer_index(const fs::path& abs) const {
        std::error_code ec;
        const auto target = fs::weakly_canonical(abs, ec);
        for (std::size_t i = 0; i < buffers_.size(); ++i) {
            const auto p = fs::weakly_canonical(buffers_[i].path, ec);
            if (p == target) {
                return i;
            }
        }
        return std::nullopt;
    }

    bool load_buffer_contents(Buffer& b) {
        b.lines.clear();
        std::ifstream in(b.path);
        if (!in.is_open()) {
            b.lines.push_back("[unable to open file]");
            return false;
        }
        std::string line;
        while (std::getline(in, line)) {
            b.lines.push_back(line);
        }
        if (b.lines.empty()) {
            b.lines.push_back({});
        }
        b.mtime = safe_mtime(b.path);
        parse_outline(b);
        return true;
    }

    std::size_t open_buffer(const fs::path& rel, bool select = true, bool in_secondary = false) {
        const fs::path abs = project_root_ / rel;
        if (auto existing = find_buffer_index(abs); existing.has_value()) {
            if (select) {
                active_buffer_ = *existing;
                buffers_[active_buffer_].access_tick = ++access_tick_;
            }
            if (in_secondary) {
                secondary_buffer_ = *existing;
            }
            return *existing;
        }

        Buffer b;
        b.path = fs::absolute(abs);
        b.rel = rel;
        b.access_tick = ++access_tick_;
        load_buffer_contents(b);
        buffers_.push_back(std::move(b));
        const std::size_t idx = buffers_.size() - 1;
        if (select) {
            active_buffer_ = idx;
        }
        if (in_secondary) {
            secondary_buffer_ = idx;
        }
        return idx;
    }

    void open_selected_file() {
        if (files_.empty() || selected_file_index_ >= files_.size()) {
            return;
        }
        open_buffer(files_[selected_file_index_], true, false);
        run_check_current(false);
    }

    bool save_buffer(Buffer& b) {
        std::ofstream out(b.path);
        if (!out.is_open()) {
            status_ = "Save failed: " + b.path.string();
            return false;
        }
        for (std::size_t i = 0; i < b.lines.size(); ++i) {
            out << b.lines[i];
            if (i + 1 < b.lines.size()) {
                out << "\n";
            }
        }
        out << "\n";
        out.flush();
        if (!out.good()) {
            status_ = "Save failed: write error";
            return false;
        }
        b.dirty = false;
        b.mtime = safe_mtime(b.path);
        parse_outline(b);
        rebuild_global_indices();
        status_ = "Saved: " + b.rel.string();
        return true;
    }

    void push_jump_history(const Buffer& b) {
        JumpLoc j{b.rel, b.cursor_line, b.cursor_col};
        if (!jump_history_.empty() && jump_pos_ < jump_history_.size()) {
            const JumpLoc& cur = jump_history_[jump_pos_];
            if (cur.file == j.file && cur.line == j.line && cur.col == j.col) {
                return;
            }
        }
        if (!jump_history_.empty() && jump_pos_ + 1 < jump_history_.size()) {
            jump_history_.erase(jump_history_.begin() + static_cast<long>(jump_pos_ + 1), jump_history_.end());
        }
        jump_history_.push_back(j);
        jump_pos_ = jump_history_.size() - 1;
    }

    void goto_location(const JumpLoc& j, bool in_secondary = false) {
        const std::size_t idx = open_buffer(j.file, !in_secondary, in_secondary);
        Buffer& b = buffers_[idx];
        b.cursor_line = std::min(j.line, b.lines.empty() ? 0UL : b.lines.size() - 1);
        b.cursor_col = j.col;
        if (b.cursor_line < b.top_line) {
            b.top_line = b.cursor_line;
        }
        if (!in_secondary) {
            active_buffer_ = idx;
            focus_ = FocusPane::Code;
        }
    }

    std::string apply_target_template(const std::string& tpl, const Buffer& b) const {
        const fs::path out = project_root_ / ".vitte-cache" / "vitte-ide" / (b.path.stem().string() + "_ide.out");
        std::string cmd = tpl;
        cmd = replace_all_copy(cmd, "{file}", shell_quote(b.path.string()));
        cmd = replace_all_copy(cmd, "{rel}", shell_quote(b.rel.string()));
        cmd = replace_all_copy(cmd, "{out}", shell_quote(out.string()));
        cmd = replace_all_copy(cmd, "{profile}", target_.profile);
        return cmd;
    }

    void replace_diag_lines(std::vector<std::string> lines) {
        if (lines.empty()) {
            lines.push_back("(no output)");
        }
        if (lines.size() > kMaxDiagLines) {
            lines.erase(lines.begin(), lines.begin() + static_cast<long>(lines.size() - kMaxDiagLines));
        }
        diag_lines_ = std::move(lines);
    }

    void parse_diag_hits_from_lines() {
        diag_hits_.clear();
        static const std::regex re(R"(([^:\n]+):(\d+):(\d+):\s*(.*))");
        for (const auto& line : diag_lines_) {
            std::smatch m;
            if (!std::regex_search(line, m, re) || m.size() < 5) {
                continue;
            }
            const fs::path f = m[1].str();
            const std::size_t l = static_cast<std::size_t>(std::stoul(m[2].str()));
            const std::size_t c = static_cast<std::size_t>(std::stoul(m[3].str()));
            const std::string msg = trim_copy(m[4].str());
            fs::path rel = f;
            std::error_code ec;
            if (rel.is_absolute()) {
                rel = fs::relative(rel, project_root_, ec);
            }
            diag_hits_.push_back({rel, l > 0 ? l - 1 : 0, c > 0 ? c - 1 : 0, msg});
        }
        if (selected_diag_ >= diag_hits_.size()) {
            selected_diag_ = 0;
        }
        if (diag_is_problems_) {
            all_problems_ = diag_hits_;
        }
    }

    bool problem_match_filter(const DiagHit& d) const {
        if (problems_filter_ == ProblemsFilter::All) {
            return true;
        }
        if (problems_filter_ == ProblemsFilter::CurrentFile) {
            const Buffer* b = active_buffer_ptr();
            return b != nullptr && d.file == b->rel;
        }
        const std::string msg = to_lower(d.message);
        return msg.find("error") != std::string::npos || msg.find("fatal") != std::string::npos;
    }

    std::string problems_filter_label() const {
        switch (problems_filter_) {
            case ProblemsFilter::All: return "all";
            case ProblemsFilter::CurrentFile: return "current-file";
            case ProblemsFilter::ErrorsOnly: return "errors-only";
        }
        return "all";
    }

    void show_problems_filtered() {
        diag_is_problems_ = false;
        diag_hits_.clear();
        std::vector<std::string> out;
        out.push_back("[problems filter=" + problems_filter_label() + "]");
        for (const auto& p : all_problems_) {
            if (!problem_match_filter(p)) {
                continue;
            }
            out.push_back(" - " + p.file.string() + ":" + std::to_string(p.line + 1) + ":" +
                          std::to_string(p.col + 1) + " " + p.message);
            diag_hits_.push_back(p);
            if (diag_hits_.size() >= 400) {
                out.push_back("... truncated ...");
                break;
            }
        }
        if (diag_hits_.empty()) {
            out.push_back("(no problem for current filter)");
        }
        replace_diag_lines(std::move(out));
        selected_diag_ = 0;
        status_ = "Problems panel updated";
    }

    void run_target_command(const std::string& kind) {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }
        if (b->dirty && !save_buffer(*b)) {
            return;
        }

        std::string tpl;
        if (kind == "check") {
            tpl = target_.check_cmd;
        } else if (kind == "build") {
            tpl = target_.build_cmd;
        } else if (kind == "run") {
            tpl = target_.run_cmd;
        } else {
            tpl = target_.test_cmd;
        }
        if (tpl.empty()) {
            status_ = "Target command is empty: " + kind;
            return;
        }

        std::string cmd = apply_target_template(tpl, *b);
        if (kind == "run" || kind == "test") {
            if (!target_.args.empty()) {
                const auto args = parse_shell_words(target_.args);
                for (const auto& a : args) {
                    cmd += " " + shell_quote(a);
                }
            }
            if (!target_.env_csv.empty()) {
                const auto envs = parse_env_pairs(target_.env_csv);
                if (!envs.empty()) {
                    std::string prefix = "env";
                    for (const auto& kv : envs) {
                        prefix += " " + shell_quote(kv.first + "=" + kv.second);
                    }
                    cmd = prefix + " " + cmd;
                }
            }
        }

        if (kind == "run" || kind == "test") {
            suspend_for_interactive(cmd, kind == "run" ? "Run" : "Test");
            status_ = kind + " finished";
            return;
        }

        diag_is_problems_ = true;
        const std::string full = "cd " + shell_quote(project_root_.string()) + " && " + cmd;
        const CommandResult res = run_capture(full);
        std::vector<std::string> out;
        out.reserve(res.lines.size() + 2);
        out.push_back("$ " + full);
        out.insert(out.end(), res.lines.begin(), res.lines.end());
        out.push_back("[exit=" + std::to_string(res.exit_code) + "]");
        replace_diag_lines(std::move(out));
        parse_diag_hits_from_lines();

        if (kind == "build" && res.exit_code == 0) {
            last_build_binary_ = project_root_ / ".vitte-cache" / "vitte-ide" / (b->path.stem().string() + "_ide.out");
        }

        status_ = (res.exit_code == 0) ? (kind + " OK") : (kind + " failed");
    }

    void run_check_current(bool auto_mode) {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }
        if (!auto_mode && b->dirty && !save_buffer(*b)) {
            return;
        }
        diag_is_problems_ = true;
        const std::string cmd = "cd " + shell_quote(project_root_.string()) + " && " + apply_target_template(target_.check_cmd, *b);
        const CommandResult res = run_capture(cmd);
        std::vector<std::string> out;
        out.push_back("$ " + cmd);
        out.insert(out.end(), res.lines.begin(), res.lines.end());
        out.push_back("[exit=" + std::to_string(res.exit_code) + "]");
        replace_diag_lines(std::move(out));
        parse_diag_hits_from_lines();
        status_ = (res.exit_code == 0) ? (auto_mode ? "Auto-check OK" : "Check OK") : (auto_mode ? "Auto-check issues" : "Check failed");
    }

    void run_project_diagnostics() {
        diag_is_problems_ = true;
        std::vector<DiagHit> all_hits;
        std::vector<std::string> lines;
        lines.push_back("[project diagnostics]");

        std::size_t checked = 0;
        for (const auto& rel : files_) {
            if (rel.extension() != ".vit") {
                continue;
            }
            const std::string cmd = "cd " + shell_quote(project_root_.string()) + " && vitte check " + shell_quote((project_root_ / rel).string());
            const CommandResult res = run_capture(cmd);
            if (res.exit_code != 0) {
                lines.push_back("# " + rel.string());
                for (const auto& l : res.lines) {
                    lines.push_back(l);
                }
            }
            ++checked;
            if (checked > 120) {
                lines.push_back("[limit reached: 120 files]");
                break;
            }
        }

        replace_diag_lines(std::move(lines));
        parse_diag_hits_from_lines();
        status_ = "Project diagnostics generated";
        (void)all_hits;
    }

    void suspend_for_interactive(const std::string& command, const std::string& title) {
        endwin();
        std::cout << "\n=== " << title << " ===\n";
        std::cout << "$ " << command << "\n\n";
        const int rc = std::system(command.c_str());
        std::cout << "\n[exit=" << rc << "]\nPress ENTER to return...";
        std::cout.flush();
        std::string dummy;
        std::getline(std::cin, dummy);

        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        timeout(120);
        curs_set(0);
    }

    std::optional<std::string> prompt_input(const std::string& title, const std::string& initial = "") {
        timeout(-1);
        echo();
        curs_set(1);
        attron(A_REVERSE);
        mvhline(LINES - 1, 0, ' ', COLS);
        std::string prompt = title;
        if (!initial.empty()) {
            prompt += " [" + initial + "]";
        }
        prompt += ": ";
        mvaddnstr(LINES - 1, 0, prompt.c_str(), COLS - 1);
        attroff(A_REVERSE);
        move(LINES - 1, std::min<int>(static_cast<int>(prompt.size()), COLS - 2));

        std::array<char, 4096> buf{};
        const int rc = getnstr(buf.data(), static_cast<int>(buf.size() - 1));

        noecho();
        curs_set(0);
        timeout(120);
        if (rc == ERR) {
            return std::nullopt;
        }
        std::string in = trim_copy(buf.data());
        if (in.empty()) {
            in = initial;
        }
        return in;
    }

    std::string token_under_cursor(const Buffer& b) const {
        if (b.cursor_line >= b.lines.size()) {
            return {};
        }
        const std::string& line = b.lines[b.cursor_line];
        if (line.empty()) {
            return {};
        }
        std::size_t col = std::min(b.cursor_col, line.size());
        std::size_t l = col;
        while (l > 0 && is_ident_char(line[l - 1])) {
            --l;
        }
        std::size_t r = col;
        while (r < line.size() && is_ident_char(line[r])) {
            ++r;
        }
        return line.substr(l, r - l);
    }

    void goto_definition(bool to_secondary = false) {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }

        const std::string tok = token_under_cursor(*b);
        if (tok.empty()) {
            status_ = "No symbol under cursor";
            return;
        }

        push_jump_history(*b);

        auto it = global_symbol_defs_.find(tok);
        if (it != global_symbol_defs_.end() && !it->second.empty()) {
            const DiagHit& d = it->second.front();
            goto_location({d.file, d.line, d.col}, to_secondary);
            status_ = "Definition: " + tok;
            return;
        }

        if (tok.find('/') != std::string::npos) {
            fs::path candidate = project_root_ / "src/vitte/packages" / tok / "mod.vit";
            if (fs::exists(candidate)) {
                std::error_code ec;
                goto_location({fs::relative(candidate, project_root_, ec), 0, 0}, to_secondary);
                status_ = "Module follow: " + tok;
                return;
            }
        }

        status_ = "Definition not found: " + tok;
    }

    void follow_symbol_other_split() {
        if (!split_enabled_) {
            split_enabled_ = true;
            split_vertical_ = true;
            secondary_buffer_ = active_buffer_;
        }
        goto_definition(true);
    }

    void history_back() {
        if (jump_history_.empty() || jump_pos_ == 0) {
            status_ = "No back history";
            return;
        }
        --jump_pos_;
        goto_location(jump_history_[jump_pos_], false);
        status_ = "History back";
    }

    void history_forward() {
        if (jump_history_.empty() || jump_pos_ + 1 >= jump_history_.size()) {
            status_ = "No forward history";
            return;
        }
        ++jump_pos_;
        goto_location(jump_history_[jump_pos_], false);
        status_ = "History forward";
    }

    std::vector<std::string> build_suggestions_for(const Buffer& b) const {
        std::map<std::string, int> score;
        const std::string prefix = token_under_cursor(b);
        const std::string lower_prefix = to_lower(prefix);

        auto add = [&](const std::string& token, int s) {
            if (!prefix.empty()) {
                const std::string lt = to_lower(token);
                if (!has_prefix(lt, lower_prefix)) {
                    return;
                }
            }
            auto it = score.find(token);
            if (it == score.end()) {
                score[token] = s;
            } else {
                it->second = std::max(it->second, s);
            }
        };

        for (std::string_view kw : kVitteKeywords) {
            add(std::string(kw), 300);
        }

        for (const auto& sym : project_symbols_) {
            int base = 180;
            if (sym.find('/') != std::string::npos) {
                base += 15;
            }
            for (const auto& imp : b.imports) {
                if (sym.find(imp) != std::string::npos) {
                    base += 40;
                    break;
                }
            }
            add(sym, base);
        }

        const fs::path rel_dir = b.rel.parent_path();
        for (const auto& bf : buffers_) {
            if (bf.rel.parent_path() == rel_dir) {
                for (const auto& s : bf.outline) {
                    add(s.name, 340);
                }
            }
        }

        std::vector<std::pair<std::string, int>> ranked(score.begin(), score.end());
        std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b2) {
            if (a.second != b2.second) {
                return a.second > b2.second;
            }
            return a.first < b2.first;
        });

        std::vector<std::string> out;
        out.push_back("[suggestions] prefix='" + prefix + "'");
        for (const auto& kv : ranked) {
            out.push_back(" - " + kv.first + " [" + std::to_string(kv.second) + "]");
            if (out.size() >= kMaxSuggestions + 1) {
                break;
            }
        }
        if (out.size() == 1) {
            out.push_back("(no suggestion)");
        }
        return out;
    }

    void show_suggestions() {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }
        diag_is_problems_ = false;
        replace_diag_lines(build_suggestions_for(*b));
        diag_hits_.clear();
        status_ = "Suggestions updated";
    }

    void show_outline_current() {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }
        diag_is_problems_ = false;
        outline_view_ = b->outline;
        diag_hits_.clear();

        std::vector<std::string> out;
        out.push_back("[outline] " + b->rel.string());
        for (const auto& s : outline_view_) {
            out.push_back(" - " + s.kind + " " + s.name + " :" + std::to_string(s.line + 1));
            diag_hits_.push_back({b->rel, s.line, 0, s.kind + " " + s.name});
        }
        if (outline_view_.empty()) {
            out.push_back("(no symbols)");
        }
        replace_diag_lines(std::move(out));
        selected_diag_ = 0;
        status_ = "Outline ready";
    }

    void search_in_files() {
        diag_is_problems_ = false;
        const auto q = prompt_input("Find in files (regex)");
        if (!q.has_value() || q->empty()) {
            return;
        }
        const std::string cmd = "cd " + shell_quote(project_root_.string()) +
                                " && rg --line-number --column --no-heading --color=never " + shell_quote(*q);
        const CommandResult res = run_capture(cmd);

        search_hits_.clear();
        diag_hits_.clear();
        std::vector<std::string> out;
        out.push_back("$ " + cmd);

        static const std::regex re(R"(([^:\n]+):(\d+):(\d+):(.*))");
        for (const auto& l : res.lines) {
            out.push_back(l);
            std::smatch m;
            if (std::regex_match(l, m, re) && m.size() >= 5) {
                const fs::path f = m[1].str();
                const std::size_t ln = static_cast<std::size_t>(std::stoul(m[2].str()));
                const std::size_t cl = static_cast<std::size_t>(std::stoul(m[3].str()));
                const std::string tx = trim_copy(m[4].str());
                search_hits_.push_back({f, ln > 0 ? ln - 1 : 0, cl > 0 ? cl - 1 : 0, tx});
                diag_hits_.push_back({f, ln > 0 ? ln - 1 : 0, cl > 0 ? cl - 1 : 0, tx});
            }
        }
        if (search_hits_.empty()) {
            out.push_back("(no matches)");
        }
        out.push_back("[matches=" + std::to_string(search_hits_.size()) + "]");
        replace_diag_lines(std::move(out));
        selected_diag_ = 0;
        status_ = "Search complete";
    }

    void replace_in_files() {
        diag_is_problems_ = false;
        const auto from = prompt_input("Replace: find text");
        if (!from.has_value() || from->empty()) {
            return;
        }
        const auto to = prompt_input("Replace: with");
        if (!to.has_value()) {
            return;
        }

        std::vector<std::string> preview;
        std::size_t files_hit = 0;
        std::size_t repl_count = 0;

        for (const auto& rel : files_) {
            std::ifstream in(project_root_ / rel);
            if (!in.is_open()) {
                continue;
            }
            std::vector<std::string> lines;
            std::string line;
            bool touched = false;
            while (std::getline(in, line)) {
                std::size_t pos = 0;
                std::size_t local_hits = 0;
                while ((pos = line.find(*from, pos)) != std::string::npos) {
                    ++local_hits;
                    pos += from->size();
                }
                if (local_hits > 0) {
                    touched = true;
                    repl_count += local_hits;
                }
                lines.push_back(line);
            }
            if (touched) {
                ++files_hit;
                preview.push_back(" - " + rel.string());
            }
        }

        std::vector<std::string> out;
        out.push_back("[replace preview]");
        out.push_back("find='" + *from + "' -> '" + *to + "'");
        out.push_back("files=" + std::to_string(files_hit) + " occurrences=" + std::to_string(repl_count));
        for (const auto& p : preview) {
            out.push_back(p);
            if (out.size() > 60) {
                out.push_back("... truncated ...");
                break;
            }
        }
        replace_diag_lines(out);

        const auto confirm = prompt_input("Apply replace? (yes/no)", "no");
        if (!confirm.has_value() || to_lower(*confirm) != "yes") {
            status_ = "Replace cancelled";
            return;
        }

        std::size_t changed_files = 0;
        for (const auto& rel : files_) {
            const fs::path p = project_root_ / rel;
            std::ifstream in(p);
            if (!in.is_open()) {
                continue;
            }
            std::vector<std::string> lines;
            std::string line;
            bool touched = false;
            while (std::getline(in, line)) {
                const std::string replaced = replace_all_copy(line, *from, *to);
                if (replaced != line) {
                    touched = true;
                }
                lines.push_back(replaced);
            }
            if (!touched) {
                continue;
            }
            std::ofstream out_file(p);
            if (!out_file.is_open()) {
                continue;
            }
            for (std::size_t i = 0; i < lines.size(); ++i) {
                out_file << lines[i];
                if (i + 1 < lines.size()) {
                    out_file << "\n";
                }
            }
            out_file << "\n";
            ++changed_files;
        }

        for (auto& b : buffers_) {
            load_buffer_contents(b);
            b.dirty = false;
        }
        rebuild_global_indices();
        status_ = "Replace done. changed files=" + std::to_string(changed_files);
    }

    void find_references() {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }
        std::string symbol = token_under_cursor(*b);
        if (symbol.empty()) {
            const auto maybe = prompt_input("Find references symbol");
            if (!maybe.has_value() || maybe->empty()) {
                return;
            }
            symbol = *maybe;
        }

        diag_is_problems_ = false;
        diag_hits_.clear();
        std::vector<std::string> out;
        out.push_back("[references] " + symbol);

        const std::regex re("\\\\b" + regex_escape(symbol) + "\\\\b");
        for (const auto& rel : files_) {
            std::ifstream in(project_root_ / rel);
            if (!in.is_open()) {
                continue;
            }
            std::string line;
            std::size_t ln = 0;
            while (std::getline(in, line)) {
                std::smatch m;
                auto begin = line.cbegin();
                while (std::regex_search(begin, line.cend(), m, re)) {
                    const std::size_t col = static_cast<std::size_t>(m.position()) + static_cast<std::size_t>(begin - line.cbegin());
                    const std::string msg = trim_copy(line);
                    out.push_back(" - " + rel.string() + ":" + std::to_string(ln + 1) + ":" + std::to_string(col + 1) + " " + msg);
                    diag_hits_.push_back({rel, ln, col, msg});
                    if (diag_hits_.size() >= 500) {
                        out.push_back("... truncated ...");
                        break;
                    }
                    begin = m.suffix().first;
                }
                if (diag_hits_.size() >= 500) {
                    break;
                }
                ++ln;
            }
            if (diag_hits_.size() >= 500) {
                break;
            }
        }

        if (diag_hits_.empty()) {
            out.push_back("(no references)");
        }
        replace_diag_lines(std::move(out));
        selected_diag_ = 0;
        focus_ = FocusPane::Diagnostics;
        status_ = "References listed";
    }

    void rename_symbol_project() {
        Buffer* b = active_buffer_ptr();
        if (b == nullptr) {
            return;
        }
        std::string old_name = token_under_cursor(*b);
        const auto old_input = prompt_input("Rename symbol: old", old_name);
        if (!old_input.has_value() || old_input->empty()) {
            return;
        }
        old_name = *old_input;
        const auto new_input = prompt_input("Rename symbol: new");
        if (!new_input.has_value() || new_input->empty()) {
            return;
        }
        const std::string new_name = *new_input;
        if (new_name == old_name) {
            status_ = "Rename cancelled: same symbol";
            return;
        }

        const std::regex re("\\\\b" + regex_escape(old_name) + "\\\\b");
        std::size_t files_hit = 0;
        std::size_t count = 0;
        std::vector<std::string> preview;

        for (const auto& rel : files_) {
            std::ifstream in(project_root_ / rel);
            if (!in.is_open()) {
                continue;
            }
            std::string line;
            bool touched = false;
            std::size_t local_count = 0;
            while (std::getline(in, line)) {
                for (std::sregex_iterator it(line.begin(), line.end(), re), end; it != end; ++it) {
                    ++local_count;
                }
            }
            if (local_count > 0) {
                touched = true;
                count += local_count;
            }
            if (touched) {
                ++files_hit;
                preview.push_back(" - " + rel.string() + " (" + std::to_string(local_count) + ")");
            }
        }

        std::vector<std::string> out;
        out.push_back("[rename preview]");
        out.push_back(old_name + " -> " + new_name);
        out.push_back("files=" + std::to_string(files_hit) + " references=" + std::to_string(count));
        for (const auto& row : preview) {
            out.push_back(row);
            if (out.size() > 70) {
                out.push_back("... truncated ...");
                break;
            }
        }
        replace_diag_lines(std::move(out));
        focus_ = FocusPane::Diagnostics;

        const auto confirm = prompt_input("Apply rename? (yes/no)", "no");
        if (!confirm.has_value() || to_lower(*confirm) != "yes") {
            status_ = "Rename cancelled";
            return;
        }

        std::size_t changed = 0;
        for (const auto& rel : files_) {
            const fs::path p = project_root_ / rel;
            std::ifstream in(p);
            if (!in.is_open()) {
                continue;
            }
            std::vector<std::string> lines;
            std::string line;
            bool touched = false;
            while (std::getline(in, line)) {
                const std::string replaced = std::regex_replace(line, re, new_name);
                if (replaced != line) {
                    touched = true;
                }
                lines.push_back(replaced);
            }
            if (!touched) {
                continue;
            }
            std::ofstream out_file(p);
            if (!out_file.is_open()) {
                continue;
            }
            for (std::size_t i = 0; i < lines.size(); ++i) {
                out_file << lines[i];
                if (i + 1 < lines.size()) {
                    out_file << "\n";
                }
            }
            out_file << "\n";
            ++changed;
        }

        for (auto& buf : buffers_) {
            load_buffer_contents(buf);
            buf.dirty = false;
        }
        rebuild_global_indices();
        status_ = "Rename applied. changed files=" + std::to_string(changed);
    }

    void open_quick_palette() {
        diag_is_problems_ = false;
        std::string query;
        std::vector<DiagHit> candidates;
        std::size_t selected = 0;
        int old_timeout = 120;
        timeout(-1);

        auto rebuild = [&]() {
            candidates.clear();
            std::vector<std::string> out;
            out.push_back("[quick palette] " + query);

            if (!query.empty() && query[0] == ':') {
                Buffer* b = active_buffer_ptr();
                if (b != nullptr) {
                    const std::string num = trim_copy(query.substr(1));
                    if (!num.empty()) {
                        try {
                            std::size_t line = static_cast<std::size_t>(std::max(1, std::stoi(num)));
                            candidates.push_back({b->rel, line - 1, 0, "line jump"});
                            out.push_back(" - :" + std::to_string(line) + " " + b->rel.string());
                        } catch (...) {
                            out.push_back("(invalid line number)");
                        }
                    }
                }
            } else if (!query.empty() && query[0] == '#') {
                const std::string needle = to_lower(trim_copy(query.substr(1)));
                for (const auto& p : all_problems_) {
                    if (!needle.empty() && to_lower(p.message).find(needle) == std::string::npos) {
                        continue;
                    }
                    candidates.push_back(p);
                    out.push_back(" - " + p.file.string() + ":" + std::to_string(p.line + 1) + " " + p.message);
                    if (candidates.size() >= 120) {
                        break;
                    }
                }
                if (candidates.empty()) {
                    out.push_back("(no diagnostic match)");
                }
            } else if (!query.empty() && query[0] == '@') {
                const std::string sym_query = to_lower(trim_copy(query.substr(1)));
                for (const auto& kv : global_symbol_defs_) {
                    if (!sym_query.empty() && to_lower(kv.first).find(sym_query) == std::string::npos) {
                        continue;
                    }
                    for (const auto& d : kv.second) {
                        candidates.push_back(d);
                        out.push_back(" - @" + kv.first + "  " + d.file.string() + ":" + std::to_string(d.line + 1));
                        if (candidates.size() >= 120) {
                            break;
                        }
                    }
                    if (candidates.size() >= 120) {
                        break;
                    }
                }
                if (candidates.empty()) {
                    out.push_back("(no symbol match)");
                }
            } else {
                const std::string file_query = to_lower(query);
                std::vector<std::pair<int, fs::path>> ranked;
                for (const auto& rel : files_) {
                    const std::string val = to_lower(rel.string());
                    int score = 0;
                    if (file_query.empty()) {
                        score = 1;
                    } else if (val.find(file_query) != std::string::npos) {
                        score = 1000 - static_cast<int>(val.find(file_query));
                    } else {
                        // subsequence fuzzy score
                        std::size_t qidx = 0;
                        int run = 0;
                        for (char c : val) {
                            if (qidx < file_query.size() && c == file_query[qidx]) {
                                ++qidx;
                                ++run;
                                score += 10 + run;
                            } else {
                                run = 0;
                            }
                        }
                        if (qidx != file_query.size()) {
                            score = 0;
                        }
                    }
                    if (score > 0) {
                        ranked.push_back({score, rel});
                    }
                }
                std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
                    if (a.first != b.first) return a.first > b.first;
                    return a.second.string() < b.second.string();
                });
                for (const auto& item : ranked) {
                    candidates.push_back({item.second, 0, 0, item.second.string()});
                    out.push_back(" - " + item.second.string());
                    if (candidates.size() >= 120) {
                        break;
                    }
                }
                if (candidates.empty()) {
                    out.push_back("(no file match)");
                }
            }

            if (selected >= candidates.size()) {
                selected = candidates.empty() ? 0 : candidates.size() - 1;
            }
            replace_diag_lines(std::move(out));
            diag_hits_ = candidates;
            selected_diag_ = selected;
            focus_ = FocusPane::Diagnostics;

            if (!candidates.empty()) {
                if (!split_enabled_) {
                    split_enabled_ = true;
                    split_vertical_ = true;
                    secondary_buffer_ = active_buffer_;
                }
                goto_location({candidates[selected].file, candidates[selected].line, candidates[selected].col}, true);
            }
        };

        rebuild();
        while (true) {
            draw();
            attron(A_REVERSE);
            mvhline(LINES - 1, 0, ' ', COLS);
            std::string footer = "Ctrl+P " + query + " (Enter=open, Esc=cancel, @symbol, :line, #diag)";
            mvaddnstr(LINES - 1, 0, footer.c_str(), COLS - 1);
            attroff(A_REVERSE);
            refresh();

            const int ch = getch();
            if (ch == 27) {
                status_ = "Quick palette cancelled";
                break;
            }
            if (ch == '\n' || ch == KEY_ENTER) {
                if (!candidates.empty()) {
                    goto_location({candidates[selected].file, candidates[selected].line, candidates[selected].col}, false);
                    status_ = "Quick palette open";
                }
                break;
            }
            if (ch == KEY_UP) {
                if (selected > 0) {
                    --selected;
                }
                rebuild();
                continue;
            }
            if (ch == KEY_DOWN) {
                if (selected + 1 < candidates.size()) {
                    ++selected;
                }
                rebuild();
                continue;
            }
            if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (!query.empty()) {
                    query.pop_back();
                    rebuild();
                }
                continue;
            }
            if (ch >= 32 && ch <= 126) {
                query.push_back(static_cast<char>(ch));
                rebuild();
                continue;
            }
        }
        timeout(old_timeout);
    }

    void toggle_breakpoint(Buffer& b) {
        auto it = std::find(b.breakpoints.begin(), b.breakpoints.end(), b.cursor_line);
        if (it == b.breakpoints.end()) {
            b.breakpoints.push_back(b.cursor_line);
            std::sort(b.breakpoints.begin(), b.breakpoints.end());
            status_ = "Breakpoint added line " + std::to_string(b.cursor_line + 1);
        } else {
            b.breakpoints.erase(it);
            status_ = "Breakpoint removed line " + std::to_string(b.cursor_line + 1);
        }
    }

    bool line_has_breakpoint(const Buffer& b, std::size_t line) const {
        return std::find(b.breakpoints.begin(), b.breakpoints.end(), line) != b.breakpoints.end();
    }

    void ensure_cursor_visible(Buffer& b, int code_height) {
        if (b.lines.empty()) {
            return;
        }
        b.cursor_line = std::min(b.cursor_line, b.lines.size() - 1);
        b.cursor_col = std::min(b.cursor_col, b.lines[b.cursor_line].size());
        if (b.cursor_line < b.top_line) {
            b.top_line = b.cursor_line;
        }
        const std::size_t vis = static_cast<std::size_t>(std::max(1, code_height - 3));
        if (b.cursor_line > b.top_line + vis) {
            b.top_line = b.cursor_line - vis;
        }
    }

    void handle_insert_key(Buffer& b, int ch, int code_height) {
        if (ch == 27) {
            insert_mode_ = false;
            status_ = "Insert mode OFF";
            return;
        }
        if (ch == 19) {
            save_buffer(b);
            return;
        }

        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (b.cursor_col > 0) {
                b.lines[b.cursor_line].erase(b.cursor_col - 1, 1);
                --b.cursor_col;
                b.dirty = true;
            } else if (b.cursor_line > 0) {
                const std::size_t prev_len = b.lines[b.cursor_line - 1].size();
                b.lines[b.cursor_line - 1] += b.lines[b.cursor_line];
                b.lines.erase(b.lines.begin() + static_cast<long>(b.cursor_line));
                --b.cursor_line;
                b.cursor_col = prev_len;
                b.dirty = true;
            }
            ensure_cursor_visible(b, code_height);
            return;
        }
        if (ch == KEY_DC) {
            if (b.cursor_col < b.lines[b.cursor_line].size()) {
                b.lines[b.cursor_line].erase(b.cursor_col, 1);
                b.dirty = true;
            } else if (b.cursor_line + 1 < b.lines.size()) {
                b.lines[b.cursor_line] += b.lines[b.cursor_line + 1];
                b.lines.erase(b.lines.begin() + static_cast<long>(b.cursor_line + 1));
                b.dirty = true;
            }
            ensure_cursor_visible(b, code_height);
            return;
        }
        if (ch == '\n' || ch == KEY_ENTER) {
            std::string right = b.lines[b.cursor_line].substr(b.cursor_col);
            b.lines[b.cursor_line].erase(b.cursor_col);
            b.lines.insert(b.lines.begin() + static_cast<long>(b.cursor_line + 1), right);
            ++b.cursor_line;
            b.cursor_col = 0;
            b.dirty = true;
            ensure_cursor_visible(b, code_height);
            return;
        }
        if (ch == '\t') {
            b.lines[b.cursor_line].insert(b.cursor_col, "  ");
            b.cursor_col += 2;
            b.dirty = true;
            ensure_cursor_visible(b, code_height);
            return;
        }
        if (ch >= 32 && ch <= 126) {
            b.lines[b.cursor_line].insert(b.cursor_col, 1, static_cast<char>(ch));
            ++b.cursor_col;
            b.dirty = true;
            ensure_cursor_visible(b, code_height);
        }
    }

    void next_tab() {
        if (buffers_.empty()) {
            return;
        }
        active_buffer_ = (active_buffer_ + 1) % buffers_.size();
        buffers_[active_buffer_].access_tick = ++access_tick_;
        status_ = "Tab next";
    }

    void prev_tab() {
        if (buffers_.empty()) {
            return;
        }
        if (active_buffer_ == 0) {
            active_buffer_ = buffers_.size() - 1;
        } else {
            --active_buffer_;
        }
        buffers_[active_buffer_].access_tick = ++access_tick_;
        status_ = "Tab prev";
    }

    void close_active_tab() {
        if (buffers_.empty()) {
            return;
        }
        if (buffers_[active_buffer_].dirty) {
            const auto ans = prompt_input("Buffer dirty, close anyway? yes/no", "no");
            if (!ans.has_value() || to_lower(*ans) != "yes") {
                return;
            }
        }
        const std::size_t removed = active_buffer_;
        buffers_.erase(buffers_.begin() + static_cast<long>(active_buffer_));
        if (buffers_.empty()) {
            active_buffer_ = 0;
            secondary_buffer_ = 0;
        } else {
            if (active_buffer_ >= buffers_.size()) {
                active_buffer_ = buffers_.size() - 1;
            }
            if (secondary_buffer_ == removed) {
                secondary_buffer_ = active_buffer_;
            } else if (secondary_buffer_ > removed) {
                --secondary_buffer_;
            }
        }
        status_ = "Tab closed";
    }

    bool handle_ctrl_tab_escape(int ch) {
        if (ch != 27) {
            return false;
        }
        int c1 = getch();
        if (c1 == ERR) {
            return false;
        }
        if (c1 != '[') {
            return false;
        }
        std::string seq;
        for (int i = 0; i < 10; ++i) {
            int ci = getch();
            if (ci == ERR) {
                break;
            }
            seq.push_back(static_cast<char>(ci));
            if (std::isalpha(static_cast<unsigned char>(ci)) != 0 || ci == '~') {
                break;
            }
        }
        // Common sequences for Ctrl+Tab / Ctrl+Shift+Tab in some terminals.
        if (seq == "1;5I" || seq == "27;5;9~") {
            next_tab();
            return true;
        }
        if (seq == "1;6I" || seq == "27;6;9~") {
            prev_tab();
            return true;
        }
        return false;
    }

    void jump_to_diag(std::size_t idx) {
        if (idx >= diag_hits_.size()) {
            return;
        }
        selected_diag_ = idx;
        goto_location({diag_hits_[idx].file, diag_hits_[idx].line, diag_hits_[idx].col}, false);
        status_ = "Jump: " + diag_hits_[idx].message;
    }

    void handle_files_key(int ch) {
        if (files_.empty()) {
            return;
        }
        if (ch == KEY_UP && selected_file_index_ > 0) {
            --selected_file_index_;
        } else if (ch == KEY_DOWN && selected_file_index_ + 1 < files_.size()) {
            ++selected_file_index_;
        } else if (ch == '\n' || ch == KEY_ENTER || ch == 'o') {
            open_selected_file();
            focus_ = FocusPane::Code;
        } else if (ch == 'S') {
            open_buffer(files_[selected_file_index_], true, true);
            split_enabled_ = true;
            status_ = "Opened in split";
        }
    }

    void handle_code_nav(Buffer& b, int ch, int code_height) {
        if (b.lines.empty()) {
            return;
        }
        const std::size_t max_line = b.lines.size() - 1;

        if (ch == KEY_UP && b.cursor_line > 0) {
            --b.cursor_line;
        } else if (ch == KEY_DOWN && b.cursor_line < max_line) {
            ++b.cursor_line;
        } else if (ch == KEY_LEFT && b.cursor_col > 0) {
            --b.cursor_col;
        } else if (ch == KEY_RIGHT) {
            b.cursor_col = std::min(b.cursor_col + 1, b.lines[b.cursor_line].size());
        } else if (ch == KEY_PPAGE) {
            const std::size_t step = static_cast<std::size_t>(std::max(1, code_height - 3));
            b.cursor_line = (b.cursor_line > step) ? (b.cursor_line - step) : 0;
        } else if (ch == KEY_NPAGE) {
            const std::size_t step = static_cast<std::size_t>(std::max(1, code_height - 3));
            b.cursor_line = std::min(max_line, b.cursor_line + step);
        }
        ensure_cursor_visible(b, code_height);
    }

    void handle_code_key(int ch, int code_height) {
        Buffer* b = pane_buffer_ptr(active_editor_pane_);
        if (b == nullptr) {
            return;
        }

        if (insert_mode_) {
            handle_insert_key(*b, ch, code_height);
            return;
        }

        if (ch == 'i') {
            insert_mode_ = true;
            status_ = "Insert mode ON";
            return;
        }
        if (ch == 'w') {
            save_buffer(*b);
            return;
        }
        if (ch == 'k') {
            toggle_breakpoint(*b);
            return;
        }
        if (ch == 'n' && !diag_hits_.empty()) {
            jump_to_diag((selected_diag_ + 1) % diag_hits_.size());
            return;
        }
        if (ch == 'p' && !diag_hits_.empty()) {
            selected_diag_ = (selected_diag_ == 0) ? (diag_hits_.size() - 1) : (selected_diag_ - 1);
            jump_to_diag(selected_diag_);
            return;
        }
        if (ch == 'f') {
            follow_symbol_other_split();
            return;
        }
        if (ch == KEY_F(12)) {
            goto_definition(false);
            return;
        }
        if (ch == 18) {  // Ctrl+R
            show_outline_current();
            return;
        }

        handle_code_nav(*b, ch, code_height);
    }

    void handle_diag_key(int ch) {
        if (diag_hits_.empty()) {
            return;
        }
        if (ch == KEY_UP && selected_diag_ > 0) {
            --selected_diag_;
        } else if (ch == KEY_DOWN && selected_diag_ + 1 < diag_hits_.size()) {
            ++selected_diag_;
        } else if (ch == '\n' || ch == KEY_ENTER || ch == 'o') {
            jump_to_diag(selected_diag_);
            focus_ = FocusPane::Code;
        }
    }

    void cycle_focus() {
        if (focus_ == FocusPane::Files) {
            focus_ = FocusPane::Code;
        } else if (focus_ == FocusPane::Code) {
            focus_ = FocusPane::Diagnostics;
        } else {
            focus_ = FocusPane::Files;
        }
    }

    void periodic_tasks() {
        const auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_autosave_).count() >= kAutosaveMs) {
            last_autosave_ = now;
            for (auto& b : buffers_) {
                if (b.dirty) {
                    save_buffer(b);
                }
            }
        }

        // Reload external file changes.
        for (auto& b : buffers_) {
            const auto mt = safe_mtime(b.path);
            if (!mt.has_value() || !b.mtime.has_value()) {
                continue;
            }
            if (*mt != *b.mtime) {
                if (b.dirty) {
                    status_ = "External change on dirty buffer: " + b.rel.string() + " (keep local)";
                    b.mtime = mt;
                } else {
                    const auto ans = prompt_input("Reload changed file " + b.rel.string() + " ? yes/no", "yes");
                    if (ans.has_value() && to_lower(*ans) == "yes") {
                        load_buffer_contents(b);
                        status_ = "Reloaded external change: " + b.rel.string();
                    } else {
                        b.mtime = mt;
                    }
                }
            }
        }

        if (auto_check_ && std::chrono::duration_cast<std::chrono::milliseconds>(now - last_autocheck_).count() >= 1200) {
            last_autocheck_ = now;
            Buffer* b = active_buffer_ptr();
            if (b != nullptr && !b->dirty) {
                const auto mt = safe_mtime(b->path);
                if (mt.has_value() && b->mtime.has_value() && *mt != *b->mtime) {
                    load_buffer_contents(*b);
                    run_check_current(true);
                }
            }
        }
    }

    void load_targets() {
        const fs::path cfg = project_root_ / ".vitte-cache" / "vitte-ide" / "targets.conf";
        std::error_code ec;
        fs::create_directories(cfg.parent_path(), ec);
        if (!fs::exists(cfg)) {
            std::ofstream out(cfg);
            out << "name=default\n";
            out << "check=vitte check {file}\n";
            out << "build=vitte build {file} -o {out}\n";
            out << "run={out}\n";
            out << "test=\n";
            out << "profile=debug\n";
            out << "args=\n";
            out << "env=\n";
            out.close();
        }

        std::ifstream in(cfg);
        if (!in.is_open()) {
            return;
        }
        std::string line;
        while (std::getline(in, line)) {
            const auto eq = line.find('=');
            if (eq == std::string::npos) {
                continue;
            }
            const std::string key = trim_copy(line.substr(0, eq));
            const std::string val = line.substr(eq + 1);
            if (key == "name") target_.name = val;
            else if (key == "check") target_.check_cmd = val;
            else if (key == "build") target_.build_cmd = val;
            else if (key == "run") target_.run_cmd = val;
            else if (key == "test") target_.test_cmd = val;
            else if (key == "profile") target_.profile = val;
            else if (key == "args") target_.args = val;
            else if (key == "env") target_.env_csv = val;
        }
    }

    void edit_target_config() {
        const fs::path cfg = project_root_ / ".vitte-cache" / "vitte-ide" / "targets.conf";
        const std::string editor = std::getenv("EDITOR") ? std::getenv("EDITOR") : "nano";
        suspend_for_interactive(editor + " " + shell_quote(cfg.string()), "Edit targets");
        load_targets();
        status_ = "Targets reloaded";
    }

    void save_session() {
        const fs::path sess = project_root_ / ".vitte-cache" / "vitte-ide" / "session.txt";
        std::error_code ec;
        fs::create_directories(sess.parent_path(), ec);
        std::ofstream out(sess);
        if (!out.is_open()) {
            return;
        }

        out << "active=" << active_buffer_ << "\n";
        out << "split_enabled=" << (split_enabled_ ? 1 : 0) << "\n";
        out << "split_vertical=" << (split_vertical_ ? 1 : 0) << "\n";
        out << "secondary=" << secondary_buffer_ << "\n";
        for (const auto& b : buffers_) {
            out << "buf=" << b.rel.string() << "|" << b.cursor_line << "|" << b.cursor_col << "|" << b.top_line << "|";
            for (std::size_t i = 0; i < b.breakpoints.size(); ++i) {
                if (i > 0) out << ",";
                out << b.breakpoints[i];
            }
            out << "\n";
        }
    }

    void load_session() {
        const fs::path sess = project_root_ / ".vitte-cache" / "vitte-ide" / "session.txt";
        std::ifstream in(sess);
        if (!in.is_open()) {
            return;
        }

        std::vector<std::tuple<fs::path, std::size_t, std::size_t, std::size_t, std::vector<std::size_t>>> entries;
        std::string line;
        while (std::getline(in, line)) {
            if (has_prefix(line, "active=")) {
                active_buffer_ = static_cast<std::size_t>(std::stoul(line.substr(7)));
                continue;
            }
            if (has_prefix(line, "split_enabled=")) {
                split_enabled_ = line.substr(14) == "1";
                continue;
            }
            if (has_prefix(line, "split_vertical=")) {
                split_vertical_ = line.substr(15) == "1";
                continue;
            }
            if (has_prefix(line, "secondary=")) {
                secondary_buffer_ = static_cast<std::size_t>(std::stoul(line.substr(10)));
                continue;
            }
            if (!has_prefix(line, "buf=")) {
                continue;
            }
            const std::string payload = line.substr(4);
            std::vector<std::string> parts;
            std::stringstream ss(payload);
            std::string part;
            while (std::getline(ss, part, '|')) {
                parts.push_back(part);
            }
            if (parts.size() < 5) {
                continue;
            }
            fs::path rel = parts[0];
            std::size_t cl = static_cast<std::size_t>(std::stoul(parts[1]));
            std::size_t cc = static_cast<std::size_t>(std::stoul(parts[2]));
            std::size_t tl = static_cast<std::size_t>(std::stoul(parts[3]));
            std::vector<std::size_t> bps;
            std::stringstream sb(parts[4]);
            std::string tok;
            while (std::getline(sb, tok, ',')) {
                tok = trim_copy(tok);
                if (!tok.empty()) {
                    bps.push_back(static_cast<std::size_t>(std::stoul(tok)));
                }
            }
            entries.emplace_back(rel, cl, cc, tl, bps);
        }

        for (const auto& e : entries) {
            const fs::path rel = std::get<0>(e);
            if (!fs::exists(project_root_ / rel)) {
                continue;
            }
            const std::size_t idx = open_buffer(rel, false, false);
            Buffer& b = buffers_[idx];
            b.cursor_line = std::min(std::get<1>(e), b.lines.empty() ? 0UL : b.lines.size() - 1);
            b.cursor_col = std::get<2>(e);
            b.top_line = std::get<3>(e);
            b.breakpoints = std::get<4>(e);
        }

        if (!buffers_.empty()) {
            if (active_buffer_ >= buffers_.size()) {
                active_buffer_ = buffers_.size() - 1;
            }
            if (secondary_buffer_ >= buffers_.size()) {
                secondary_buffer_ = active_buffer_;
            }
        }
    }

    void switch_split_layout() {
        split_enabled_ = !split_enabled_;
        if (split_enabled_ && buffers_.size() > 1 && secondary_buffer_ == active_buffer_) {
            secondary_buffer_ = (active_buffer_ + 1) % buffers_.size();
        }
        status_ = std::string("Split ") + (split_enabled_ ? "ON" : "OFF");
    }

    void toggle_split_orientation() {
        split_vertical_ = !split_vertical_;
        status_ = std::string("Split orientation: ") + (split_vertical_ ? "vertical" : "horizontal");
    }

    void cycle_active_editor_pane() {
        if (!split_enabled_) {
            active_editor_pane_ = 0;
            return;
        }
        active_editor_pane_ = (active_editor_pane_ == 0) ? 1 : 0;
        status_ = std::string("Active pane: ") + (active_editor_pane_ == 0 ? "left" : "right");
    }

    void handle_key(int ch) {
        if (handle_ctrl_tab_escape(ch)) {
            return;
        }

        const int code_h = std::max(4, LINES - kBottomPaneHeight - 2);

        if (ch == '\t' && !insert_mode_) {
            cycle_focus();
            return;
        }
        if (ch == KEY_BTAB && !insert_mode_) {
            prev_tab();
            return;
        }

        switch (ch) {
            case 'q':
                quit_ = true;
                return;
            case 16:  // Ctrl+P
                open_quick_palette();
                return;
            case ']':
                next_tab();
                return;
            case '[':
                prev_tab();
                return;
            case 'X':
                close_active_tab();
                return;
            case 'v':
                switch_split_layout();
                return;
            case 'h':
                toggle_split_orientation();
                return;
            case 't':
                cycle_active_editor_pane();
                return;
            case '/':
                search_in_files();
                return;
            case '%':
                replace_in_files();
                return;
            case 'P':
                if (problems_filter_ == ProblemsFilter::All) {
                    problems_filter_ = ProblemsFilter::CurrentFile;
                } else if (problems_filter_ == ProblemsFilter::CurrentFile) {
                    problems_filter_ = ProblemsFilter::ErrorsOnly;
                } else {
                    problems_filter_ = ProblemsFilter::All;
                }
                show_problems_filtered();
                return;
            case 'U':
                find_references();
                return;
            case 'N':
                rename_symbol_project();
                return;
            case 'g':
                goto_definition(false);
                return;
            case 'f':
                follow_symbol_other_split();
                return;
            case 'H':
                history_back();
                return;
            case 'L':
                history_forward();
                return;
            case 's':
                show_suggestions();
                return;
            case 'c':
                run_target_command("check");
                return;
            case 'b':
                run_target_command("build");
                return;
            case 'x':
                run_target_command("run");
                return;
            case 'u':
                run_target_command("test");
                return;
            case 'D':
                run_project_diagnostics();
                return;
            case 'T':
                edit_target_config();
                return;
            case 'a':
                auto_check_ = !auto_check_;
                status_ = std::string("Auto-check ") + (auto_check_ ? "ON" : "OFF");
                return;
            case KEY_F(5):
                run_target_command("run");
                return;
            case KEY_F(6):
                run_target_command("check");
                return;
            case KEY_F(7):
                run_target_command("build");
                return;
            case KEY_F(8):
                run_target_command("test");
                return;
            default:
                break;
        }

        if (focus_ == FocusPane::Files) {
            handle_files_key(ch);
        } else if (focus_ == FocusPane::Code) {
            handle_code_key(ch, code_h);
        } else {
            handle_diag_key(ch);
        }
    }

    void draw_tabs_bar(int row, int left, int width) {
        mvhline(row, left, ' ', width);
        std::string line = "tabs: ";
        for (std::size_t i = 0; i < buffers_.size(); ++i) {
            const Buffer& b = buffers_[i];
            std::string label = basename_of(b.rel);
            if (b.dirty) {
                label += "*";
            }
            if (i == active_buffer_) {
                label = "[" + label + "]";
            } else if (split_enabled_ && i == secondary_buffer_) {
                label = "{" + label + "}";
            }
            if (line.size() + label.size() + 1 > static_cast<std::size_t>(width)) {
                line += " ...";
                break;
            }
            line += label + " ";
        }
        mvaddnstr(row, left, line.c_str(), width - 1);
    }

    void draw_status_bar() {
        attron(A_REVERSE);
        mvhline(0, 0, ' ', COLS);
        std::string f = (focus_ == FocusPane::Files) ? "FILES" : (focus_ == FocusPane::Code ? "CODE" : "DIAG");
        std::string m = insert_mode_ ? "INSERT" : "NORMAL";
        std::string head = " vitte-ide | " + project_root_.string() + " | focus=" + f + " | mode=" + m +
                           " | target=" + target_.name + "(" + target_.profile + ")" +
                           " | split=" + (split_enabled_ ? (split_vertical_ ? std::string("V") : std::string("H")) : std::string("off")) +
                           " | problems=" + problems_filter_label();
        mvaddnstr(0, 0, head.c_str(), COLS - 1);
        attroff(A_REVERSE);
    }

    void draw_files_pane(int top, int height, int width) {
        mvaddnstr(top, 1, "Project Files", width - 2);
        const int visible = std::max(0, height - 2);
        int start = 0;
        if (static_cast<int>(selected_file_index_) >= visible) {
            start = static_cast<int>(selected_file_index_) - visible + 1;
        }
        for (int i = 0; i < visible; ++i) {
            const int idx = start + i;
            const int row = top + 1 + i;
            mvhline(row, 1, ' ', width - 2);
            if (idx < 0 || idx >= static_cast<int>(files_.size())) {
                continue;
            }
            const std::string label = files_[static_cast<std::size_t>(idx)].string();
            if (static_cast<std::size_t>(idx) == selected_file_index_) {
                attron((focus_ == FocusPane::Files) ? A_REVERSE : A_BOLD);
                mvaddnstr(row, 1, label.c_str(), width - 2);
                attroff(A_REVERSE | A_BOLD);
            } else {
                mvaddnstr(row, 1, label.c_str(), width - 2);
            }
        }
        mvvline(top, width, ACS_VLINE, height);
    }

    void draw_editor_buffer(const Buffer& b, int top, int left, int width, int height, bool is_active_pane) {
        std::string title = b.rel.string();
        std::string symbol_here;
        for (const auto& s : b.outline) {
            if (s.line <= b.cursor_line) {
                symbol_here = s.kind + " " + s.name;
            } else {
                break;
            }
        }
        if (!symbol_here.empty()) {
            title += " > " + symbol_here;
        }
        mvhline(top, left, ' ', width);
        if (is_active_pane && focus_ == FocusPane::Code) {
            attron(A_BOLD);
        }
        mvaddnstr(top, left + 1, title.c_str(), width - 2);
        if (is_active_pane && focus_ == FocusPane::Code) {
            attroff(A_BOLD);
        }

        const int visible = std::max(0, height - 1);
        const int content_w = std::max(8, width - 2);
        for (int i = 0; i < visible; ++i) {
            const int row = top + 1 + i;
            const std::size_t ln = b.top_line + static_cast<std::size_t>(i);
            mvhline(row, left, ' ', width);
            if (ln >= b.lines.size()) {
                continue;
            }

            bool has_diag = false;
            for (const auto& d : diag_hits_) {
                if (d.file == b.rel && d.line == ln) {
                    has_diag = true;
                    break;
                }
            }
            const bool has_bp = line_has_breakpoint(b, ln);
            char marker = ' ';
            if (has_bp && has_diag) marker = '*';
            else if (has_bp) marker = 'B';
            else if (has_diag) marker = '!';

            std::ostringstream oss;
            oss << marker;
            oss.width(5);
            oss << (ln + 1) << " " << b.lines[ln];
            const std::string txt = oss.str();

            if (ln == b.cursor_line && is_active_pane) {
                attron((focus_ == FocusPane::Code) ? A_REVERSE : A_BOLD);
                mvaddnstr(row, left, txt.c_str(), content_w);
                attroff(A_REVERSE | A_BOLD);
            } else {
                mvaddnstr(row, left, txt.c_str(), content_w);
            }
        }

        // Text minimap at right edge of pane.
        if (width >= 18 && !b.lines.empty()) {
            const int mini_col = left + width - 1;
            for (int i = 0; i < visible; ++i) {
                const std::size_t mapped = (static_cast<std::size_t>(i) * b.lines.size()) / static_cast<std::size_t>(std::max(1, visible));
                char c = '.';
                if (mapped == b.cursor_line) {
                    c = 'C';
                } else if (line_has_breakpoint(b, mapped)) {
                    c = 'B';
                } else {
                    for (const auto& d : diag_hits_) {
                        if (d.file == b.rel && d.line == mapped) {
                            c = '!';
                            break;
                        }
                    }
                }
                mvaddch(top + 1 + i, mini_col, c);
            }
        }
    }

    void draw_code_area(int top, int height) {
        const int left = kFilePaneWidth + 1;
        const int width = COLS - left;

        if (buffers_.empty()) {
            mvaddnstr(top + 1, left + 1, "Open a file from the left pane.", width - 2);
            return;
        }

        draw_tabs_bar(top, left, width);

        const int pane_top = top + 1;
        const int pane_height = height - 1;
        if (!split_enabled_) {
            const Buffer& b = buffers_[active_buffer_];
            draw_editor_buffer(b, pane_top, left, width, pane_height, true);
            return;
        }

        if (split_vertical_) {
            const int w1 = width / 2;
            const int w2 = width - w1;
            const Buffer& b1 = buffers_[active_buffer_];
            const Buffer& b2 = buffers_[secondary_buffer_ < buffers_.size() ? secondary_buffer_ : active_buffer_];
            draw_editor_buffer(b1, pane_top, left, w1, pane_height, active_editor_pane_ == 0);
            mvvline(pane_top, left + w1, ACS_VLINE, pane_height);
            draw_editor_buffer(b2, pane_top, left + w1 + 1, w2 - 1, pane_height, active_editor_pane_ == 1);
        } else {
            const int h1 = pane_height / 2;
            const int h2 = pane_height - h1;
            const Buffer& b1 = buffers_[active_buffer_];
            const Buffer& b2 = buffers_[secondary_buffer_ < buffers_.size() ? secondary_buffer_ : active_buffer_];
            draw_editor_buffer(b1, pane_top, left, width, h1, active_editor_pane_ == 0);
            mvhline(pane_top + h1, left, ACS_HLINE, width);
            draw_editor_buffer(b2, pane_top + h1 + 1, left, width, h2 - 1, active_editor_pane_ == 1);
        }
    }

    void draw_diag_pane(int top, int height) {
        mvhline(top, 0, ACS_HLINE, COLS);
        std::string head = "Diagnostics/Search/Outline (Enter=open)";
        mvaddnstr(top, 1, head.c_str(), COLS - 2);

        const int visible = std::max(0, height - 2);
        int start = 0;
        if (static_cast<int>(diag_lines_.size()) > visible) {
            start = static_cast<int>(diag_lines_.size()) - visible;
        }

        for (int i = 0; i < visible; ++i) {
            const int idx = start + i;
            const int row = top + 1 + i;
            mvhline(row, 1, ' ', COLS - 2);
            if (idx < 0 || idx >= static_cast<int>(diag_lines_.size())) {
                continue;
            }
            const bool hi = focus_ == FocusPane::Diagnostics && selected_diag_ < diag_lines_.size() &&
                            idx == static_cast<int>(selected_diag_ + (diag_lines_.size() - std::min<std::size_t>(diag_lines_.size(), static_cast<std::size_t>(visible))));
            if (hi) {
                attron(A_REVERSE);
                mvaddnstr(row, 1, diag_lines_[static_cast<std::size_t>(idx)].c_str(), COLS - 2);
                attroff(A_REVERSE);
            } else {
                mvaddnstr(row, 1, diag_lines_[static_cast<std::size_t>(idx)].c_str(), COLS - 2);
            }
        }
    }

    void draw_footer() {
        attron(A_REVERSE);
        mvhline(LINES - 1, 0, ' ', COLS);
        mvaddnstr(LINES - 1, 0, status_.c_str(), COLS - 1);
        attroff(A_REVERSE);
    }

    void draw() {
        erase();
        if (LINES < 20 || COLS < 100) {
            mvaddstr(0, 0, "Terminal too small. Need >= 100x20.");
            mvaddstr(1, 0, "Press q to quit.");
            refresh();
            return;
        }

        draw_status_bar();
        const int top = 1;
        const int code_height = std::max(6, LINES - kBottomPaneHeight - 2);
        const int bottom_top = top + code_height;

        draw_files_pane(top, code_height, kFilePaneWidth);
        draw_code_area(top, code_height);
        draw_diag_pane(bottom_top, LINES - bottom_top - 1);
        draw_footer();

        refresh();
    }
};

int main(int argc, char** argv) {
    fs::path root = fs::current_path();
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]) != "") {
        root = argv[1];
    }

    if (!fs::exists(root)) {
        std::cerr << "vitte-ide: project path not found: " << root << "\n";
        return 2;
    }

    VitteIdeApp app(root);
    return app.run();
}
