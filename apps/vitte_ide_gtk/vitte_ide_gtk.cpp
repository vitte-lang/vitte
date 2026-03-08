#include <gtk/gtk.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <dlfcn.h>

std::string lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
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

std::vector<std::string> split_csv_trimmed(const std::string& csv) {
    std::vector<std::string> out;
    std::stringstream ss(csv);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        tok = trim_copy(tok);
        if (!tok.empty()) {
            out.push_back(tok);
        }
    }
    return out;
}

std::string collapse_spaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool prev_space = false;
    for (char c : s) {
        const bool is_space = std::isspace(static_cast<unsigned char>(c)) != 0;
        if (is_space) {
            if (!prev_space) {
                out.push_back(' ');
            }
            prev_space = true;
            continue;
        }
        prev_space = false;
        out.push_back(c);
    }
    return trim_copy(out);
}

std::string normalize_suggestion_key(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : collapse_spaces(lower_copy(s))) {
        if (c == ' ') {
            continue;
        }
        if (c == '(' || c == ')' || c == ',' || c == ':' || c == ';' || c == '<' || c == '>' || c == '[' || c == ']') {
            continue;
        }
        out.push_back(c);
    }
    return out;
}

std::string infer_completion_type(const std::string& s) {
    if (s.find('(') != std::string::npos) {
        return "function";
    }
    if (s.find("::") != std::string::npos) {
        return "module";
    }
    return "symbol";
}

bool fuzzy_match(const std::string& needle_in, const std::string& hay_in) {
    const std::string needle = lower_copy(needle_in);
    const std::string hay = lower_copy(hay_in);
    if (needle.empty()) {
        return true;
    }

    std::size_t i = 0;
    for (char c : hay) {
        if (i < needle.size() && needle[i] == c) {
            ++i;
        }
    }
    return i == needle.size();
}

double fuzzy_prefix_score(const std::string& query, const std::string& candidate) {
    const std::string q = lower_copy(trim_copy(query));
    const std::string c = lower_copy(candidate);
    if (q.empty() || c.empty()) {
        return 0.0;
    }
    if (c.rfind(q, 0) == 0) {
        return 3.0;
    }
    if (c.find(q) != std::string::npos) {
        return 2.0;
    }
    return fuzzy_match(q, c) ? 1.0 : 0.0;
}

double percentile(std::vector<double> xs, double q) {
    if (xs.empty()) {
        return 0.0;
    }
    std::sort(xs.begin(), xs.end());
    const std::size_t i = static_cast<std::size_t>(std::round((xs.size() - 1) * q));
    return xs[std::min(i, xs.size() - 1)];
}

namespace fs = std::filesystem;

namespace {
constexpr std::size_t kDefaultMaxCompletionSuggestions = 300;
constexpr std::size_t kDefaultMaxPaletteSuggestions = 800;
constexpr std::size_t kDefaultCompletionPageSize = 100;
constexpr int kDefaultCompletionCacheTtlMs = 5000;
constexpr int kDefaultLspCompletionTimeoutMs = 220;

enum ProjectColumns {
    COL_NAME = 0,
    COL_PATH = 1,
    NUM_PROJECT_COLS = 2,
};

enum ProblemsColumns {
    PROB_FILE = 0,
    PROB_LINE = 1,
    PROB_COL = 2,
    PROB_SEV = 3,
    PROB_TEXT = 4,
    NUM_PROB_COLS = 5,
};

enum OutlineColumns {
    OUT_ICON = 0,
    OUT_KIND = 1,
    OUT_NAME = 2,
    OUT_LINE = 3,
    OUT_IS_GROUP = 4,
    NUM_OUT_COLS = 5,
};

enum PaletteColumns {
    PAL_LABEL = 0,
    PAL_FILE = 1,
    PAL_LINE = 2,
    PAL_COL = 3,
    NUM_PAL_COLS = 4,
};

enum DebugCallstackColumns {
    DBG_STACK_FRAME = 0,
    NUM_DBG_STACK_COLS = 1,
};

enum DebugVarColumns {
    DBG_VAR_NAME = 0,
    DBG_VAR_VALUE = 1,
    NUM_DBG_VAR_COLS = 2,
};

enum DebugBpColumns {
    DBG_BP_FILE = 0,
    DBG_BP_LINE = 1,
    NUM_DBG_BP_COLS = 2,
};

enum CompletionColumns {
    COMP_ICON = 0,
    COMP_LABEL = 1,
    COMP_TYPE = 2,
    COMP_SOURCE = 3,
    NUM_COMP_COLS = 4,
};

enum class ProblemFilter {
    All,
    ErrorsOnly,
    CurrentFile,
};

struct VitteIdePluginV1 {
    int abi_version;
    const char* plugin_name;
    const char* (*commands_csv)();
    const char* (*run_command)(const char* command, const char* payload);
    const char* (*provide_completion)(const char* file, int line, int col, const char* prefix);
};

struct SymbolDef {
    std::string kind;
    std::string name;
    std::string file;
    int line = 1;
    int col = 1;
};

struct ProblemItem {
    std::string file;
    int line = 1;
    int col = 1;
    std::string severity;
    std::string text;
};

struct CompletionSuggestion {
    std::string label;
    std::string norm_key;
    std::string source;
    std::string type;
    std::string file;
    double score = 0.0;
};

struct CompletionCacheEntry {
    std::chrono::steady_clock::time_point ts{};
    std::vector<CompletionSuggestion> items;
};

struct RenameHit {
    std::string file;
    int line = 1;
    std::string before;
    std::string after;
};

struct BuildPreset {
    std::string debug_extra;
    std::string release_extra;
    std::string run_args;
    std::string run_env;
};

struct EditorTab {
    std::string path;
    GtkWidget* scrolled = nullptr;
    GtkWidget* text_view = nullptr;
    GtkTextBuffer* buffer = nullptr;
    GtkWidget* tab_header = nullptr;
    GtkWidget* tab_label = nullptr;
    bool dirty = false;
    GtkWidget* owner_notebook = nullptr;
    std::time_t last_autosave_at = 0;
};

struct AppState;

struct PaletteState {
    AppState* app = nullptr;
    GtkWidget* dialog = nullptr;
    GtkWidget* entry = nullptr;
    GtkWidget* view = nullptr;
    GtkListStore* store = nullptr;
};

struct CompletionDialogState {
    AppState* app = nullptr;
    GtkWidget* dialog = nullptr;
    GtkWidget* view = nullptr;
    GtkListStore* store = nullptr;
    GtkWidget* info = nullptr;
    GtkWidget* load_more_btn = nullptr;
    std::vector<CompletionSuggestion> all;
    std::size_t shown = 0;
    std::string query;
};

struct AppState {
    GtkApplication* app = nullptr;
    GtkWidget* window = nullptr;

    GtkWidget* editor_split = nullptr;
    GtkWidget* notebook_primary = nullptr;
    GtkWidget* notebook_secondary = nullptr;
    GtkWidget* active_notebook = nullptr;
    bool split_enabled = false;
    GtkOrientation split_orientation = GTK_ORIENTATION_HORIZONTAL;

    GtkWidget* project_view = nullptr;
    GtkTreeStore* project_store = nullptr;

    GtkWidget* outline_view = nullptr;
    GtkTreeStore* outline_store = nullptr;
    std::vector<SymbolDef> current_symbols;
    GtkWidget* outline_filter_entry = nullptr;
    GtkWidget* outline_empty_label = nullptr;
    GtkWidget* left_tabs = nullptr;
    std::unordered_set<std::string> outline_collapsed_groups;
    GtkWidget* docs_view = nullptr;
    GtkTextBuffer* docs_buffer = nullptr;
    GtkWidget* tools_view = nullptr;
    GtkTextBuffer* tools_buffer = nullptr;

    GtkWidget* problems_view = nullptr;
    GtkListStore* problems_store = nullptr;
    GtkWidget* problems_filter = nullptr;
    ProblemFilter problem_filter = ProblemFilter::All;
    std::vector<ProblemItem> all_problems;
    GtkListStore* gutter_store = nullptr;
    GtkWidget* gutter_view = nullptr;
    GtkWidget* bottom_notebook = nullptr;
    GtkWidget* bottom_panel = nullptr;
    bool bottom_panel_visible = true;
    GtkWidget* terminal_output_view = nullptr;
    GtkTextBuffer* terminal_output_buffer = nullptr;
    GtkWidget* terminal_cmd_entry = nullptr;
    GtkListStore* debug_callstack_store = nullptr;
    GtkListStore* debug_vars_store = nullptr;
    GtkListStore* debug_watches_store = nullptr;
    GtkListStore* debug_breakpoints_store = nullptr;
    GtkWidget* debug_watches_entry = nullptr;

    GtkWidget* status_label = nullptr;

    std::string root;
    std::vector<std::string> workspace_roots;
    std::vector<EditorTab*> tabs_primary;
    std::vector<EditorTab*> tabs_secondary;

    bool autosave_enabled = true;
    int autosave_seconds = 30;
    guint autosave_timer = 0;
    std::unordered_map<std::string, int> autosave_seconds_by_file;

    std::string build_profile = "Debug";
    std::string build_extra_debug;
    std::string build_extra_release;
    std::string run_args;
    std::string run_env;
    std::unordered_map<std::string, BuildPreset> file_presets;

    std::string theme_mode = "System";
    std::string window_start_mode = "auto";
    std::string font_family = "Monospace";
    int font_size = 11;
    GtkCssProvider* css_provider = nullptr;

    bool lsp_bridge_enabled = false;
    std::string lsp_bridge_cmd = "vitte lsp";
    bool formatter_on_save = false;
    std::string formatter_cmd = "vitte fmt {file}";

    std::unordered_map<std::string, std::string> project_tasks;

    bool perf_mode_enabled = false;
    guint deferred_outline_timer = 0;
    std::atomic<bool> search_index_worker_running{false};
    std::mutex search_index_mu;
    std::unordered_map<std::string, std::unordered_set<std::string>> search_index;
    bool search_index_loaded = false;

    std::string keymap_mode = "default";
    bool high_contrast = false;
    bool compact_mode = false;
    int ui_zoom_percent = 100;
    int editor_tab_width = 4;

    bool telemetry_opt_in = false;
    std::uint64_t telemetry_errors = 0;
    std::uint64_t telemetry_commands = 0;
    std::string release_doctor_last_failing_report;

    std::vector<void*> plugin_handles;
    std::vector<std::string> plugin_names;
    std::unordered_map<std::string, std::function<std::string(const std::string&)>> plugin_commands;
    std::vector<std::function<std::vector<std::string>(const std::string&, int, int)>> completion_providers;
    std::unordered_map<std::string, int> completion_usage_freq;
    std::unordered_map<std::string, CompletionCacheEntry> completion_cache;
    std::vector<double> completion_latency_samples_ms;
    guint completion_prefetch_timer = 0;
    std::string completion_prefetch_key;
    int completion_max_suggestions = static_cast<int>(kDefaultMaxCompletionSuggestions);
    int palette_max_suggestions = static_cast<int>(kDefaultMaxPaletteSuggestions);
    int completion_page_size = static_cast<int>(kDefaultCompletionPageSize);
    int completion_cache_ttl_ms = kDefaultCompletionCacheTtlMs;
    int lsp_completion_timeout_ms = kDefaultLspCompletionTimeoutMs;

    long long config_last_mtime_ticks = 0;
};

void apply_appearance(AppState* s);
std::string read_file_text(const fs::path& p);
std::string current_symbol_under_cursor(EditorTab* tab);
void current_cursor_position(EditorTab* tab, int* line, int* col);
std::string completion_cache_key(const std::string& file, int line, const std::string& prefix);
std::vector<CompletionSuggestion> collect_completion_suggestions(AppState* s, EditorTab* tab, bool* lsp_timed_out);
bool run_plugin_command_by_name(AppState* s, const std::string& command, const std::string& payload, std::string* out);
std::string plugin_cursor_payload(EditorTab* tab);
void build_project_tree(AppState* s);
void load_project_tasks(AppState* s);
void show_tools_output(AppState* s, const std::string& title, const std::string& text);
void restore_recovery_snapshot(AppState* s);

void apply_window_start_mode(AppState* s) {
    if (s == nullptr || s->window == nullptr) {
        return;
    }
    if (s->window_start_mode == "maximized") {
        gtk_window_maximize(GTK_WINDOW(s->window));
    } else {
        gtk_window_unmaximize(GTK_WINDOW(s->window));
    }
}

bool is_source(const fs::path& p) {
    const std::string ext = p.extension().string();
    return ext == ".vit" || ext == ".cpp" || ext == ".hpp" || ext == ".h";
}

bool should_skip_dir_name(const std::string& base) {
    return base == ".git" || base == "build" || base == "target" || base == ".vitte-cache" || base == "node_modules" ||
           base == ".venv" || base == ".debstage" || base == ".vscode";
}

std::string primary_workspace_root(const AppState* s) {
    if (s != nullptr && !s->workspace_roots.empty()) {
        return s->workspace_roots.front();
    }
    return s != nullptr ? s->root : std::string();
}

std::string workspace_root_for_path(const AppState* s, const std::string& path) {
    if (s == nullptr) {
        return std::string();
    }
    const fs::path abs = fs::absolute(path);
    std::string best = primary_workspace_root(s);
    std::size_t best_len = 0;
    for (const std::string& r : s->workspace_roots) {
        const fs::path rr = fs::absolute(r);
        const std::string rs = rr.string();
        if (abs.string().rfind(rs, 0) == 0 && rs.size() >= best_len) {
            best = rs;
            best_len = rs.size();
        }
    }
    return best;
}

std::string to_project_relative(const AppState* s, const std::string& path) {
    fs::path p(path);
    if (p.is_absolute()) {
        std::error_code ec;
        fs::path rel = fs::relative(p, fs::path(workspace_root_for_path(s, p.string())), ec);
        if (!ec) {
            return rel.string();
        }
    }
    return path;
}

std::string normalize_path(const AppState* s, const std::string& file) {
    fs::path p(file);
    if (p.is_absolute()) {
        return p.string();
    }
    return (fs::path(primary_workspace_root(s)) / p).string();
}

std::vector<EditorTab*>& tabs_for_notebook(AppState* s, GtkWidget* notebook) {
    if (notebook == s->notebook_secondary) {
        return s->tabs_secondary;
    }
    return s->tabs_primary;
}

EditorTab* find_tab_by_buffer(AppState* s, GtkTextBuffer* buffer) {
    auto scan = [buffer](const std::vector<EditorTab*>& tabs) -> EditorTab* {
        for (EditorTab* t : tabs) {
            if (t != nullptr && t->buffer == buffer) {
                return t;
            }
        }
        return nullptr;
    };
    if (EditorTab* t = scan(s->tabs_primary); t != nullptr) {
        return t;
    }
    return scan(s->tabs_secondary);
}

EditorTab* find_open_tab(AppState* s, const std::string& abs_path) {
    auto scan = [&abs_path](const std::vector<EditorTab*>& tabs) -> EditorTab* {
        for (EditorTab* t : tabs) {
            if (t != nullptr && t->path == abs_path) {
                return t;
            }
        }
        return nullptr;
    };
    if (EditorTab* t = scan(s->tabs_primary); t != nullptr) {
        return t;
    }
    return scan(s->tabs_secondary);
}

void set_status(AppState* s, const std::string& msg) {
    if (s != nullptr && s->status_label != nullptr) {
        gtk_label_set_text(GTK_LABEL(s->status_label), msg.c_str());
    }
}

std::string search_index_path(const AppState* s) {
    fs::path p = fs::path(primary_workspace_root(s)) / ".vitte-cache" / "vitte-ide-gtk" / "search.index";
    fs::create_directories(p.parent_path());
    return p.string();
}

std::string config_path(const AppState* s) {
    fs::path p = fs::path(primary_workspace_root(s)) / ".vitte-cache" / "vitte-ide-gtk" / "config.conf";
    fs::create_directories(p.parent_path());
    return p.string();
}

std::string telemetry_path(const AppState* s) {
    fs::path p = fs::path(primary_workspace_root(s)) / ".vitte-cache" / "vitte-ide-gtk" / "telemetry.log";
    fs::create_directories(p.parent_path());
    return p.string();
}

void record_telemetry(AppState* s, const std::string& event, const std::string& detail) {
    if (s == nullptr || !s->telemetry_opt_in) {
        return;
    }
    std::ofstream out(telemetry_path(s), std::ios::app);
    if (!out.is_open()) {
        return;
    }
    out << std::time(nullptr) << "|" << event << "|" << detail << "\n";
}

void apply_editor_settings_to_tab(AppState* s, EditorTab* tab) {
    if (s == nullptr || tab == nullptr || tab->text_view == nullptr) {
        return;
    }
    PangoTabArray* tabs = pango_tab_array_new(1, TRUE);
    pango_tab_array_set_tab(tabs, 0, PANGO_TAB_LEFT, s->editor_tab_width * 8);
    gtk_text_view_set_tabs(GTK_TEXT_VIEW(tab->text_view), tabs);
    pango_tab_array_free(tabs);
}

int effective_editor_font_size(const AppState* s) {
    if (s == nullptr) {
        return 11;
    }
    const int zoom = std::max(70, std::min(200, s->ui_zoom_percent));
    const double scaled = static_cast<double>(std::max(8, s->font_size)) * static_cast<double>(zoom) / 100.0;
    return std::max(8, static_cast<int>(std::lround(scaled)));
}

void push_completion_latency_sample(AppState* s, double ms) {
    if (s == nullptr) {
        return;
    }
    s->completion_latency_samples_ms.push_back(ms);
    if (s->completion_latency_samples_ms.size() > 256) {
        s->completion_latency_samples_ms.erase(s->completion_latency_samples_ms.begin());
    }
}

std::string completion_cache_key(const std::string& file, int line, const std::string& prefix) {
    return file + "|" + std::to_string(line) + "|" + normalize_suggestion_key(prefix);
}

void unload_plugins(AppState* s) {
    if (s == nullptr) {
        return;
    }
    for (void* h : s->plugin_handles) {
        if (h != nullptr) {
            dlclose(h);
        }
    }
    s->plugin_handles.clear();
    s->plugin_names.clear();
    s->plugin_commands.clear();
    s->completion_providers.clear();
}

void load_plugins(AppState* s) {
    if (s == nullptr) {
        return;
    }
    unload_plugins(s);
    std::vector<fs::path> dirs = {fs::path(primary_workspace_root(s)) / "plugins", fs::path(primary_workspace_root(s)) / ".vitte-cache" / "vitte-ide-gtk" / "plugins"};
    for (const fs::path& d : dirs) {
        if (!fs::exists(d)) {
            continue;
        }
        for (const auto& e : fs::directory_iterator(d)) {
            if (!e.is_regular_file() || e.path().extension() != ".so") {
                continue;
            }
            void* h = dlopen(e.path().c_str(), RTLD_NOW);
            if (h == nullptr) {
                continue;
            }
            using GetPluginFn = const VitteIdePluginV1* (*)();
            auto* get_plugin = reinterpret_cast<GetPluginFn>(dlsym(h, "vitte_ide_plugin_v1"));
            if (get_plugin == nullptr) {
                dlclose(h);
                continue;
            }
            const VitteIdePluginV1* p = get_plugin();
            if (p == nullptr || p->abi_version != 1) {
                dlclose(h);
                continue;
            }
            s->plugin_handles.push_back(h);
            s->plugin_names.push_back(p->plugin_name != nullptr ? p->plugin_name : e.path().filename().string());

            if (p->commands_csv != nullptr && p->run_command != nullptr) {
                const std::string csv = p->commands_csv();
                for (const std::string& cmd : split_csv_trimmed(csv)) {
                    s->plugin_commands[cmd] = [p, cmd](const std::string& payload) -> std::string {
                        const char* out = p->run_command(cmd.c_str(), payload.c_str());
                        return out != nullptr ? out : "";
                    };
                }
            }
            if (p->provide_completion != nullptr) {
                s->completion_providers.push_back([p](const std::string& file, int line, int col) {
                    std::vector<std::string> out;
                    const char* r = p->provide_completion(file.c_str(), line, col, "");
                    if (r != nullptr) {
                        out = split_csv_trimmed(r);
                    }
                    return out;
                });
            }
        }
    }
}

void apply_runtime_config_file(AppState* s, bool announce) {
    if (s == nullptr) {
        return;
    }
    std::ifstream in(config_path(s));
    if (!in.is_open()) {
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trim_copy(line.substr(0, eq));
        const std::string val = trim_copy(line.substr(eq + 1));
        if (key == "theme_mode") {
            s->theme_mode = (val == "Dark" || val == "Light") ? val : "System";
        } else if (key == "window_start") {
            s->window_start_mode = (val == "maximized") ? "maximized" : "auto";
        } else if (key == "font_family") {
            s->font_family = val.empty() ? "Monospace" : val;
        } else if (key == "font_size") {
            s->font_size = std::max(8, std::atoi(val.c_str()));
        } else if (key == "high_contrast") {
            s->high_contrast = (val == "1" || lower_copy(val) == "true");
        } else if (key == "compact_mode") {
            s->compact_mode = (val == "1" || lower_copy(val) == "true");
        } else if (key == "ui_zoom_percent") {
            s->ui_zoom_percent = std::max(70, std::min(200, std::atoi(val.c_str())));
        } else if (key == "tab_width") {
            s->editor_tab_width = std::max(1, std::atoi(val.c_str()));
        } else if (key == "perf_mode") {
            s->perf_mode_enabled = (val == "1" || lower_copy(val) == "true");
        } else if (key == "keymap") {
            s->keymap_mode = (val == "vim" || val == "emacs") ? val : "default";
        } else if (key == "telemetry_opt_in") {
            s->telemetry_opt_in = (val == "1" || lower_copy(val) == "true");
        } else if (key == "completion.max_suggestions") {
            s->completion_max_suggestions = std::max(20, std::atoi(val.c_str()));
        } else if (key == "palette.max_suggestions") {
            s->palette_max_suggestions = std::max(50, std::atoi(val.c_str()));
        } else if (key == "completion.page_size") {
            s->completion_page_size = std::max(20, std::atoi(val.c_str()));
        } else if (key == "completion.cache_ttl_ms") {
            s->completion_cache_ttl_ms = std::max(200, std::atoi(val.c_str()));
        } else if (key == "completion.lsp_timeout_ms") {
            s->lsp_completion_timeout_ms = std::max(50, std::atoi(val.c_str()));
        }
    }
    apply_appearance(s);
    if (announce) {
        set_status(s, "Config reloaded");
    }
}

void maybe_hot_reload_config(AppState* s) {
    if (s == nullptr) {
        return;
    }
    std::error_code ec;
    const fs::path cfg = config_path(s);
    if (!fs::exists(cfg)) {
        return;
    }
    const auto ft = fs::last_write_time(cfg, ec);
    if (ec) {
        return;
    }
    const long long mt = static_cast<long long>(ft.time_since_epoch().count());
    if (s->config_last_mtime_ticks == 0) {
        s->config_last_mtime_ticks = mt;
        return;
    }
    if (mt != s->config_last_mtime_ticks) {
        s->config_last_mtime_ticks = mt;
        apply_runtime_config_file(s, true);
    }
}

std::vector<std::string> workspace_roots_or_default(const AppState* s) {
    std::vector<std::string> roots = s->workspace_roots;
    if (roots.empty() && s != nullptr) {
        roots.push_back(s->root);
    }
    return roots;
}

void persist_search_index(AppState* s) {
    if (s == nullptr) {
        return;
    }
    std::ofstream out(search_index_path(s));
    if (!out.is_open()) {
        return;
    }
    std::lock_guard<std::mutex> lk(s->search_index_mu);
    for (const auto& [token, files] : s->search_index) {
        out << token;
        bool first = true;
        for (const std::string& f : files) {
            out << (first ? '\t' : ';') << f;
            first = false;
        }
        out << '\n';
    }
}

void load_search_index(AppState* s) {
    if (s == nullptr) {
        return;
    }
    std::ifstream in(search_index_path(s));
    if (!in.is_open()) {
        s->search_index_loaded = false;
        return;
    }
    std::unordered_map<std::string, std::unordered_set<std::string>> idx;
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t tab = line.find('\t');
        if (tab == std::string::npos) {
            continue;
        }
        const std::string token = line.substr(0, tab);
        std::stringstream ss(line.substr(tab + 1));
        std::string f;
        while (std::getline(ss, f, ';')) {
            if (!f.empty()) {
                idx[token].insert(f);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lk(s->search_index_mu);
        s->search_index.swap(idx);
    }
    s->search_index_loaded = true;
}

std::vector<std::string> tokenize_identifiers(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : text) {
        if (is_ident_char(c)) {
            cur.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (!cur.empty()) {
            if (cur.size() >= 2) {
                out.push_back(cur);
            }
            cur.clear();
        }
    }
    if (!cur.empty() && cur.size() >= 2) {
        out.push_back(cur);
    }
    return out;
}

void rebuild_search_index_async(AppState* s) {
    if (s == nullptr || s->search_index_worker_running.exchange(true)) {
        return;
    }
    std::vector<std::string> roots = workspace_roots_or_default(s);
    std::thread([s, roots]() {
        std::unordered_map<std::string, std::unordered_set<std::string>> local;
        for (const std::string& root : roots) {
            if (!fs::exists(root)) {
                continue;
            }
            for (auto it = fs::recursive_directory_iterator(root); it != fs::recursive_directory_iterator(); ++it) {
                const auto& e = *it;
                if (e.is_directory()) {
                    const std::string base = e.path().filename().string();
                    if (should_skip_dir_name(base)) {
                        it.disable_recursion_pending();
                    }
                    continue;
                }
                if (!e.is_regular_file() || !is_source(e.path())) {
                    continue;
                }
                std::ifstream in(e.path());
                if (!in.is_open()) {
                    continue;
                }
                std::stringstream ss;
                ss << in.rdbuf();
                const std::string rel = to_project_relative(s, e.path().string());
                for (const std::string& tok : tokenize_identifiers(ss.str())) {
                    if (local.size() > 120000) {
                        break;
                    }
                    local[tok].insert(rel);
                }
            }
        }
        {
            std::lock_guard<std::mutex> lk(s->search_index_mu);
            s->search_index.swap(local);
        }
        s->search_index_loaded = true;
        persist_search_index(s);
        s->search_index_worker_running = false;
        g_idle_add(
            [](gpointer data) -> gboolean {
                auto* st = static_cast<AppState*>(data);
                set_status(st, "Search index rebuilt");
                return G_SOURCE_REMOVE;
            },
            s);
    }).detach();
}

void clear_inline_diagnostic_tags(EditorTab* tab) {
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }
    GtkTextIter st;
    GtkTextIter en;
    gtk_text_buffer_get_start_iter(tab->buffer, &st);
    gtk_text_buffer_get_end_iter(tab->buffer, &en);
    gtk_text_buffer_remove_tag_by_name(tab->buffer, "diag-error", &st, &en);
    gtk_text_buffer_remove_tag_by_name(tab->buffer, "diag-warning", &st, &en);
    gtk_text_buffer_remove_tag_by_name(tab->buffer, "diag-info", &st, &en);
}

void apply_inline_diagnostics(AppState* s, EditorTab* tab) {
    if (s == nullptr || tab == nullptr || tab->buffer == nullptr) {
        return;
    }
    clear_inline_diagnostic_tags(tab);
    GtkTextTagTable* tbl = gtk_text_buffer_get_tag_table(tab->buffer);
    if (gtk_text_tag_table_lookup(tbl, "diag-error") == nullptr) {
        gtk_text_buffer_create_tag(tab->buffer, "diag-error", "underline", PANGO_UNDERLINE_ERROR, "background", "#5b1f1f", nullptr);
        gtk_text_buffer_create_tag(tab->buffer, "diag-warning", "underline", PANGO_UNDERLINE_SINGLE, "background", "#5b4d1f", nullptr);
        gtk_text_buffer_create_tag(tab->buffer, "diag-info", "underline", PANGO_UNDERLINE_SINGLE, "background", "#1f3a5b", nullptr);
    }
    for (const ProblemItem& p : s->all_problems) {
        if (normalize_path(s, p.file) != tab->path) {
            continue;
        }
        GtkTextIter ls;
        GtkTextIter le;
        gtk_text_buffer_get_iter_at_line(tab->buffer, &ls, std::max(0, p.line - 1));
        le = ls;
        gtk_text_iter_forward_to_line_end(&le);
        const std::string sev = lower_copy(p.severity);
        const char* tag = sev == "error" ? "diag-error" : (sev == "warning" ? "diag-warning" : "diag-info");
        gtk_text_buffer_apply_tag_by_name(tab->buffer, tag, &ls, &le);
    }
}

void append_terminal_text(AppState* s, const std::string& text) {
    if (s == nullptr || s->terminal_output_buffer == nullptr) {
        return;
    }
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(s->terminal_output_buffer, &end);
    gtk_text_buffer_insert(s->terminal_output_buffer, &end, text.c_str(), -1);
    gtk_text_buffer_get_end_iter(s->terminal_output_buffer, &end);
    gtk_text_buffer_place_cursor(s->terminal_output_buffer, &end);
    if (s->terminal_output_view != nullptr) {
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(s->terminal_output_view), &end, 0.0, FALSE, 0.0, 1.0);
    }
}

void run_terminal_command(AppState* s, const std::string& command) {
    if (command.empty()) {
        return;
    }
    gchar* q_root = g_shell_quote(s->root.c_str());
    std::string cmd = "cd " + std::string(q_root) + " && " + command;
    g_free(q_root);
    append_terminal_text(s, "$ " + command + "\n");

    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint exit_status = 0;
    GError* error = nullptr;
    gboolean ok = g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &exit_status, &error);
    if (!ok) {
        append_terminal_text(s, std::string("[terminal error] ") + (error != nullptr ? error->message : "unknown") + "\n");
        if (error != nullptr) {
            g_error_free(error);
        }
        g_free(out_buf);
        g_free(err_buf);
        return;
    }

    if (out_buf != nullptr && *out_buf != '\0') {
        append_terminal_text(s, out_buf);
    }
    if (err_buf != nullptr && *err_buf != '\0') {
        append_terminal_text(s, err_buf);
    }
    append_terminal_text(s, "[exit " + std::to_string(exit_status) + "]\n\n");
    g_free(out_buf);
    g_free(err_buf);
}

void apply_appearance(AppState* s) {
    if (s == nullptr) {
        return;
    }
    GtkSettings* settings = gtk_settings_get_default();
    if (settings != nullptr) {
        const gboolean dark = (s->theme_mode == "Dark");
        if (s->theme_mode == "System") {
            g_object_set(settings, "gtk-application-prefer-dark-theme", FALSE, nullptr);
        } else {
            g_object_set(settings, "gtk-application-prefer-dark-theme", dark, nullptr);
        }
    }

    if (s->css_provider == nullptr) {
        s->css_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                                  GTK_STYLE_PROVIDER(s->css_provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    s->ui_zoom_percent = std::max(70, std::min(200, s->ui_zoom_percent));
    const int font_size_pt = effective_editor_font_size(s);
    const int pad = s->compact_mode ? 2 : 6;
    const int spacing = s->compact_mode ? 2 : 6;
    const int row_min = s->compact_mode ? 20 : 26;
    const std::string fg = s->high_contrast ? "#ffffff" : "#e8e8e8";
    const std::string bg = s->high_contrast ? "#000000" : "#1f1f1f";
    const std::string css = "textview, entry { font-family: '" + s->font_family + "'; font-size: " +
                            std::to_string(font_size_pt) + "pt; } "
                            "textview text { color: " +
                            fg + "; background: " + bg + "; } "
                            "treeview, list, listbox row { min-height: " + std::to_string(row_min) + "px; } "
                            ".toolbar button, button { padding: " + std::to_string(pad) + "px; } "
                            "paned { -GtkPaned-handle-size: " + std::to_string(s->compact_mode ? 4 : 7) + "; } "
                            "notebook tab { padding: " + std::to_string(pad) + "px; } "
                            "box, grid { border-spacing: " + std::to_string(spacing) + "px; }";
    gtk_css_provider_load_from_data(s->css_provider, css.c_str(), -1, nullptr);

    for (EditorTab* t : s->tabs_primary) {
        apply_editor_settings_to_tab(s, t);
    }
    for (EditorTab* t : s->tabs_secondary) {
        apply_editor_settings_to_tab(s, t);
    }
}

void update_tab_label(EditorTab* tab) {
    if (tab == nullptr || tab->tab_label == nullptr) {
        return;
    }
    std::string title = fs::path(tab->path).filename().string();
    if (tab->dirty) {
        title += " *";
    }
    gtk_label_set_text(GTK_LABEL(tab->tab_label), title.c_str());
}

void jump_to_line(EditorTab* tab, int one_based_line, int one_based_col) {
    if (tab == nullptr || tab->buffer == nullptr || tab->text_view == nullptr) {
        return;
    }

    const int line = std::max(0, one_based_line - 1);
    const int col = std::max(0, one_based_col - 1);

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(tab->buffer, &iter, line);
    gtk_text_iter_set_line_offset(&iter, col);
    gtk_text_buffer_place_cursor(tab->buffer, &iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(tab->text_view), &iter, 0.25, FALSE, 0.0, 0.0);
    gtk_widget_grab_focus(tab->text_view);
}

void ensure_active_notebook(AppState* s) {
    if (s->active_notebook == nullptr) {
        s->active_notebook = s->notebook_primary;
    }
}

EditorTab* current_tab(AppState* s) {
    ensure_active_notebook(s);
    GtkWidget* notebook = s->active_notebook;
    const int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    std::vector<EditorTab*>& tabs = tabs_for_notebook(s, notebook);
    if (page < 0 || static_cast<std::size_t>(page) >= tabs.size()) {
        return nullptr;
    }
    return tabs[static_cast<std::size_t>(page)];
}

std::string get_buffer_text(GtkTextBuffer* buffer) {
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    char* txt = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    std::string out = txt != nullptr ? txt : "";
    g_free(txt);
    return out;
}

void reload_tab_from_disk(EditorTab* tab);

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

void format_buffer_basic(EditorTab* tab) {
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }
    std::istringstream in(get_buffer_text(tab->buffer));
    std::ostringstream out;
    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
            line.pop_back();
        }
        if (!first) {
            out << '\n';
        }
        first = false;
        out << line;
    }
    out << '\n';
    gtk_text_buffer_set_text(tab->buffer, out.str().c_str(), -1);
    tab->dirty = true;
    update_tab_label(tab);
}

bool run_formatter_external(AppState* s, EditorTab* tab) {
    if (s == nullptr || tab == nullptr || s->formatter_cmd.empty()) {
        return false;
    }
    const std::string cmd_templ = s->formatter_cmd;
    gchar* q_file = g_shell_quote(tab->path.c_str());
    gchar* q_root = g_shell_quote(s->root.c_str());
    std::string cmd = replace_all_copy(cmd_templ, "{file}", q_file);
    cmd = replace_all_copy(cmd, "{root}", q_root);
    cmd = "cd " + std::string(q_root) + " && " + cmd;
    g_free(q_file);
    g_free(q_root);

    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint status = 0;
    GError* error = nullptr;
    gboolean ok = g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &status, &error);
    if (!ok || status != 0) {
        if (error != nullptr) {
            append_terminal_text(s, std::string("[formatter] ") + error->message + "\n");
            g_error_free(error);
        } else if (err_buf != nullptr && *err_buf != '\0') {
            append_terminal_text(s, std::string("[formatter] ") + err_buf + "\n");
        }
        g_free(out_buf);
        g_free(err_buf);
        return false;
    }
    g_free(out_buf);
    g_free(err_buf);
    return true;
}

void maybe_format_before_save(AppState* s, EditorTab* tab) {
    if (s == nullptr || tab == nullptr || !s->formatter_on_save) {
        return;
    }
    if (!run_formatter_external(s, tab)) {
        format_buffer_basic(tab);
    } else {
        reload_tab_from_disk(tab);
    }
}

void save_tab(AppState* s, EditorTab* tab) {
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }

    maybe_format_before_save(s, tab);

    std::ofstream out(tab->path);
    if (!out.is_open()) {
        set_status(s, "Save failed: " + tab->path);
        return;
    }

    out << get_buffer_text(tab->buffer);
    if (!out.good()) {
        set_status(s, "Save failed: write error");
        return;
    }

    tab->dirty = false;
    update_tab_label(tab);
    set_status(s, "Saved: " + tab->path);
}

void reload_tab_from_disk(EditorTab* tab) {
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }
    std::ifstream in(tab->path);
    if (!in.is_open()) {
        return;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    gtk_text_buffer_set_text(tab->buffer, ss.str().c_str(), -1);
    tab->dirty = false;
    update_tab_label(tab);
}

std::vector<SymbolDef> extract_symbols_from_text(const std::string& text, const std::string& file) {
    std::vector<SymbolDef> symbols;
    std::regex rx(R"(^\s*(proc|form|entry|trait)\s+([A-Za-z_][A-Za-z0-9_]*)\b)");

    std::istringstream iss(text);
    std::string line;
    int ln = 1;
    while (std::getline(iss, line)) {
        std::smatch m;
        if (std::regex_search(line, m, rx)) {
            SymbolDef sym;
            sym.kind = m[1].str();
            sym.name = m[2].str();
            sym.file = file;
            sym.line = ln;
            sym.col = static_cast<int>(m.position(2)) + 1;
            symbols.push_back(sym);
        }
        ++ln;
    }
    return symbols;
}

const char* outline_icon_for_kind(const std::string& kind_in) {
    const std::string kind = lower_copy(kind_in);
    if (kind == "proc") {
        return "system-run";
    }
    if (kind == "form") {
        return "x-office-address-book";
    }
    if (kind == "entry") {
        return "media-playback-start";
    }
    if (kind == "trait") {
        return "applications-development";
    }
    return "text-x-generic";
}

void refresh_outline(AppState* s) {
    if (s == nullptr || s->outline_store == nullptr) {
        return;
    }
    gtk_tree_store_clear(s->outline_store);
    s->current_symbols.clear();

    EditorTab* tab = current_tab(s);
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }

    const std::string txt = get_buffer_text(tab->buffer);
    s->current_symbols = extract_symbols_from_text(txt, tab->path);

    std::string filter;
    if (s->outline_filter_entry != nullptr) {
        filter = trim_copy(lower_copy(gtk_entry_get_text(GTK_ENTRY(s->outline_filter_entry))));
    }

    std::vector<const SymbolDef*> procs;
    std::vector<const SymbolDef*> forms;
    std::vector<const SymbolDef*> entries;
    std::vector<const SymbolDef*> traits;
    std::vector<const SymbolDef*> others;

    auto matches_filter = [&filter](const SymbolDef& sym) -> bool {
        if (filter.empty()) {
            return true;
        }
        const std::string name = lower_copy(sym.name);
        const std::string kind = lower_copy(sym.kind);
        return name.find(filter) != std::string::npos || kind.find(filter) != std::string::npos || fuzzy_match(filter, name);
    };

    for (const SymbolDef& sym : s->current_symbols) {
        if (!matches_filter(sym)) {
            continue;
        }
        const std::string k = lower_copy(sym.kind);
        if (k == "proc") {
            procs.push_back(&sym);
        } else if (k == "form") {
            forms.push_back(&sym);
        } else if (k == "entry") {
            entries.push_back(&sym);
        } else if (k == "trait") {
            traits.push_back(&sym);
        } else {
            others.push_back(&sym);
        }
    }

    auto append_group = [&](const char* title, const char* icon, const std::vector<const SymbolDef*>& items) {
        if (items.empty()) {
            return 0;
        }
        GtkTreeIter group_iter;
        gtk_tree_store_append(s->outline_store, &group_iter, nullptr);
        gtk_tree_store_set(s->outline_store,
                           &group_iter,
                           OUT_ICON,
                           icon,
                           OUT_KIND,
                           "",
                           OUT_NAME,
                           title,
                           OUT_LINE,
                           0,
                           OUT_IS_GROUP,
                           TRUE,
                           -1);

        for (const SymbolDef* sym : items) {
            GtkTreeIter child;
            gtk_tree_store_append(s->outline_store, &child, &group_iter);
            gtk_tree_store_set(s->outline_store,
                               &child,
                               OUT_ICON,
                               outline_icon_for_kind(sym->kind),
                               OUT_KIND,
                               sym->kind.c_str(),
                               OUT_NAME,
                               sym->name.c_str(),
                               OUT_LINE,
                               sym->line,
                               OUT_IS_GROUP,
                               FALSE,
                               -1);
        }

        if (s->outline_view != nullptr) {
            GtkTreePath* p = gtk_tree_model_get_path(GTK_TREE_MODEL(s->outline_store), &group_iter);
            if (p != nullptr) {
                if (s->outline_collapsed_groups.find(title) != s->outline_collapsed_groups.end()) {
                    gtk_tree_view_collapse_row(GTK_TREE_VIEW(s->outline_view), p);
                } else {
                    gtk_tree_view_expand_row(GTK_TREE_VIEW(s->outline_view), p, FALSE);
                }
                gtk_tree_path_free(p);
            }
        }
        return static_cast<int>(items.size());
    };

    int shown = 0;
    shown += append_group("proc", "system-run", procs);
    shown += append_group("form", "x-office-address-book", forms);
    shown += append_group("entry", "media-playback-start", entries);
    shown += append_group("trait", "applications-development", traits);
    shown += append_group("other", "text-x-generic", others);

    if (s->outline_empty_label != nullptr) {
        if (shown == 0) {
            const char* msg = filter.empty() ? "Aucun symbole trouvé" : "Aucun symbole pour ce filtre";
            gtk_label_set_text(GTK_LABEL(s->outline_empty_label), msg);
            gtk_widget_show(s->outline_empty_label);
        } else {
            gtk_widget_hide(s->outline_empty_label);
        }
    }
}

void on_outline_filter_changed(GtkEditable*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    refresh_outline(s);
}

void on_outline_filter_clear(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr || s->outline_filter_entry == nullptr) {
        return;
    }
    gtk_entry_set_text(GTK_ENTRY(s->outline_filter_entry), "");
    refresh_outline(s);
}

void refresh_problems_view(AppState* s) {
    gtk_list_store_clear(s->problems_store);
    if (s->gutter_store != nullptr) {
        gtk_list_store_clear(s->gutter_store);
    }

    std::string current_file;
    if (EditorTab* tab = current_tab(s); tab != nullptr) {
        current_file = tab->path;
    }

    for (const ProblemItem& p : s->all_problems) {
        if (s->problem_filter == ProblemFilter::ErrorsOnly) {
            if (lower_copy(p.severity) != "error") {
                continue;
            }
        } else if (s->problem_filter == ProblemFilter::CurrentFile) {
            if (current_file.empty()) {
                continue;
            }
            if (normalize_path(s, p.file) != current_file) {
                continue;
            }
        }

        GtkTreeIter iter;
        gtk_list_store_append(s->problems_store, &iter);
        gtk_list_store_set(s->problems_store,
                           &iter,
                           PROB_FILE,
                           p.file.c_str(),
                           PROB_LINE,
                           p.line,
                           PROB_COL,
                           p.col,
                           PROB_SEV,
                           p.severity.c_str(),
                           PROB_TEXT,
                           p.text.c_str(),
                           -1);
        if (s->gutter_store != nullptr && normalize_path(s, p.file) == current_file) {
            GtkTreeIter giter;
            gtk_list_store_append(s->gutter_store, &giter);
            std::string label = std::to_string(p.line) + " " + p.severity;
            gtk_list_store_set(s->gutter_store, &giter, 0, label.c_str(), -1);
        }
    }
    if (EditorTab* tab = current_tab(s); tab != nullptr) {
        apply_inline_diagnostics(s, tab);
    }
}

void clear_problems(AppState* s) {
    s->all_problems.clear();
    refresh_problems_view(s);
}

void add_problem(AppState* s, const std::string& file, int line, int col, const std::string& severity, const std::string& text) {
    ProblemItem item;
    item.file = file;
    item.line = std::max(1, line);
    item.col = std::max(1, col);
    item.severity = severity;
    item.text = text;
    s->all_problems.push_back(item);
    refresh_problems_view(s);
}

bool confirm_discard_if_dirty(AppState* s, EditorTab* tab) {
    if (tab == nullptr || !tab->dirty) {
        return true;
    }

    GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(s->window),
                                           GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_WARNING,
                                           GTK_BUTTONS_NONE,
                                           "Unsaved changes in %s",
                                           fs::path(tab->path).filename().string().c_str());
    gtk_dialog_add_buttons(GTK_DIALOG(d), "_Cancel", GTK_RESPONSE_CANCEL, "_Discard", GTK_RESPONSE_REJECT, "_Save", GTK_RESPONSE_ACCEPT, nullptr);
    const int rc = gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);

    if (rc == GTK_RESPONSE_ACCEPT) {
        save_tab(s, tab);
        return !tab->dirty;
    }
    if (rc == GTK_RESPONSE_REJECT) {
        return true;
    }
    return false;
}

void close_tab(AppState* s, EditorTab* tab) {
    if (tab == nullptr || tab->owner_notebook == nullptr) {
        return;
    }

    if (!confirm_discard_if_dirty(s, tab)) {
        return;
    }

    std::vector<EditorTab*>& tabs = tabs_for_notebook(s, tab->owner_notebook);
    auto it = std::find(tabs.begin(), tabs.end(), tab);
    if (it == tabs.end()) {
        return;
    }

    const int idx = static_cast<int>(std::distance(tabs.begin(), it));
    gtk_notebook_remove_page(GTK_NOTEBOOK(tab->owner_notebook), idx);
    tabs.erase(it);
    delete tab;

    refresh_outline(s);
    refresh_problems_view(s);
}

void on_tab_close_clicked(GtkWidget*, gpointer user_data) {
    auto* tab = static_cast<EditorTab*>(user_data);
    if (tab == nullptr) {
        return;
    }

    GtkWidget* notebook = tab->owner_notebook;
    GtkWidget* top = gtk_widget_get_toplevel(notebook);
    AppState* s = static_cast<AppState*>(g_object_get_data(G_OBJECT(top), "app-state"));
    if (s == nullptr) {
        return;
    }
    close_tab(s, tab);
}

gboolean on_completion_prefetch_tick(gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr) {
        return G_SOURCE_REMOVE;
    }
    s->completion_prefetch_timer = 0;
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return G_SOURCE_REMOVE;
    }
    int line = 1;
    int col = 1;
    current_cursor_position(tab, &line, &col);
    const std::string key = completion_cache_key(tab->path, line, current_symbol_under_cursor(tab));
    if (key == s->completion_prefetch_key) {
        return G_SOURCE_REMOVE;
    }
    s->completion_prefetch_key = key;
    bool lsp_timed_out = false;
    (void)collect_completion_suggestions(s, tab, &lsp_timed_out);
    return G_SOURCE_REMOVE;
}

gboolean run_deferred_startup(gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr) {
        return G_SOURCE_REMOVE;
    }
    set_status(s, "Loading workspace...");
    build_project_tree(s);
    load_project_tasks(s);
    load_plugins(s);
    load_search_index(s);
    if (!s->search_index_loaded) {
        rebuild_search_index_async(s);
    }
    restore_recovery_snapshot(s);
    set_status(s, "Ready");
    return G_SOURCE_REMOVE;
}

GtkWidget* make_tab_header(EditorTab* tab) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget* label = gtk_label_new(fs::path(tab->path).filename().string().c_str());
    GtkWidget* close_btn = gtk_button_new_with_label("x");
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(close_btn, 20, 20);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), close_btn, FALSE, FALSE, 0);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_tab_close_clicked), tab);
    tab->tab_label = label;
    tab->tab_header = box;
    return box;
}

void on_buffer_changed(GtkTextBuffer* buffer, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = find_tab_by_buffer(s, buffer);
    if (tab == nullptr) {
        return;
    }
    tab->dirty = true;
    update_tab_label(tab);
    if (tab != current_tab(s)) {
        return;
    }
    if (!s->perf_mode_enabled) {
        refresh_outline(s);
    } else {
        if (s->deferred_outline_timer != 0) {
            g_source_remove(s->deferred_outline_timer);
            s->deferred_outline_timer = 0;
        }
        s->deferred_outline_timer = g_timeout_add(
            220,
            [](gpointer data) -> gboolean {
                auto* st = static_cast<AppState*>(data);
                st->deferred_outline_timer = 0;
                refresh_outline(st);
                return G_SOURCE_REMOVE;
            },
            s);
    }
    if (s->completion_prefetch_timer != 0) {
        g_source_remove(s->completion_prefetch_timer);
        s->completion_prefetch_timer = 0;
    }
    s->completion_prefetch_timer = g_timeout_add(180, on_completion_prefetch_tick, s);
}

EditorTab* open_file_in_notebook(AppState* s, const std::string& path, GtkWidget* notebook, bool focus) {
    const std::string abs = fs::absolute(path).string();
    if (EditorTab* existing = find_open_tab(s, abs); existing != nullptr) {
        GtkWidget* owner = existing->owner_notebook;
        std::vector<EditorTab*>& tabs = tabs_for_notebook(s, owner);
        for (std::size_t i = 0; i < tabs.size(); ++i) {
            if (tabs[i] == existing) {
                gtk_notebook_set_current_page(GTK_NOTEBOOK(owner), static_cast<int>(i));
                if (focus) {
                    s->active_notebook = owner;
                    gtk_widget_grab_focus(existing->text_view);
                }
                return existing;
            }
        }
    }

    std::ifstream in(abs);
    if (!in.is_open()) {
        set_status(s, "Open failed: " + abs);
        return nullptr;
    }

    std::stringstream ss;
    ss << in.rdbuf();

    auto* tab = new EditorTab();
    tab->path = abs;
    tab->owner_notebook = notebook;
    tab->scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    tab->text_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tab->text_view), TRUE);
    apply_editor_settings_to_tab(s, tab);
    gtk_container_add(GTK_CONTAINER(tab->scrolled), tab->text_view);
    tab->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view));
    gtk_text_buffer_set_text(tab->buffer, ss.str().c_str(), -1);
    tab->dirty = false;

    g_signal_connect(tab->buffer, "changed", G_CALLBACK(on_buffer_changed), s);

    GtkWidget* header = make_tab_header(tab);
    const int page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab->scrolled, header);
    gtk_widget_show_all(tab->scrolled);
    gtk_widget_show_all(header);

    std::vector<EditorTab*>& tabs = tabs_for_notebook(s, notebook);
    tabs.push_back(tab);

    if (focus) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
        s->active_notebook = notebook;
        gtk_widget_grab_focus(tab->text_view);
    }

    set_status(s, "Opened: " + abs);
    refresh_outline(s);
    apply_inline_diagnostics(s, tab);
    return tab;
}

EditorTab* open_file_in_active_pane(AppState* s, const std::string& path) {
    ensure_active_notebook(s);
    return open_file_in_notebook(s, path, s->active_notebook, true);
}

void add_project_node(AppState* s, GtkTreeIter* parent, const fs::path& p) {
    GtkTreeIter iter;
    const std::string name = p.filename().string().empty() ? p.string() : p.filename().string();
    const std::string full = p.string();
    gtk_tree_store_append(s->project_store, &iter, parent);
    gtk_tree_store_set(s->project_store, &iter, COL_NAME, name.c_str(), COL_PATH, full.c_str(), -1);

    if (!fs::is_directory(p)) {
        return;
    }

    std::vector<fs::path> children;
    for (const auto& e : fs::directory_iterator(p)) {
        children.push_back(e.path());
    }
    std::sort(children.begin(), children.end());

    for (const auto& c : children) {
        const std::string base = c.filename().string();
        if (should_skip_dir_name(base)) {
            continue;
        }
        if (fs::is_directory(c) || is_source(c)) {
            add_project_node(s, &iter, c);
        }
    }
}

void for_each_workspace_source_file(const AppState* s, const std::function<void(const fs::path&)>& fn) {
    if (s == nullptr) {
        return;
    }
    std::vector<std::string> roots = s->workspace_roots;
    if (roots.empty()) {
        roots.push_back(s->root);
    }
    for (const std::string& root : roots) {
        if (!fs::exists(root)) {
            continue;
        }
        for (auto it = fs::recursive_directory_iterator(root); it != fs::recursive_directory_iterator(); ++it) {
            const auto& e = *it;
            if (e.is_directory()) {
                const std::string base = e.path().filename().string();
                if (should_skip_dir_name(base)) {
                    it.disable_recursion_pending();
                }
                continue;
            }
            if (!e.is_regular_file()) {
                continue;
            }
            const fs::path p = e.path();
            const std::string base = p.filename().string();
            if (base == "tasks.vitte.json") {
                continue;
            }
            if (is_source(p)) {
                fn(p);
            }
        }
    }
}

void build_project_tree(AppState* s) {
    gtk_tree_store_clear(s->project_store);
    if (s == nullptr) {
        return;
    }
    std::vector<std::string> roots = s->workspace_roots;
    if (roots.empty()) {
        roots.push_back(s->root);
    }
    for (const std::string& root : roots) {
        fs::path rp(root);
        if (!fs::exists(rp)) {
            continue;
        }
        add_project_node(s, nullptr, rp);
    }
}

void on_project_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkTreeIter iter;
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return;
    }

    char* full = nullptr;
    gtk_tree_model_get(model, &iter, COL_PATH, &full, -1);
    if (full == nullptr) {
        return;
    }

    fs::path p(full);
    g_free(full);
    if (fs::is_regular_file(p) && is_source(p)) {
        open_file_in_active_pane(s, p.string());
    }
}

void on_outline_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return;
    }

    gboolean is_group = FALSE;
    int line = 1;
    gtk_tree_model_get(model, &iter, OUT_IS_GROUP, &is_group, OUT_LINE, &line, -1);
    if (is_group) {
        gchar* group_name = nullptr;
        gtk_tree_model_get(model, &iter, OUT_NAME, &group_name, -1);
        const std::string name = group_name != nullptr ? group_name : "";
        g_free(group_name);
        if (gtk_tree_view_row_expanded(tree_view, path)) {
            gtk_tree_view_collapse_row(tree_view, path);
            if (!name.empty()) {
                s->outline_collapsed_groups.insert(name);
            }
        } else {
            gtk_tree_view_expand_row(tree_view, path, FALSE);
            if (!name.empty()) {
                s->outline_collapsed_groups.erase(name);
            }
        }
        return;
    }
    if (EditorTab* tab = current_tab(s); tab != nullptr) {
        jump_to_line(tab, line, 1);
    }
}

void on_problem_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return;
    }

    char* file = nullptr;
    char* text = nullptr;
    int line = 1;
    int col = 1;
    gtk_tree_model_get(model, &iter, PROB_FILE, &file, PROB_LINE, &line, PROB_COL, &col, PROB_TEXT, &text, -1);
    if (file == nullptr) {
        return;
    }

    const std::string abs = normalize_path(s, file);
    g_free(file);

    EditorTab* tab = open_file_in_active_pane(s, abs);
    jump_to_line(tab, line, col);

    std::string msg = text != nullptr ? text : "";
    g_free(text);
    std::smatch m;
    if (std::regex_search(msg, m, std::regex("(VITTE-[A-Z][0-9]{4})"))) {
        const std::string code = m.str(1);
        const fs::path idx = fs::path(primary_workspace_root(s)) / "target/reports/diagnostics_index.json";
        if (fs::exists(idx)) {
            const std::string idx_text = read_file_text(idx);
            const std::string marker = "\"" + code + "\"";
            const std::size_t pos = idx_text.find(marker);
            if (pos != std::string::npos) {
                const std::size_t end = idx_text.find("}", pos);
                std::string snippet = idx_text.substr(pos, end == std::string::npos ? std::string::npos : (end - pos + 1));
                show_tools_output(s, "Diagnostic Quick-fix", code + "\n\n" + snippet);
            }
        }
    }
}

void on_gutter_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return;
    }
    char* txt = nullptr;
    gtk_tree_model_get(model, &iter, 0, &txt, -1);
    if (txt == nullptr) {
        return;
    }
    const int line = std::max(1, std::atoi(txt));
    g_free(txt);
    if (EditorTab* tab = current_tab(s); tab != nullptr) {
        jump_to_line(tab, line, 1);
    }
}

void on_notebook_switch_page(GtkNotebook* notebook, GtkWidget*, guint, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    s->active_notebook = GTK_WIDGET(notebook);
    refresh_outline(s);
    refresh_problems_view(s);
    if (EditorTab* tab = current_tab(s); tab != nullptr) {
        apply_inline_diagnostics(s, tab);
    }
}

void on_notebook_focus(GtkWidget* notebook, GdkEventFocus*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    s->active_notebook = notebook;
}

void apply_split_layout(AppState* s) {
    gtk_orientable_set_orientation(GTK_ORIENTABLE(s->editor_split), s->split_orientation);
    if (s->split_enabled) {
        gtk_widget_show(s->notebook_secondary);
        gtk_paned_set_position(GTK_PANED(s->editor_split), 760);
    } else {
        gtk_widget_hide(s->notebook_secondary);
    }
}

void on_toggle_split(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    s->split_enabled = !s->split_enabled;
    apply_split_layout(s);
    set_status(s, s->split_enabled ? "Split editor enabled" : "Split editor disabled");
}

void on_toggle_split_orientation(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    s->split_orientation = (s->split_orientation == GTK_ORIENTATION_HORIZONTAL)
                               ? GTK_ORIENTATION_VERTICAL
                               : GTK_ORIENTATION_HORIZONTAL;
    apply_split_layout(s);
    set_status(s, s->split_orientation == GTK_ORIENTATION_HORIZONTAL ? "Split horizontal" : "Split vertical");
}

void on_switch_active_pane(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (!s->split_enabled) {
        return;
    }
    s->active_notebook = (s->active_notebook == s->notebook_primary) ? s->notebook_secondary : s->notebook_primary;
    EditorTab* tab = current_tab(s);
    if (tab != nullptr) {
        gtk_widget_grab_focus(tab->text_view);
    }
}

void on_action_open(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Open File",
                                                     GTK_WINDOW(s->window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel",
                                                     GTK_RESPONSE_CANCEL,
                                                     "_Open",
                                                     GTK_RESPONSE_ACCEPT,
                                                     nullptr);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), s->root.c_str());
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename != nullptr) {
            open_file_in_active_pane(s, filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

void on_action_save(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    save_tab(s, current_tab(s));
}

void run_find_in_files(AppState* s, const std::string& pattern) {
    clear_problems(s);
    const auto t0 = std::chrono::steady_clock::now();

    gchar* q_pat = g_shell_quote(pattern.c_str());
    std::vector<std::string> targets;
    std::string needle = lower_copy(pattern);
    if (!needle.empty() && std::all_of(needle.begin(), needle.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; }) &&
        s->search_index_loaded) {
        std::lock_guard<std::mutex> lk(s->search_index_mu);
        auto it = s->search_index.find(needle);
        if (it != s->search_index.end()) {
            for (const std::string& f : it->second) {
                targets.push_back(normalize_path(s, f));
            }
        }
    }
    if (targets.empty()) {
        targets = workspace_roots_or_default(s);
    }

    std::string cmd = "rg --line-number --column --no-heading --color=never " + std::string(q_pat);
    for (const std::string& r : targets) {
        gchar* q = g_shell_quote(r.c_str());
        cmd += " " + std::string(q);
        g_free(q);
    }
    g_free(q_pat);

    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint status = 0;
    GError* error = nullptr;
    const gboolean ok = g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &status, &error);
    if (!ok) {
        set_status(s, std::string("rg failed: ") + (error != nullptr ? error->message : "unknown"));
        if (error != nullptr) {
            g_error_free(error);
        }
        g_free(out_buf);
        g_free(err_buf);
        return;
    }

    std::stringstream ss(out_buf != nullptr ? out_buf : "");
    std::string line;
    int count = 0;
    while (std::getline(ss, line)) {
        const std::size_t p1 = line.find(':');
        const std::size_t p2 = (p1 == std::string::npos) ? std::string::npos : line.find(':', p1 + 1);
        const std::size_t p3 = (p2 == std::string::npos) ? std::string::npos : line.find(':', p2 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) {
            continue;
        }

        const std::string file = line.substr(0, p1);
        const int ln = std::max(1, std::atoi(line.substr(p1 + 1, p2 - p1 - 1).c_str()));
        const int col = std::max(1, std::atoi(line.substr(p2 + 1, p3 - p2 - 1).c_str()));
        const std::string txt = line.substr(p3 + 1);
        add_problem(s, file, ln, col, "info", txt);
        ++count;
    }

    g_free(out_buf);
    g_free(err_buf);
    set_status(s, "Find in files: " + std::to_string(count) + " match(es)");
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    record_telemetry(s, "find_in_files", "ms=" + std::to_string(ms) + ",count=" + std::to_string(count));
}

void on_action_find(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Find In Files",
                                                     GTK_WINDOW(s->window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Cancel",
                                                     GTK_RESPONSE_CANCEL,
                                                     "_Find",
                                                     GTK_RESPONSE_ACCEPT,
                                                     nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Regex pattern (rg)");
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 8);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char* pat = gtk_entry_get_text(GTK_ENTRY(entry));
        if (pat != nullptr && *pat != '\0') {
            run_find_in_files(s, pat);
        }
    }
    gtk_widget_destroy(dialog);
}

void on_action_replace(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Replace In Files",
                                                     GTK_WINDOW(s->window),
                                                     GTK_DIALOG_MODAL,
                                                     "_Cancel",
                                                     GTK_RESPONSE_CANCEL,
                                                     "_Preview",
                                                     GTK_RESPONSE_OK,
                                                     "_Apply",
                                                     GTK_RESPONSE_ACCEPT,
                                                     nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* find_entry = gtk_entry_new();
    GtkWidget* repl_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(find_entry), "Find text");
    gtk_entry_set_placeholder_text(GTK_ENTRY(repl_entry), "Replace with");
    gtk_box_pack_start(GTK_BOX(box), find_entry, TRUE, TRUE, 6);
    gtk_box_pack_start(GTK_BOX(box), repl_entry, TRUE, TRUE, 6);
    gtk_widget_show_all(dialog);

    const int resp = gtk_dialog_run(GTK_DIALOG(dialog));
    const std::string find = gtk_entry_get_text(GTK_ENTRY(find_entry));
    const std::string repl = gtk_entry_get_text(GTK_ENTRY(repl_entry));
    gtk_widget_destroy(dialog);

    if (find.empty()) {
        return;
    }

    clear_problems(s);

    int files_changed = 0;
    int replacements = 0;

    for_each_workspace_source_file(s, [&](const fs::path& p) {

        std::ifstream in(p);
        if (!in.is_open()) {
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        int local = 0;

        while (std::getline(in, line)) {
            int line_hits = 0;
            std::size_t pos = 0;
            while ((pos = line.find(find, pos)) != std::string::npos) {
                ++line_hits;
                pos += find.size();
            }
            local += line_hits;

            std::string replaced = line;
            if (resp == GTK_RESPONSE_ACCEPT && line_hits > 0) {
                pos = 0;
                while ((pos = replaced.find(find, pos)) != std::string::npos) {
                    replaced.replace(pos, find.size(), repl);
                    pos += repl.size();
                }
            }
            lines.push_back(replaced);
        }

        if (local == 0) {
            return;
        }

        ++files_changed;
        replacements += local;
        add_problem(s,
                    to_project_relative(s, p.string()),
                    1,
                    1,
                    "info",
                    std::string("preview: ") + std::to_string(local) + " occurrence(s)");

        if (resp == GTK_RESPONSE_ACCEPT) {
            std::ofstream out(p);
            for (std::size_t i = 0; i < lines.size(); ++i) {
                out << lines[i];
                if (i + 1 < lines.size()) {
                    out << '\n';
                }
            }
            out << '\n';
        }
    });

    if (resp == GTK_RESPONSE_ACCEPT) {
        set_status(s, "Replace applied: " + std::to_string(replacements) + " occurrence(s)");
    } else if (resp == GTK_RESPONSE_OK) {
        set_status(s, "Replace preview: " + std::to_string(files_changed) + " file(s)");
    }
}

std::string profile_suffix(const AppState* s) {
    return (s->build_profile == "Release") ? "release" : "debug";
}

std::string current_tab_key(const AppState* s, const EditorTab* tab) {
    if (tab == nullptr) {
        return "";
    }
    return to_project_relative(s, tab->path);
}

BuildPreset effective_preset_for_tab(const AppState* s, const EditorTab* tab) {
    BuildPreset p;
    p.debug_extra = s->build_extra_debug;
    p.release_extra = s->build_extra_release;
    p.run_args = s->run_args;
    p.run_env = s->run_env;

    const std::string key = current_tab_key(s, tab);
    auto it = s->file_presets.find(key);
    if (it != s->file_presets.end()) {
        p = it->second;
    }
    return p;
}

std::string current_build_extra_for_tab(const AppState* s, const EditorTab* tab) {
    BuildPreset p = effective_preset_for_tab(s, tab);
    return (s->build_profile == "Release") ? p.release_extra : p.debug_extra;
}

fs::path output_bin_path(const AppState* s, const EditorTab* tab) {
    return fs::path(s->root) / ".vitte-cache" / "vitte-ide" /
           (fs::path(tab->path).stem().string() + "_" + profile_suffix(s) + "_gtk.out");
}

void on_action_build(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }

    save_tab(s, tab);
    clear_problems(s);

    fs::path out_bin = output_bin_path(s, tab);
    fs::create_directories(out_bin.parent_path());

    gchar* q_root = g_shell_quote(s->root.c_str());
    gchar* q_file = g_shell_quote(tab->path.c_str());
    gchar* q_out = g_shell_quote(out_bin.string().c_str());
    const std::string build_extra = trim_copy(current_build_extra_for_tab(s, tab));
    std::string cmd = "cd " + std::string(q_root) + " && env VITTE_PROFILE=" + profile_suffix(s) +
                      " vitte build " + std::string(q_file) + " -o " + std::string(q_out);
    if (!build_extra.empty()) {
        cmd += " " + build_extra;
    }
    g_free(q_root);
    g_free(q_file);
    g_free(q_out);

    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint exit_status = 0;
    g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &exit_status, nullptr);

    auto parse_lines = [&](const char* blob) {
        std::stringstream ss(blob != nullptr ? blob : "");
        std::string ln;
        while (std::getline(ss, ln)) {
            const std::string sev = (lower_copy(ln).find("error") != std::string::npos) ? "error" : "info";
            add_problem(s, to_project_relative(s, tab->path), 1, 1, sev, ln);
        }
    };

    parse_lines(out_buf);
    parse_lines(err_buf);

    g_free(out_buf);
    g_free(err_buf);
    set_status(s, "Build done (" + s->build_profile + ")");
    record_telemetry(s, "build", "profile=" + s->build_profile);
}

void on_action_check(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }

    save_tab(s, tab);
    clear_problems(s);

    gchar* q_root = g_shell_quote(s->root.c_str());
    gchar* q_file = g_shell_quote(tab->path.c_str());
    std::string cmd = "cd " + std::string(q_root) + " && env VITTE_PROFILE=" + profile_suffix(s) + " vitte check " + std::string(q_file);
    g_free(q_root);
    g_free(q_file);

    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint exit_status = 0;
    g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &exit_status, nullptr);

    auto parse_lines = [&](const char* blob) {
        std::stringstream ss(blob != nullptr ? blob : "");
        std::string ln;
        while (std::getline(ss, ln)) {
            const std::string sev = (lower_copy(ln).find("error") != std::string::npos) ? "error" : "info";
            add_problem(s, to_project_relative(s, tab->path), 1, 1, sev, ln);
        }
    };
    parse_lines(out_buf);
    parse_lines(err_buf);
    g_free(out_buf);
    g_free(err_buf);
    set_status(s, exit_status == 0 ? "Check passed" : "Check failed");
    if (exit_status != 0) {
        ++s->telemetry_errors;
    }
    record_telemetry(s, "check", std::string("rc=") + std::to_string(exit_status));
}

void on_action_run(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }

    fs::path out_bin = output_bin_path(s, tab);
    if (!fs::exists(out_bin)) {
        set_status(s, "Run failed: build first");
        return;
    }

    gchar* q_bin = g_shell_quote(out_bin.string().c_str());
    BuildPreset p = effective_preset_for_tab(s, tab);
    const std::string run_args = trim_copy(p.run_args);
    const std::string run_env = trim_copy(p.run_env);
    std::string inner = "env VITTE_PROFILE=" + profile_suffix(s);
    if (!run_env.empty()) {
        inner += " " + run_env;
    }
    inner += " " + std::string(q_bin);
    if (!run_args.empty()) {
        inner += " " + run_args;
    }
    gchar* q_inner = g_shell_quote(inner.c_str());
    std::string cmd = "x-terminal-emulator -e bash -lc " + std::string(q_inner);
    g_free(q_bin);
    g_free(q_inner);
    const int rc = std::system(cmd.c_str());
    if (rc != 0) {
        set_status(s, "Run launch failed");
        ++s->telemetry_errors;
        return;
    }
    set_status(s, "Run launched in terminal (" + s->build_profile + ")");
    record_telemetry(s, "run", s->build_profile);
}

void on_terminal_run(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr || s->terminal_cmd_entry == nullptr) {
        return;
    }
    const std::string cmd = trim_copy(gtk_entry_get_text(GTK_ENTRY(s->terminal_cmd_entry)));
    if (cmd.empty()) {
        return;
    }
    run_terminal_command(s, cmd);
}

void on_terminal_entry_activate(GtkEntry*, gpointer user_data) {
    on_terminal_run(nullptr, user_data);
}

void on_terminal_clear(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s != nullptr && s->terminal_output_buffer != nullptr) {
        gtk_text_buffer_set_text(s->terminal_output_buffer, "", -1);
    }
}

std::string escape_regex_literal(const std::string& input) {
    static const std::regex meta(R"([-[\]{}()*+?.,\\^$|#\s])");
    return std::regex_replace(input, meta, R"(\\$&)");
}

std::string current_symbol_under_cursor(EditorTab* tab) {
    if (tab == nullptr || tab->buffer == nullptr) {
        return "";
    }

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(tab->buffer, &iter, gtk_text_buffer_get_insert(tab->buffer));

    GtkTextIter start = iter;
    GtkTextIter end = iter;

    while (!gtk_text_iter_starts_line(&start)) {
        GtkTextIter prev = start;
        gtk_text_iter_backward_char(&prev);
        gunichar c = gtk_text_iter_get_char(&prev);
        if (!(g_unichar_isalnum(c) || c == '_')) {
            break;
        }
        start = prev;
    }

    while (!gtk_text_iter_ends_line(&end)) {
        gunichar c = gtk_text_iter_get_char(&end);
        if (!(g_unichar_isalnum(c) || c == '_')) {
            break;
        }
        if (!gtk_text_iter_forward_char(&end)) {
            break;
        }
    }

    char* sym = gtk_text_buffer_get_text(tab->buffer, &start, &end, FALSE);
    std::string out = sym != nullptr ? sym : "";
    g_free(sym);
    return out;
}

void current_cursor_position(EditorTab* tab, int* line, int* col) {
    if (line != nullptr) {
        *line = 1;
    }
    if (col != nullptr) {
        *col = 1;
    }
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }
    GtkTextIter it;
    gtk_text_buffer_get_iter_at_mark(tab->buffer, &it, gtk_text_buffer_get_insert(tab->buffer));
    if (line != nullptr) {
        *line = gtk_text_iter_get_line(&it) + 1;
    }
    if (col != nullptr) {
        *col = gtk_text_iter_get_line_offset(&it) + 1;
    }
}

std::string lsp_bridge_request(AppState* s, EditorTab* tab, const std::string& method) {
    if (s == nullptr || tab == nullptr || !s->lsp_bridge_enabled || s->lsp_bridge_cmd.empty()) {
        return "";
    }
    int line = 1;
    int col = 1;
    current_cursor_position(tab, &line, &col);
    const std::string sym = current_symbol_under_cursor(tab);
    gchar* q_file = g_shell_quote(tab->path.c_str());
    gchar* q_sym = g_shell_quote(sym.c_str());
    gchar* q_root = g_shell_quote(s->root.c_str());
    std::string cmd = "cd " + std::string(q_root) + " && " + s->lsp_bridge_cmd + " --" + method +
                      " --file " + std::string(q_file) + " --line " + std::to_string(line) + " --col " +
                      std::to_string(col) + " --symbol " + std::string(q_sym);
    g_free(q_file);
    g_free(q_sym);
    g_free(q_root);

    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint status = 0;
    gboolean ok = g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &status, nullptr);
    std::string out;
    if (ok && status == 0 && out_buf != nullptr) {
        out = out_buf;
    } else if (err_buf != nullptr && *err_buf != '\0') {
        append_terminal_text(s, std::string("[lsp bridge] ") + err_buf + "\n");
    }
    g_free(out_buf);
    g_free(err_buf);
    return out;
}

void show_simple_text_dialog(GtkWindow* parent, const char* title, const std::string& text) {
    GtkWidget* d = gtk_dialog_new_with_buttons(title, parent, GTK_DIALOG_MODAL, "_Close", GTK_RESPONSE_CLOSE, nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(scroll, 680, 320);
    GtkWidget* tv = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    GtkTextBuffer* b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(b, text.c_str(), -1);
    gtk_container_add(GTK_CONTAINER(scroll), tv);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 6);
    gtk_widget_show_all(d);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}
std::string lsp_bridge_request_with_timeout(AppState* s, EditorTab* tab, const std::string& method, int timeout_ms, bool* timed_out) {
    if (timed_out != nullptr) {
        *timed_out = false;
    }
    if (method != "completion" || timeout_ms <= 0) {
        return lsp_bridge_request(s, tab, method);
    }
    auto fut = std::async(std::launch::async, [s, tab, method]() { return lsp_bridge_request(s, tab, method); });
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::ready) {
        return fut.get();
    }
    if (timed_out != nullptr) {
        *timed_out = true;
    }
    return "";
}

std::vector<std::string> split_lines_trimmed(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        line = trim_copy(line);
        if (!line.empty()) {
            out.push_back(line);
        }
    }
    return out;
}

std::string highlight_match(const std::string& label, const std::string& query) {
    const std::string q = lower_copy(trim_copy(query));
    if (q.empty()) {
        return label;
    }
    const std::string l = lower_copy(label);
    const std::size_t pos = l.find(q);
    if (pos == std::string::npos) {
        return label;
    }
    return label.substr(0, pos) + "[" + label.substr(pos, q.size()) + "]" + label.substr(pos + q.size());
}

double completion_score(AppState* s,
                        const std::string& query,
                        const std::string& label,
                        const std::string& source,
                        const std::string& current_file) {
    double score = fuzzy_prefix_score(query, label);
    if (!current_file.empty()) {
        const std::string stem = fs::path(current_file).stem().string();
        if (!stem.empty() && lower_copy(label).find(lower_copy(stem)) != std::string::npos) {
            score += 0.8;
        }
    }
    auto it = s->completion_usage_freq.find(normalize_suggestion_key(label));
    if (it != s->completion_usage_freq.end()) {
        score += std::min(3.0, static_cast<double>(it->second) * 0.15);
    }
    if (source == "lsp") {
        score += 0.6;
    } else if (source == "plugin") {
        score += 0.4;
    } else if (source == "local") {
        score += 0.2;
    }
    return score;
}

void append_completion_candidate(AppState* s,
                                 std::unordered_map<std::string, CompletionSuggestion>* by_key,
                                 const std::string& query,
                                 const std::string& label,
                                 const std::string& source,
                                 const std::string& current_file) {
    if (s == nullptr || by_key == nullptr || label.empty()) {
        return;
    }
    CompletionSuggestion cand;
    cand.label = label;
    cand.norm_key = normalize_suggestion_key(label);
    if (cand.norm_key.empty()) {
        return;
    }
    cand.source = source;
    cand.type = infer_completion_type(label);
    cand.file = current_file;
    cand.score = completion_score(s, query, label, source, current_file);
    auto it = by_key->find(cand.norm_key);
    if (it == by_key->end() || cand.score > it->second.score) {
        (*by_key)[cand.norm_key] = cand;
    }
}

std::vector<CompletionSuggestion> collect_completion_suggestions(AppState* s, EditorTab* tab, bool* lsp_timed_out) {
    std::vector<CompletionSuggestion> out;
    if (s == nullptr || tab == nullptr) {
        return out;
    }
    if (lsp_timed_out != nullptr) {
        *lsp_timed_out = false;
    }
    int line = 1;
    int col = 1;
    current_cursor_position(tab, &line, &col);
    const std::string query = current_symbol_under_cursor(tab);
    const std::string cache_key = completion_cache_key(tab->path, line, query);
    auto cache_it = s->completion_cache.find(cache_key);
    if (cache_it != s->completion_cache.end()) {
        const auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - cache_it->second.ts)
                                .count();
        if (age_ms <= s->completion_cache_ttl_ms) {
            return cache_it->second.items;
        }
    }

    std::unordered_map<std::string, CompletionSuggestion> by_key;

    bool timed_out = false;
    const std::string lsp_res = lsp_bridge_request_with_timeout(s, tab, "completion", s->lsp_completion_timeout_ms, &timed_out);
    if (timed_out && lsp_timed_out != nullptr) {
        *lsp_timed_out = true;
    }
    for (const std::string& item : split_lines_trimmed(lsp_res)) {
        append_completion_candidate(s, &by_key, query, item, "lsp", tab->path);
    }

    for (const auto& provider : s->completion_providers) {
        for (const std::string& item : provider(tab->path, line, col)) {
            append_completion_candidate(s, &by_key, query, item, "local", tab->path);
        }
    }

    std::string plugin_result;
    if (run_plugin_command_by_name(s, "vitte-analyzer.completion", plugin_cursor_payload(tab), &plugin_result)) {
        for (const std::string& item : split_csv_trimmed(plugin_result)) {
            append_completion_candidate(s, &by_key, query, item, "plugin", tab->path);
        }
    }

    out.reserve(by_key.size());
    for (auto& kv : by_key) {
        out.push_back(std::move(kv.second));
    }
    std::sort(out.begin(), out.end(), [](const CompletionSuggestion& a, const CompletionSuggestion& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.label < b.label;
    });
    if (out.size() > static_cast<std::size_t>(s->completion_max_suggestions)) {
        out.resize(static_cast<std::size_t>(s->completion_max_suggestions));
    }
    s->completion_cache[cache_key] = CompletionCacheEntry{std::chrono::steady_clock::now(), out};
    return out;
}

void completion_dialog_fill_page(CompletionDialogState* st) {
    if (st == nullptr || st->store == nullptr || st->app == nullptr) {
        return;
    }
    const std::size_t page = static_cast<std::size_t>(std::max(1, st->app->completion_page_size));
    const std::size_t end = std::min(st->all.size(), st->shown + page);
    for (std::size_t i = st->shown; i < end; ++i) {
        const CompletionSuggestion& s = st->all[i];
        GtkTreeIter it;
        gtk_list_store_append(st->store, &it);
        const char* icon = s.type == "function" ? "system-run" : (s.type == "module" ? "folder-open" : "text-x-generic");
        const std::string shown_label = highlight_match(s.label, st->query);
        gtk_list_store_set(st->store,
                           &it,
                           COMP_ICON,
                           icon,
                           COMP_LABEL,
                           shown_label.c_str(),
                           COMP_TYPE,
                           s.type.c_str(),
                           COMP_SOURCE,
                           s.source.c_str(),
                           -1);
    }
    st->shown = end;
    if (st->info != nullptr) {
        std::ostringstream info;
        info << "Showing " << st->shown << " / " << st->all.size();
        if (!st->query.empty()) {
            info << " for \"" << st->query << "\"";
        }
        gtk_label_set_text(GTK_LABEL(st->info), info.str().c_str());
    }
    if (st->load_more_btn != nullptr) {
        gtk_widget_set_sensitive(st->load_more_btn, st->shown < st->all.size());
    }
}

void on_completion_load_more(GtkWidget*, gpointer user_data) {
    auto* st = static_cast<CompletionDialogState*>(user_data);
    completion_dialog_fill_page(st);
}

void on_completion_row_activated(GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn*, gpointer user_data) {
    auto* st = static_cast<CompletionDialogState*>(user_data);
    if (st == nullptr || st->app == nullptr) {
        return;
    }
    GtkTreeModel* model = gtk_tree_view_get_model(view);
    GtkTreeIter it;
    if (!gtk_tree_model_get_iter(model, &it, path)) {
        return;
    }
    char* label = nullptr;
    gtk_tree_model_get(model, &it, COMP_LABEL, &label, -1);
    if (label != nullptr) {
        st->app->completion_usage_freq[normalize_suggestion_key(label)] += 1;
        g_free(label);
    }
}

void show_completion_dialog(AppState* s, EditorTab* tab, std::vector<CompletionSuggestion> items, bool lsp_timed_out, double collect_ms) {
    if (s == nullptr || tab == nullptr) {
        return;
    }
    GtkWidget* d = gtk_dialog_new_with_buttons("Completion", GTK_WINDOW(s->window), GTK_DIALOG_MODAL, "_Close", GTK_RESPONSE_CLOSE, nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* info = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(box), info, FALSE, FALSE, 6);
    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(scroll, 760, 420);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 6);

    GtkListStore* store = gtk_list_store_new(NUM_COMP_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer* icon_r = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer* txt_r = gtk_cell_renderer_text_new();
    GtkCellRenderer* type_r = gtk_cell_renderer_text_new();
    GtkCellRenderer* src_r = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), gtk_tree_view_column_new_with_attributes("", icon_r, "icon-name", COMP_ICON, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), gtk_tree_view_column_new_with_attributes("Suggestion", txt_r, "text", COMP_LABEL, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), gtk_tree_view_column_new_with_attributes("Type", type_r, "text", COMP_TYPE, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), gtk_tree_view_column_new_with_attributes("Source", src_r, "text", COMP_SOURCE, nullptr));
    gtk_container_add(GTK_CONTAINER(scroll), view);

    GtkWidget* load_more = gtk_button_new_with_label("Load more");
    gtk_box_pack_start(GTK_BOX(box), load_more, FALSE, FALSE, 6);

    CompletionDialogState st;
    st.app = s;
    st.dialog = d;
    st.view = view;
    st.store = store;
    st.info = info;
    st.load_more_btn = load_more;
    st.all = std::move(items);
    st.query = current_symbol_under_cursor(tab);
    const auto r0 = std::chrono::steady_clock::now();
    completion_dialog_fill_page(&st);
    const auto r1 = std::chrono::steady_clock::now();
    const double render_ms = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(r1 - r0).count());
    g_signal_connect(load_more, "clicked", G_CALLBACK(on_completion_load_more), &st);
    g_signal_connect(view, "row-activated", G_CALLBACK(on_completion_row_activated), &st);

    if (lsp_timed_out) {
        set_status(s, "Completion: LSP timeout, showing local fallback");
    }
    record_telemetry(s,
                     "completion",
                     "collect_ms=" + std::to_string(static_cast<int>(collect_ms)) + ",count=" + std::to_string(st.all.size()) +
                         ",render_ms=" + std::to_string(static_cast<int>(render_ms)) +
                         ",p50=" + std::to_string(static_cast<int>(percentile(s->completion_latency_samples_ms, 0.5))) +
                         ",p95=" + std::to_string(static_cast<int>(percentile(s->completion_latency_samples_ms, 0.95))));

    gtk_widget_show_all(d);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

bool run_plugin_command_by_name(AppState* s, const std::string& command, const std::string& payload, std::string* out) {
    if (s == nullptr) {
        return false;
    }
    auto it = s->plugin_commands.find(command);
    if (it == s->plugin_commands.end()) {
        return false;
    }
    if (out != nullptr) {
        *out = it->second(payload);
    } else {
        (void)it->second(payload);
    }
    return true;
}

std::string plugin_cursor_payload(EditorTab* tab) {
    if (tab == nullptr) {
        return "";
    }
    int line = 1;
    int col = 1;
    current_cursor_position(tab, &line, &col);
    const std::string sym = current_symbol_under_cursor(tab);
    return "file=" + tab->path + ";line=" + std::to_string(line) + ";col=" + std::to_string(col) + ";symbol=" + sym;
}

std::string value_after_key_line(const std::string& text, const std::string& key) {
    std::istringstream in(text);
    std::string line;
    const std::string prefix = key + "=";
    while (std::getline(in, line)) {
        if (line.rfind(prefix, 0) == 0) {
            return line.substr(prefix.size());
        }
    }
    return "";
}

int fill_problems_from_plugin_output(AppState* s, const std::string& output, const std::string& default_sev) {
    if (s == nullptr) {
        return 0;
    }
    std::istringstream in(output);
    std::string row;
    int hits = 0;
    while (std::getline(in, row)) {
        if (row.empty()) {
            continue;
        }
        const std::size_t c1 = row.find(':');
        const std::size_t c2 = c1 == std::string::npos ? std::string::npos : row.find(':', c1 + 1);
        const std::size_t c3 = c2 == std::string::npos ? std::string::npos : row.find(':', c2 + 1);
        const std::size_t c4 = c3 == std::string::npos ? std::string::npos : row.find(':', c3 + 1);
        if (c1 == std::string::npos || c2 == std::string::npos || c3 == std::string::npos || c4 == std::string::npos) {
            continue;
        }
        const std::string file = row.substr(0, c1);
        const int line = std::max(1, std::atoi(row.substr(c1 + 1, c2 - c1 - 1).c_str()));
        const int col = std::max(1, std::atoi(row.substr(c2 + 1, c3 - c2 - 1).c_str()));
        std::string sev = trim_copy(row.substr(c3 + 1, c4 - c3 - 1));
        if (sev.empty()) {
            sev = default_sev;
        }
        const std::string msg = row.substr(c4 + 1);
        add_problem(s, file, line, col, sev, msg);
        ++hits;
    }
    return hits;
}

void show_tools_output(AppState* s, const std::string& title, const std::string& text) {
    if (s == nullptr || s->tools_buffer == nullptr) {
        return;
    }
    std::string out = "# " + title + "\n\n" + text;
    gtk_text_buffer_set_text(s->tools_buffer, out.c_str(), -1);
    if (s->left_tabs != nullptr) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(s->left_tabs), 2);
    }
}

void on_action_lsp_completion(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    const auto t0 = std::chrono::steady_clock::now();
    bool lsp_timed_out = false;
    std::vector<CompletionSuggestion> items = collect_completion_suggestions(s, tab, &lsp_timed_out);
    const auto t1 = std::chrono::steady_clock::now();
    const double collect_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
    push_completion_latency_sample(s, collect_ms);
    if (items.empty()) {
        show_simple_text_dialog(GTK_WINDOW(s->window),
                                "Completion",
                                lsp_timed_out ? "LSP timeout and no fallback result." : "No completion result.");
        return;
    }
    show_completion_dialog(s, tab, std::move(items), lsp_timed_out, collect_ms);
}

void on_action_lsp_hover(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    std::string result = lsp_bridge_request(s, tab, "hover");
    if (result.empty()) {
        std::string plugin_result;
        if (run_plugin_command_by_name(s, "vitte-analyzer.hover", plugin_cursor_payload(tab), &plugin_result) && !plugin_result.empty()) {
            result = plugin_result;
        }
    }
    if (result.empty()) {
        const std::string sym = current_symbol_under_cursor(tab);
        result = sym.empty() ? "No symbol under cursor" : ("Hover fallback: " + sym);
    }
    show_simple_text_dialog(GTK_WINDOW(s->window), "Hover", result);
}

void on_action_lsp_signature(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    std::string result = lsp_bridge_request(s, tab, "signature");
    if (result.empty()) {
        std::string plugin_result;
        if (run_plugin_command_by_name(s, "vitte-analyzer.signature", plugin_cursor_payload(tab), &plugin_result) && !plugin_result.empty()) {
            result = plugin_result;
        }
    }
    if (result.empty()) {
        result = "Signature help unavailable.";
    }
    show_simple_text_dialog(GTK_WINDOW(s->window), "Signature", result);
}

std::vector<SymbolDef> build_global_symbol_index(const AppState* s) {
    std::vector<SymbolDef> out;
    for_each_workspace_source_file(s, [&](const fs::path& p) {
        std::ifstream in(p);
        if (!in.is_open()) {
            return;
        }

        std::stringstream ss;
        ss << in.rdbuf();
        std::vector<SymbolDef> part = extract_symbols_from_text(ss.str(), p.string());
        out.insert(out.end(), part.begin(), part.end());
    });
    return out;
}

void on_action_go_to_definition(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }

    const std::string sym = current_symbol_under_cursor(tab);
    if (sym.empty()) {
        set_status(s, "Go to definition: no symbol under cursor");
        return;
    }

    std::string plugin_goto;
    if (run_plugin_command_by_name(s, "vitte-analyzer.goto", plugin_cursor_payload(tab), &plugin_goto) &&
        !plugin_goto.empty() &&
        plugin_goto.rfind("goto:", 0) != 0) {
        const std::size_t c1 = plugin_goto.find(':');
        const std::size_t c2 = c1 == std::string::npos ? std::string::npos : plugin_goto.find(':', c1 + 1);
        const std::size_t c3 = c2 == std::string::npos ? std::string::npos : plugin_goto.find(':', c2 + 1);
        const std::size_t c4 = c3 == std::string::npos ? std::string::npos : plugin_goto.find(':', c3 + 1);
        if (c1 != std::string::npos && c2 != std::string::npos && c3 != std::string::npos) {
            const std::string mode = plugin_goto.substr(0, c1);
            const std::string file = plugin_goto.substr(c1 + 1, c2 - c1 - 1);
            const int line = std::max(1, std::atoi(plugin_goto.substr(c2 + 1, c3 - c2 - 1).c_str()));
            const int col = (c4 == std::string::npos)
                                ? 1
                                : std::max(1, std::atoi(plugin_goto.substr(c3 + 1, c4 - c3 - 1).c_str()));
            std::string confidence = "n/a";
            const std::size_t pc = plugin_goto.find("confidence=");
            if (pc != std::string::npos) {
                const std::size_t pe = plugin_goto.find(':', pc);
                confidence = plugin_goto.substr(pc + 11, pe == std::string::npos ? std::string::npos : pe - (pc + 11));
            }
            EditorTab* dst = open_file_in_active_pane(s, file);
            jump_to_line(dst, line, col);
            set_status(s, "Definition(" + mode + ") " + sym + " confidence=" + confidence);
            return;
        }
    }

    std::vector<SymbolDef> idx = build_global_symbol_index(s);
    auto it = std::find_if(idx.begin(), idx.end(), [&sym](const SymbolDef& x) { return x.name == sym; });
    if (it == idx.end()) {
        set_status(s, "Definition not found: " + sym);
        return;
    }

    EditorTab* dst = open_file_in_active_pane(s, it->file);
    jump_to_line(dst, it->line, it->col);
    set_status(s, "Definition: " + sym);
}

void on_action_find_references(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }

    const std::string sym = current_symbol_under_cursor(tab);
    if (sym.empty()) {
        set_status(s, "Find references: no symbol under cursor");
        return;
    }

    clear_problems(s);

    std::string plugin_refs;
    if (run_plugin_command_by_name(s, "vitte-analyzer.refs", plugin_cursor_payload(tab), &plugin_refs) &&
        !plugin_refs.empty() &&
        plugin_refs != "refs: none" &&
        plugin_refs != "refs: no symbol") {
        std::istringstream in(plugin_refs);
        std::string row;
        int hits = 0;
        while (std::getline(in, row)) {
            if (row.empty()) {
                continue;
            }
            const std::size_t c1 = row.find(':');
            const std::size_t c2 = c1 == std::string::npos ? std::string::npos : row.find(':', c1 + 1);
            const std::size_t c3 = c2 == std::string::npos ? std::string::npos : row.find(':', c2 + 1);
            if (c1 == std::string::npos || c2 == std::string::npos || c3 == std::string::npos) {
                continue;
            }
            const std::string file = row.substr(0, c1);
            const int line = std::max(1, std::atoi(row.substr(c1 + 1, c2 - c1 - 1).c_str()));
            const int col = std::max(1, std::atoi(row.substr(c2 + 1, c3 - c2 - 1).c_str()));
            const std::string text = row.substr(c3 + 1);
            add_problem(s, file, line, col, "info", text);
            ++hits;
        }
        set_status(s, "References (analyzer): " + sym + " => " + std::to_string(hits));
        return;
    }

    const std::regex rx("(^|[^A-Za-z0-9_])" + escape_regex_literal(sym) + "([^A-Za-z0-9_]|$)");
    int hits = 0;

    for_each_workspace_source_file(s, [&](const fs::path& p) {
        std::ifstream in(p);
        if (!in.is_open()) {
            return;
        }

        std::string line;
        int ln = 0;
        while (std::getline(in, line)) {
            ++ln;
            std::smatch m;
            if (std::regex_search(line, m, rx)) {
                const int col = static_cast<int>(m.position()) + 1;
                add_problem(s, to_project_relative(s, p.string()), ln, col, "info", line);
                ++hits;
            }
        }
    });

    set_status(s, "References: " + sym + " => " + std::to_string(hits));
}

void palette_fill(PaletteState* p, const std::string& query) {
    gtk_list_store_clear(p->store);
    std::size_t emitted = 0;
    const std::size_t max_items = static_cast<std::size_t>(std::max(50, p->app->palette_max_suggestions));

    if (!query.empty() && query[0] == ':') {
        if (EditorTab* tab = current_tab(p->app); tab != nullptr) {
            int line = std::atoi(query.c_str() + 1);
            if (line <= 0) {
                line = 1;
            }
            GtkTreeIter it;
            gtk_list_store_append(p->store, &it);
            gtk_list_store_set(p->store,
                               &it,
                               PAL_LABEL,
                               (std::string("Go to line ") + std::to_string(line)).c_str(),
                               PAL_FILE,
                               tab->path.c_str(),
                               PAL_LINE,
                               line,
                               PAL_COL,
                               1,
                               -1);
        }
        return;
    }

    if (!query.empty() && query[0] == '@') {
        const std::string term = query.substr(1);
        refresh_outline(p->app);
        for (const SymbolDef& sym : p->app->current_symbols) {
            if (emitted >= max_items) {
                break;
            }
            const std::string label = sym.kind + " " + sym.name + " (" + std::to_string(sym.line) + ")";
            if (!fuzzy_match(term, label)) {
                continue;
            }
            GtkTreeIter it;
            gtk_list_store_append(p->store, &it);
            gtk_list_store_set(p->store,
                               &it,
                               PAL_LABEL,
                               label.c_str(),
                               PAL_FILE,
                               sym.file.c_str(),
                               PAL_LINE,
                               sym.line,
                               PAL_COL,
                               sym.col,
                               -1);
            ++emitted;
        }
        return;
    }

    for_each_workspace_source_file(p->app, [&](const fs::path& path) {
        if (emitted >= max_items) {
            return;
        }
        const std::string rel = to_project_relative(p->app, path.string());
        if (!fuzzy_match(query, rel)) {
            return;
        }

        GtkTreeIter it;
        gtk_list_store_append(p->store, &it);
        gtk_list_store_set(p->store,
                           &it,
                           PAL_LABEL,
                           rel.c_str(),
                           PAL_FILE,
                           path.string().c_str(),
                           PAL_LINE,
                           1,
                           PAL_COL,
                           1,
                           -1);
        ++emitted;
    });
}

void palette_open_selected(PaletteState* p, bool preview_only) {
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(p->view));
    GtkTreeModel* model = nullptr;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        return;
    }

    char* file = nullptr;
    int line = 1;
    int col = 1;
    gtk_tree_model_get(model, &iter, PAL_FILE, &file, PAL_LINE, &line, PAL_COL, &col, -1);
    if (file == nullptr) {
        return;
    }

    EditorTab* tab = open_file_in_active_pane(p->app, file);
    jump_to_line(tab, line, col);
    g_free(file);

    if (!preview_only) {
        gtk_dialog_response(GTK_DIALOG(p->dialog), GTK_RESPONSE_ACCEPT);
    }
}

void on_palette_entry_changed(GtkEditable* editable, gpointer user_data) {
    auto* p = static_cast<PaletteState*>(user_data);
    const std::string q = gtk_entry_get_text(GTK_ENTRY(editable));
    palette_fill(p, q);
}

void on_palette_row_activated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer user_data) {
    auto* p = static_cast<PaletteState*>(user_data);
    palette_open_selected(p, false);
}

void on_palette_cursor_changed(GtkTreeView*, gpointer user_data) {
    auto* p = static_cast<PaletteState*>(user_data);
    palette_open_selected(p, true);
}

void on_action_palette(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);

    PaletteState p;
    p.app = s;
    p.dialog = gtk_dialog_new_with_buttons("Quick Open (Ctrl+P)",
                                            GTK_WINDOW(s->window),
                                            GTK_DIALOG_MODAL,
                                            "_Close",
                                            GTK_RESPONSE_CANCEL,
                                            nullptr);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(p.dialog));
    p.entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(p.entry), "fuzzy file | @symbol | :line");
    gtk_box_pack_start(GTK_BOX(box), p.entry, FALSE, FALSE, 6);

    p.store = gtk_list_store_new(NUM_PAL_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    p.view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(p.store));
    GtkCellRenderer* r = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(p.view), gtk_tree_view_column_new_with_attributes("Result", r, "text", PAL_LABEL, nullptr));

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(scroll, 780, 420);
    gtk_container_add(GTK_CONTAINER(scroll), p.view);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 6);

    g_signal_connect(p.entry, "changed", G_CALLBACK(on_palette_entry_changed), &p);
    g_signal_connect(p.view, "row-activated", G_CALLBACK(on_palette_row_activated), &p);
    g_signal_connect(p.view, "cursor-changed", G_CALLBACK(on_palette_cursor_changed), &p);

    gtk_widget_show_all(p.dialog);

    palette_fill(&p, "");
    gtk_widget_grab_focus(p.entry);

    gtk_dialog_run(GTK_DIALOG(p.dialog));
    gtk_widget_destroy(p.dialog);
    g_object_unref(p.store);
}

void on_action_outline_jump(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    refresh_outline(s);

    GtkWidget* d = gtk_dialog_new_with_buttons("Outline",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Close",
                                                GTK_RESPONSE_CANCEL,
                                                "_Jump",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->outline_store));
    GtkCellRenderer* r1 = gtk_cell_renderer_text_new();
    GtkCellRenderer* r2 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), gtk_tree_view_column_new_with_attributes("Kind", r1, "text", OUT_KIND, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), gtk_tree_view_column_new_with_attributes("Symbol", r2, "text", OUT_NAME, nullptr));

    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(scroll, 560, 320);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 6);
    gtk_widget_show_all(d);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
        GtkTreeModel* model = nullptr;
        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
            int ln = 1;
            gtk_tree_model_get(model, &iter, OUT_LINE, &ln, -1);
            if (EditorTab* tab = current_tab(s); tab != nullptr) {
                jump_to_line(tab, ln, 1);
            }
        }
    }

    gtk_widget_destroy(d);
}

void on_problems_filter_changed(GtkComboBoxText* combo, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const gchar* t = gtk_combo_box_text_get_active_text(combo);
    std::string val = t != nullptr ? t : "All";
    if (val == "Errors") {
        s->problem_filter = ProblemFilter::ErrorsOnly;
    } else if (val == "Current File") {
        s->problem_filter = ProblemFilter::CurrentFile;
    } else {
        s->problem_filter = ProblemFilter::All;
    }
    refresh_problems_view(s);
}

std::string session_path(const AppState* s) {
    fs::path p = fs::path(s->root) / ".vitte-cache" / "vitte-ide-gtk" / "session.txt";
    fs::create_directories(p.parent_path());
    return p.string();
}

std::string recovery_dir(const AppState* s) {
    fs::path p = fs::path(primary_workspace_root(s)) / ".vitte-cache" / "vitte-ide-gtk" / "recovery";
    fs::create_directories(p);
    return p.string();
}

std::string recovery_index_path(const AppState* s) {
    fs::path p = fs::path(recovery_dir(s)) / "recovery.index";
    return p.string();
}

std::string recovery_file_path_for_tab(const AppState* s, const EditorTab* tab) {
    const std::size_t h = std::hash<std::string>{}(tab->path);
    fs::path p = fs::path(recovery_dir(s)) / (std::to_string(h) + ".recovery");
    return p.string();
}

int autosave_interval_for_tab(const AppState* s, const EditorTab* tab) {
    if (s == nullptr || tab == nullptr || !s->autosave_enabled) {
        return -1;
    }
    const std::string key = to_project_relative(s, tab->path);
    auto it = s->autosave_seconds_by_file.find(key);
    if (it == s->autosave_seconds_by_file.end()) {
        return std::max(5, s->autosave_seconds);
    }
    if (it->second < 0) {
        return -1;
    }
    if (it->second == 0) {
        return std::max(5, s->autosave_seconds);
    }
    return std::max(5, it->second);
}

void write_recovery_snapshot(AppState* s) {
    if (s == nullptr) {
        return;
    }
    std::ofstream idx(recovery_index_path(s));
    if (!idx.is_open()) {
        return;
    }
    auto dump_tabs = [&](const std::vector<EditorTab*>& tabs, const char* pane) {
        for (EditorTab* tab : tabs) {
            if (tab == nullptr || tab->buffer == nullptr || !tab->dirty) {
                continue;
            }
            const std::string rec_file = recovery_file_path_for_tab(s, tab);
            std::ofstream out(rec_file);
            if (!out.is_open()) {
                continue;
            }
            out << get_buffer_text(tab->buffer);
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_mark(tab->buffer, &iter, gtk_text_buffer_get_insert(tab->buffer));
            idx << pane << "|" << tab->path << "|" << (gtk_text_iter_get_line(&iter) + 1) << "|"
                << (gtk_text_iter_get_line_offset(&iter) + 1) << "|" << rec_file << "\n";
        }
    };
    dump_tabs(s->tabs_primary, "primary");
    dump_tabs(s->tabs_secondary, "secondary");
}

void clear_recovery_snapshot(AppState* s) {
    if (s == nullptr) {
        return;
    }
    fs::path dir = recovery_dir(s);
    if (fs::exists(dir)) {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
}

void restore_recovery_snapshot(AppState* s) {
    std::ifstream idx(recovery_index_path(s));
    if (!idx.is_open()) {
        return;
    }
    GtkWidget* q = gtk_message_dialog_new(GTK_WINDOW(s->window),
                                          GTK_DIALOG_MODAL,
                                          GTK_MESSAGE_QUESTION,
                                          GTK_BUTTONS_YES_NO,
                                          "Unsaved recovery data found.\nRestore unsaved buffers?");
    const int rc = gtk_dialog_run(GTK_DIALOG(q));
    gtk_widget_destroy(q);
    if (rc != GTK_RESPONSE_YES) {
        clear_recovery_snapshot(s);
        return;
    }

    std::string line;
    while (std::getline(idx, line)) {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string tok;
        while (std::getline(ss, tok, '|')) {
            parts.push_back(tok);
        }
        if (parts.size() < 5) {
            continue;
        }
        std::ifstream in(parts[4]);
        if (!in.is_open()) {
            continue;
        }
        std::stringstream content;
        content << in.rdbuf();
        GtkWidget* pane = parts[0] == "secondary" ? s->notebook_secondary : s->notebook_primary;
        EditorTab* tab = open_file_in_notebook(s, parts[1], pane, false);
        if (tab == nullptr || tab->buffer == nullptr) {
            continue;
        }
        gtk_text_buffer_set_text(tab->buffer, content.str().c_str(), -1);
        tab->dirty = true;
        update_tab_label(tab);
        jump_to_line(tab, std::atoi(parts[2].c_str()), std::atoi(parts[3].c_str()));
    }
    set_status(s, "Recovery restored");
}

void save_session(AppState* s) {
    std::ofstream out(session_path(s));
    if (!out.is_open()) {
        return;
    }

    out << "root=" << s->root << "\n";
    out << "split_enabled=" << (s->split_enabled ? 1 : 0) << "\n";
    out << "split_orientation=" << (s->split_orientation == GTK_ORIENTATION_HORIZONTAL ? "h" : "v") << "\n";
    out << "autosave_enabled=" << (s->autosave_enabled ? 1 : 0) << "\n";
    out << "autosave_seconds=" << s->autosave_seconds << "\n";
    out << "build_profile=" << s->build_profile << "\n";
    out << "build_extra_debug=" << s->build_extra_debug << "\n";
    out << "build_extra_release=" << s->build_extra_release << "\n";
    out << "run_args=" << s->run_args << "\n";
    out << "run_env=" << s->run_env << "\n";
    out << "theme_mode=" << s->theme_mode << "\n";
    out << "window_start_mode=" << s->window_start_mode << "\n";
    out << "font_family=" << s->font_family << "\n";
    out << "font_size=" << s->font_size << "\n";
    out << "high_contrast=" << (s->high_contrast ? 1 : 0) << "\n";
    out << "compact_mode=" << (s->compact_mode ? 1 : 0) << "\n";
    out << "ui_zoom_percent=" << s->ui_zoom_percent << "\n";
    out << "editor_tab_width=" << s->editor_tab_width << "\n";
    out << "bottom_panel_visible=" << (s->bottom_panel_visible ? 1 : 0) << "\n";
    out << "lsp_bridge_enabled=" << (s->lsp_bridge_enabled ? 1 : 0) << "\n";
    out << "lsp_bridge_cmd=" << s->lsp_bridge_cmd << "\n";
    out << "formatter_on_save=" << (s->formatter_on_save ? 1 : 0) << "\n";
    out << "formatter_cmd=" << s->formatter_cmd << "\n";
    out << "perf_mode_enabled=" << (s->perf_mode_enabled ? 1 : 0) << "\n";
    out << "keymap_mode=" << s->keymap_mode << "\n";
    out << "telemetry_opt_in=" << (s->telemetry_opt_in ? 1 : 0) << "\n";
    out << "completion_max_suggestions=" << s->completion_max_suggestions << "\n";
    out << "palette_max_suggestions=" << s->palette_max_suggestions << "\n";
    out << "completion_page_size=" << s->completion_page_size << "\n";
    out << "completion_cache_ttl_ms=" << s->completion_cache_ttl_ms << "\n";
    out << "lsp_completion_timeout_ms=" << s->lsp_completion_timeout_ms << "\n";
    for (const std::string& wr : s->workspace_roots) {
        out << "workspace_root=" << wr << "\n";
    }
    for (const auto& [path, secs] : s->autosave_seconds_by_file) {
        out << "autosave_file=" << path << "|" << secs << "\n";
    }
    for (const auto& [path, preset] : s->file_presets) {
        out << "file_preset "
            << std::quoted(path) << " "
            << std::quoted(preset.debug_extra) << " "
            << std::quoted(preset.release_extra) << " "
            << std::quoted(preset.run_args) << " "
            << std::quoted(preset.run_env) << "\n";
    }

    auto dump_tabs = [&](const std::vector<EditorTab*>& tabs, const char* pane) {
        for (EditorTab* tab : tabs) {
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_mark(tab->buffer, &iter, gtk_text_buffer_get_insert(tab->buffer));
            out << "tab|" << pane << "|" << tab->path << "|" << (gtk_text_iter_get_line(&iter) + 1) << "|"
                << (gtk_text_iter_get_line_offset(&iter) + 1) << "\n";
        }
    };

    dump_tabs(s->tabs_primary, "primary");
    dump_tabs(s->tabs_secondary, "secondary");
    out << "active=" << (s->active_notebook == s->notebook_secondary ? "secondary" : "primary") << "\n";
}

void start_autosave_timer(AppState* s);

void load_session(AppState* s) {
    std::ifstream in(session_path(s));
    if (!in.is_open()) {
        return;
    }

    s->workspace_roots.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("split_enabled=", 0) == 0) {
            s->split_enabled = (line.substr(14) == "1");
        } else if (line.rfind("split_orientation=", 0) == 0) {
            s->split_orientation = (line.substr(18) == "v") ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
        } else if (line.rfind("autosave_enabled=", 0) == 0) {
            s->autosave_enabled = (line.substr(17) == "1");
        } else if (line.rfind("autosave_seconds=", 0) == 0) {
            s->autosave_seconds = std::max(5, std::atoi(line.substr(17).c_str()));
        } else if (line.rfind("build_profile=", 0) == 0) {
            const std::string v = line.substr(14);
            s->build_profile = (v == "Release") ? "Release" : "Debug";
        } else if (line.rfind("build_extra_debug=", 0) == 0) {
            s->build_extra_debug = line.substr(18);
        } else if (line.rfind("build_extra_release=", 0) == 0) {
            s->build_extra_release = line.substr(20);
        } else if (line.rfind("run_args=", 0) == 0) {
            s->run_args = line.substr(9);
        } else if (line.rfind("run_env=", 0) == 0) {
            s->run_env = line.substr(8);
        } else if (line.rfind("theme_mode=", 0) == 0) {
            const std::string v = line.substr(11);
            s->theme_mode = (v == "Dark" || v == "Light") ? v : "System";
        } else if (line.rfind("window_start_mode=", 0) == 0) {
            const std::string v = line.substr(18);
            s->window_start_mode = (v == "maximized") ? "maximized" : "auto";
        } else if (line.rfind("font_family=", 0) == 0) {
            s->font_family = line.substr(12);
            if (s->font_family.empty()) {
                s->font_family = "Monospace";
            }
        } else if (line.rfind("font_size=", 0) == 0) {
            s->font_size = std::max(8, std::atoi(line.substr(10).c_str()));
        } else if (line.rfind("high_contrast=", 0) == 0) {
            s->high_contrast = (line.substr(14) == "1");
        } else if (line.rfind("compact_mode=", 0) == 0) {
            s->compact_mode = (line.substr(13) == "1");
        } else if (line.rfind("ui_zoom_percent=", 0) == 0) {
            s->ui_zoom_percent = std::max(70, std::min(200, std::atoi(line.substr(16).c_str())));
        } else if (line.rfind("editor_tab_width=", 0) == 0) {
            s->editor_tab_width = std::max(1, std::atoi(line.substr(17).c_str()));
        } else if (line.rfind("bottom_panel_visible=", 0) == 0) {
            s->bottom_panel_visible = (line.substr(21) == "1");
        } else if (line.rfind("lsp_bridge_enabled=", 0) == 0) {
            s->lsp_bridge_enabled = (line.substr(19) == "1");
        } else if (line.rfind("lsp_bridge_cmd=", 0) == 0) {
            s->lsp_bridge_cmd = line.substr(15);
        } else if (line.rfind("formatter_on_save=", 0) == 0) {
            s->formatter_on_save = (line.substr(18) == "1");
        } else if (line.rfind("formatter_cmd=", 0) == 0) {
            s->formatter_cmd = line.substr(14);
        } else if (line.rfind("perf_mode_enabled=", 0) == 0) {
            s->perf_mode_enabled = (line.substr(18) == "1");
        } else if (line.rfind("keymap_mode=", 0) == 0) {
            const std::string m = line.substr(11);
            s->keymap_mode = (m == "vim" || m == "emacs") ? m : "default";
        } else if (line.rfind("telemetry_opt_in=", 0) == 0) {
            s->telemetry_opt_in = (line.substr(17) == "1");
        } else if (line.rfind("completion_max_suggestions=", 0) == 0) {
            s->completion_max_suggestions = std::max(20, std::atoi(line.substr(27).c_str()));
        } else if (line.rfind("palette_max_suggestions=", 0) == 0) {
            s->palette_max_suggestions = std::max(50, std::atoi(line.substr(24).c_str()));
        } else if (line.rfind("completion_page_size=", 0) == 0) {
            s->completion_page_size = std::max(20, std::atoi(line.substr(21).c_str()));
        } else if (line.rfind("completion_cache_ttl_ms=", 0) == 0) {
            s->completion_cache_ttl_ms = std::max(200, std::atoi(line.substr(24).c_str()));
        } else if (line.rfind("lsp_completion_timeout_ms=", 0) == 0) {
            s->lsp_completion_timeout_ms = std::max(50, std::atoi(line.substr(26).c_str()));
        } else if (line.rfind("workspace_root=", 0) == 0) {
            const std::string wr = line.substr(15);
            if (!wr.empty()) {
                s->workspace_roots.push_back(wr);
            }
        } else if (line.rfind("autosave_file=", 0) == 0) {
            const std::string payload = line.substr(14);
            const std::size_t sep = payload.rfind('|');
            if (sep != std::string::npos) {
                const std::string p = payload.substr(0, sep);
                const int secs = std::atoi(payload.substr(sep + 1).c_str());
                if (!p.empty()) {
                    s->autosave_seconds_by_file[p] = secs;
                }
            }
        } else if (line.rfind("file_preset ", 0) == 0) {
            std::istringstream ls(line);
            std::string key;
            BuildPreset p;
            std::string path;
            ls >> key >> std::quoted(path) >> std::quoted(p.debug_extra) >> std::quoted(p.release_extra) >>
                std::quoted(p.run_args) >> std::quoted(p.run_env);
            if (!path.empty()) {
                s->file_presets[path] = p;
            }
        } else if (line.rfind("tab|", 0) == 0) {
            std::vector<std::string> parts;
            std::stringstream ss(line);
            std::string tok;
            while (std::getline(ss, tok, '|')) {
                parts.push_back(tok);
            }
            if (parts.size() < 5) {
                continue;
            }

            GtkWidget* pane = parts[1] == "secondary" ? s->notebook_secondary : s->notebook_primary;
            EditorTab* tab = open_file_in_notebook(s, parts[2], pane, false);
            if (tab != nullptr) {
                jump_to_line(tab, std::atoi(parts[3].c_str()), std::atoi(parts[4].c_str()));
            }
        } else if (line.rfind("active=", 0) == 0) {
            if (line.substr(7) == "secondary") {
                s->active_notebook = s->notebook_secondary;
            } else {
                s->active_notebook = s->notebook_primary;
            }
        }
    }
    if (s->workspace_roots.empty()) {
        s->workspace_roots.push_back(s->root);
    }
    s->root = s->workspace_roots.front();

    apply_split_layout(s);
    apply_appearance(s);
    if (s->bottom_panel != nullptr) {
        if (s->bottom_panel_visible) {
            gtk_widget_show(s->bottom_panel);
        } else {
            gtk_widget_hide(s->bottom_panel);
        }
    }
    start_autosave_timer(s);
}

gboolean on_autosave_tick(gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    maybe_hot_reload_config(s);
    auto autosave_tabs = [&](std::vector<EditorTab*>& tabs, std::time_t now) {
        for (EditorTab* tab : tabs) {
            if (tab == nullptr || !tab->dirty) {
                continue;
            }
            const int interval = autosave_interval_for_tab(s, tab);
            if (interval > 0 && (tab->last_autosave_at == 0 || (now - tab->last_autosave_at) >= interval)) {
                save_tab(s, tab);
                tab->last_autosave_at = now;
            }
        }
    };
    const std::time_t now = std::time(nullptr);
    autosave_tabs(s->tabs_primary, now);
    autosave_tabs(s->tabs_secondary, now);
    write_recovery_snapshot(s);
    save_session(s);
    if (s->telemetry_opt_in) {
        record_telemetry(s, "tick", "errors=" + std::to_string(s->telemetry_errors));
    }
    return G_SOURCE_CONTINUE;
}

void start_autosave_timer(AppState* s) {
    if (s->autosave_timer != 0) {
        g_source_remove(s->autosave_timer);
        s->autosave_timer = 0;
    }
    s->autosave_timer = g_timeout_add_seconds(1, on_autosave_tick, s);
}

void on_action_autosave_settings(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);

    GtkWidget* d = gtk_dialog_new_with_buttons("Autosave Settings",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* enabled = gtk_check_button_new_with_label("Enable autosave");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enabled), s->autosave_enabled);
    gtk_box_pack_start(GTK_BOX(box), enabled, FALSE, FALSE, 6);

    GtkWidget* spin = gtk_spin_button_new_with_range(5, 600, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), s->autosave_seconds);
    gtk_box_pack_start(GTK_BOX(box), spin, FALSE, FALSE, 6);

    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        s->autosave_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enabled));
        s->autosave_seconds = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        start_autosave_timer(s);
        save_session(s);
        set_status(s, "Autosave settings updated");
    }
    gtk_widget_destroy(d);
}

void on_action_buffer_autosave_settings(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    const std::string key = to_project_relative(s, tab->path);
    const auto it = s->autosave_seconds_by_file.find(key);
    const int current = (it == s->autosave_seconds_by_file.end()) ? 0 : it->second;

    GtkWidget* d = gtk_dialog_new_with_buttons("Buffer Autosave",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* disable = gtk_check_button_new_with_label("Disable autosave for this buffer");
    GtkWidget* use_global = gtk_check_button_new_with_label("Use global autosave interval");
    GtkWidget* spin = gtk_spin_button_new_with_range(5, 600, 5);
    if (current < 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable), TRUE);
    } else if (current == 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_global), TRUE);
    } else {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), current);
    }
    if (current <= 0) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), s->autosave_seconds);
    }

    gtk_box_pack_start(GTK_BOX(box), disable, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), use_global, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), spin, FALSE, FALSE, 4);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(disable))) {
            s->autosave_seconds_by_file[key] = -1;
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_global))) {
            s->autosave_seconds_by_file[key] = 0;
        } else {
            s->autosave_seconds_by_file[key] = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        }
        save_session(s);
        set_status(s, "Buffer autosave updated: " + key);
    }
    gtk_widget_destroy(d);
}

void load_project_tasks(AppState* s) {
    s->project_tasks.clear();
    std::vector<std::string> roots = s->workspace_roots;
    if (roots.empty()) {
        roots.push_back(s->root);
    }
    for (const std::string& root : roots) {
        fs::path p = fs::path(root) / "tasks.vitte.json";
        std::ifstream in(p);
        if (!in.is_open()) {
            continue;
        }
        std::stringstream ss;
        ss << in.rdbuf();
        const std::string txt = ss.str();
        std::regex pair_rx("\"([A-Za-z0-9_\\-.]+)\"\\s*:\\s*\"([^\"]+)\"");
        for (std::sregex_iterator it(txt.begin(), txt.end(), pair_rx), end; it != end; ++it) {
            const std::string name = (*it)[1].str();
            if (name == "tasks") {
                continue;
            }
            const std::string cmd = (*it)[2].str();
            const std::string key = fs::path(root).filename().string() + ":" + name;
            gchar* q_root = g_shell_quote(root.c_str());
            s->project_tasks[key] = "cd " + std::string(q_root) + " && " + cmd;
            g_free(q_root);
        }
    }
}

void on_action_run_task(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string names_csv;
    if (run_plugin_command_by_name(s, "vitte-task-orchestrator.list", "file=" + payload_file, &names_csv) &&
        !names_csv.empty() && names_csv != "tasks: none") {
        GtkWidget* d = gtk_dialog_new_with_buttons("Run Orchestrated Task",
                                                    GTK_WINDOW(s->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    "_Preview",
                                                    GTK_RESPONSE_OK,
                                                    "_Run",
                                                    GTK_RESPONSE_ACCEPT,
                                                    nullptr);
        GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
        GtkWidget* combo = gtk_combo_box_text_new();
        for (const std::string& name : split_csv_trimmed(names_csv)) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), name.c_str());
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
        GtkWidget* details = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(details), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(details), TRUE);
        GtkWidget* details_scroll = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_widget_set_size_request(details_scroll, 780, 240);
        gtk_container_add(GTK_CONTAINER(details_scroll), details);
        gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 6);
        gtk_box_pack_start(GTK_BOX(box), details_scroll, TRUE, TRUE, 6);
        gtk_widget_show_all(d);

        while (true) {
            const int r = gtk_dialog_run(GTK_DIALOG(d));
            if (r == GTK_RESPONSE_CANCEL || r == GTK_RESPONSE_DELETE_EVENT) {
                break;
            }
            gchar* selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
            if (selected == nullptr) {
                continue;
            }
            const std::string task = selected;
            g_free(selected);
            std::string out;
            run_plugin_command_by_name(s, "vitte-task-orchestrator.plan", "file=" + payload_file + ";task=" + task, &out);
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(details)), out.c_str(), -1);
            if (r != GTK_RESPONSE_ACCEPT) {
                set_status(s, "Task plan preview: " + task);
                continue;
            }
            const std::string cmd = value_after_key_line(out, "command");
            if (!cmd.empty()) {
                run_terminal_command(s, cmd);
                set_status(s, "Task launched: " + task);
            } else {
                set_status(s, "Task command not found");
            }
            break;
        }
        gtk_widget_destroy(d);
        return;
    }

    load_project_tasks(s);
    if (s->project_tasks.empty()) {
        set_status(s, "No tasks found (tasks.vitte.json)");
        return;
    }
    GtkWidget* d = gtk_dialog_new_with_buttons("Run Project Task",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Run",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* combo = gtk_combo_box_text_new();
    for (const auto& [name, _] : s->project_tasks) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), name.c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 6);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        gchar* selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
        if (selected != nullptr) {
            auto it = s->project_tasks.find(selected);
            if (it != s->project_tasks.end()) {
                run_terminal_command(s, it->second);
                set_status(s, std::string("Task launched: ") + selected);
            }
            g_free(selected);
        }
    }
    gtk_widget_destroy(d);
}

void on_action_doc_preview(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr || s->docs_buffer == nullptr) {
        return;
    }
    std::string doc;
    if (run_plugin_command_by_name(s, "vitte-docgen.preview", plugin_cursor_payload(tab), &doc) && !doc.empty()) {
        gtk_text_buffer_set_text(s->docs_buffer, doc.c_str(), -1);
        set_status(s, "Doc preview updated");
        return;
    }
    gtk_text_buffer_set_text(s->docs_buffer, "Doc preview unavailable (plugin not loaded).", -1);
    set_status(s, "Doc preview unavailable");
}

void on_action_test_runner(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-test-runner.detect", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Test runner plugin unavailable");
        return;
    }
    const std::string cmd = value_after_key_line(out, "command");
    const std::string mode = value_after_key_line(out, "mode");
    if (cmd.empty()) {
        set_status(s, "Test runner returned no command");
        return;
    }
    run_terminal_command(s, cmd);
    set_status(s, "Test runner launched (" + (mode.empty() ? "auto" : mode) + ")");
}

void on_action_validate_tasks(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/tasks.vitte.json");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-task-orchestrator.validate", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Task validator unavailable");
        return;
    }
    clear_problems(s);
    if (out == "tasks: valid") {
        set_status(s, "tasks.vitte.json valid");
        return;
    }
    if (out == "tasks: none") {
        set_status(s, "No tasks.vitte.json or no tasks");
        return;
    }
    const int issues = fill_problems_from_plugin_output(s, out, "error");
    set_status(s, "Task validation issues: " + std::to_string(issues));
}

void on_action_security_lint(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-security-lint.run", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Security lint unavailable");
        return;
    }
    show_tools_output(s, "Security Lint", out);
    clear_problems(s);
    if (out == "security-lint: clean") {
        set_status(s, "Security lint clean");
        return;
    }
    const int issues = fill_problems_from_plugin_output(s, out, "error");
    set_status(s, "Security lint issues: " + std::to_string(issues));
}

void on_action_style_enforcer(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-style-enforcer.run", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Style enforcer unavailable");
        return;
    }
    show_tools_output(s, "Style Enforcer", out);
    clear_problems(s);
    if (out == "style-enforcer: clean") {
        set_status(s, "Style enforcer clean");
        return;
    }
    const int issues = fill_problems_from_plugin_output(s, out, "warning");
    set_status(s, "Style issues: " + std::to_string(issues));
}

void on_action_memory_snapshot(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-memory-watch.snapshot", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Memory snapshot unavailable");
        return;
    }
    show_tools_output(s, "Memory Snapshot", out);
    set_status(s, "Memory snapshot captured");
}

void on_action_memory_diff(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-memory-watch.diff", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Memory diff unavailable");
        return;
    }
    show_tools_output(s, "Memory Diff", out);
    set_status(s, "Memory diff ready");
}

void on_action_profiler_timeline(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-profiler.timeline", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Profiler timeline unavailable");
        return;
    }
    show_tools_output(s, "Profiler Timeline", out);
    set_status(s, "Profiler timeline ready");
}

void on_action_profiler_hotspots(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-profiler.hotspots", "file=" + payload_file, &out) || out.empty()) {
        set_status(s, "Profiler hotspots unavailable");
        return;
    }
    show_tools_output(s, "Profiler Hotspots", out);
    set_status(s, "Profiler hotspots ready");
}

void on_action_semantic_search(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    run_plugin_command_by_name(s, "vitte-semantic-search.index", "file=" + payload_file, nullptr);

    GtkWidget* d = gtk_dialog_new_with_buttons("Semantic Search",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Search",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "symbol/kind query");
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 6);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const std::string q = trim_copy(gtk_entry_get_text(GTK_ENTRY(entry)));
        if (!q.empty()) {
            std::string out;
            run_plugin_command_by_name(s, "vitte-semantic-search.query", "file=" + payload_file + ";q=" + q, &out);
            show_tools_output(s, "Semantic Search", out);
            clear_problems(s);
            const int hits = fill_problems_from_plugin_output(s, out, "info");
            set_status(s, "Semantic search hits: " + std::to_string(hits));
        }
    }
    gtk_widget_destroy(d);
}

void on_action_git_diff(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    std::string out;
    run_plugin_command_by_name(s, "vitte-git-assistant.diff", "file=" + tab->path, &out);
    show_tools_output(s, "Git Diff", out);
    set_status(s, "Git diff ready");
}

void on_action_git_blame(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    int line = 1;
    int col = 1;
    current_cursor_position(tab, &line, &col);
    std::string out;
    run_plugin_command_by_name(s, "vitte-git-assistant.blame", "file=" + tab->path + ";line=" + std::to_string(line), &out);
    show_tools_output(s, "Git Blame", out);
    set_status(s, "Git blame line " + std::to_string(line));
}

void on_action_git_stage_hunk(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    int line = 1;
    int col = 1;
    current_cursor_position(tab, &line, &col);
    std::string out;
    run_plugin_command_by_name(s, "vitte-git-assistant.stage-hunk", "file=" + tab->path + ";line=" + std::to_string(line), &out);
    show_tools_output(s, "Git Stage Hunk", out);
    set_status(s, out);
}

void on_action_terminal_actions(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr || s->terminal_output_buffer == nullptr) {
        return;
    }
    const fs::path log_path = fs::path(primary_workspace_root(s)) / ".vitte-cache" / "vitte-ide-gtk" / "terminal.actions.log";
    fs::create_directories(log_path.parent_path());
    {
        std::ofstream out(log_path);
        out << get_buffer_text(s->terminal_output_buffer);
    }
    std::string res;
    run_plugin_command_by_name(s, "vitte-terminal-actions.extract", "log_file=" + log_path.string(), &res);
    show_tools_output(s, "Terminal Actions", res);
    clear_problems(s);
    const int hits = fill_problems_from_plugin_output(s, res, "info");
    set_status(s, "Terminal actions: " + std::to_string(hits));
}

void on_action_build_matrix(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/Makefile");
    std::string out;
    run_plugin_command_by_name(s, "vitte-build-matrix.plan", "file=" + payload_file + ";target=check,build,test", &out);
    show_tools_output(s, "Build Matrix", out);
    const std::string cmd = value_after_key_line(out, "command");
    if (!cmd.empty()) {
        run_terminal_command(s, cmd);
        set_status(s, "Build matrix launched");
    } else {
        set_status(s, "Build matrix plan unavailable");
    }
}

void on_action_fix_core_imports(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    save_tab(s, tab);
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-import-fix.apply", "file=" + tab->path, &out)) {
        set_status(s, "Core import fix unavailable");
        return;
    }
    reload_tab_from_disk(tab);
    show_tools_output(s, "Core Import Fix", out);
    set_status(s, "Core import fix applied");
}

std::string run_shell_capture(const std::string& cmd) {
    gchar* out_buf = nullptr;
    gchar* err_buf = nullptr;
    gint status = 0;
    GError* err = nullptr;
    const gboolean ok = g_spawn_command_line_sync(cmd.c_str(), &out_buf, &err_buf, &status, &err);
    std::string out;
    if (!ok) {
        out = std::string("[spawn-error] ") + (err != nullptr ? err->message : "unknown");
    } else {
        if (out_buf != nullptr) {
            out += out_buf;
        }
        if (err_buf != nullptr && *err_buf != '\0') {
            out += err_buf;
        }
        out += "[exit " + std::to_string(status) + "]\n";
    }
    if (err != nullptr) {
        g_error_free(err);
    }
    g_free(out_buf);
    g_free(err_buf);
    return out;
}

std::string read_file_text(const fs::path& p) {
    std::ifstream in(p);
    if (!in.is_open()) {
        return "";
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

std::string first_failing_report_from_release_doctor_json(const std::string& text) {
    const std::string key = "\"failing_reports\"";
    const std::size_t k = text.find(key);
    if (k == std::string::npos) {
        return "";
    }
    const std::size_t lb = text.find('[', k);
    if (lb == std::string::npos) {
        return "";
    }
    const std::size_t q1 = text.find('"', lb);
    if (q1 == std::string::npos) {
        return "";
    }
    const std::size_t q2 = text.find('"', q1 + 1);
    if (q2 == std::string::npos || q2 <= q1 + 1) {
        return "";
    }
    return text.substr(q1 + 1, q2 - q1 - 1);
}

void on_action_core_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const fs::path root = primary_workspace_root(s);
    std::ostringstream report;
    report << "Core API Doctor\n\n";
    const std::vector<std::pair<std::string, std::string>> checks = {
        {"core-mod-lint", "cd " + root.string() + " && python3 tools/lint_core_mod_contracts.py"},
        {"core-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_core_no_internal_exports.py"},
        {"core-contract-snapshots", "cd " + root.string() + " && tools/core_contract_snapshots.sh"},
        {"core-facade-snapshot", "cd " + root.string() + " && tools/core_facade_snapshot.sh"},
        {"core-compat-contracts", "cd " + root.string() + " && python3 tools/lint_core_compat_contracts.py"},
    };
    for (const auto& [name, cmd] : checks) {
        report << "## " << name << "\n";
        report << run_shell_capture(cmd) << "\n";
    }
    show_tools_output(s, "Core API Doctor", report.str());
    set_status(s, "Core API Doctor updated");
}

void on_action_std_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/std/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-std.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "Std API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"std-mod-lint", "cd " + root.string() + " && python3 tools/lint_std_mod_contracts.py"},
            {"std-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_std_no_internal_exports.py"},
            {"std-contract-snapshots", "cd " + root.string() + " && tools/std_contract_snapshots.sh"},
            {"std-facade-snapshot", "cd " + root.string() + " && tools/std_facade_snapshot.sh"},
            {"std-compat-contracts", "cd " + root.string() + " && python3 tools/lint_std_compat_contracts.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "Std API Doctor", out);
    set_status(s, "Std API Doctor updated");
}

void on_action_log_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/log/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-log.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "Log API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"log-mod-lint", "cd " + root.string() + " && python3 tools/lint_log_mod_contracts.py"},
            {"log-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_log_no_internal_exports.py"},
            {"log-contract-snapshots", "cd " + root.string() + " && tools/log_contract_snapshots.sh"},
            {"log-facade-snapshot", "cd " + root.string() + " && tools/log_facade_snapshot.sh"},
            {"log-compat-contracts", "cd " + root.string() + " && python3 tools/lint_log_compat_contracts.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "Log API Doctor", out);
    set_status(s, "Log API Doctor updated");
}

bool run_quickfix_with_preview(AppState* s,
                               EditorTab* tab,
                               const std::string& preview_cmd,
                               const std::string& apply_cmd,
                               const std::string& title,
                               const std::string& unavailable_status) {
    if (tab == nullptr) {
        return false;
    }
    save_tab(s, tab);
    std::string preview;
    run_plugin_command_by_name(s, preview_cmd, "file=" + tab->path, &preview);
    if (!preview.empty()) {
        show_tools_output(s, title + " Preview", preview);
    }
    std::string out;
    if (!run_plugin_command_by_name(s, apply_cmd, "file=" + tab->path, &out)) {
        set_status(s, unavailable_status);
        return false;
    }
    reload_tab_from_disk(tab);
    show_tools_output(s, title, out);
    set_status(s, out);
    return true;
}

void on_action_log_quickfix_config(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    run_quickfix_with_preview(s,
                              tab,
                              "vitte-log.quickfix-config.preview",
                              "vitte-log.quickfix-config.apply",
                              "Log Config Quick-fix",
                              "Log config quick-fix unavailable");
}

void on_action_fs_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/fs/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-fs.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "FS API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"fs-mod-lint", "cd " + root.string() + " && python3 tools/lint_fs_mod_contracts.py"},
            {"fs-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_fs_no_internal_exports.py"},
            {"fs-contract-snapshots", "cd " + root.string() + " && tools/fs_contract_snapshots.sh"},
            {"fs-facade-snapshot", "cd " + root.string() + " && tools/fs_facade_snapshot.sh"},
            {"fs-compat-contracts", "cd " + root.string() + " && python3 tools/lint_fs_compat_contracts.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "FS API Doctor", out);
    set_status(s, "FS API Doctor updated");
}

void on_action_db_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/db/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-db.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "DB API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"db-mod-lint", "cd " + root.string() + " && python3 tools/lint_db_mod_contracts.py"},
            {"db-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_db_no_internal_exports.py"},
            {"db-contract-snapshots", "cd " + root.string() + " && tools/db_contract_snapshots.sh"},
            {"db-facade-snapshot", "cd " + root.string() + " && tools/db_facade_snapshot.sh"},
            {"db-compat-contracts", "cd " + root.string() + " && python3 tools/lint_db_compat_contracts.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "DB API Doctor", out);
    set_status(s, "DB API Doctor updated");
}

void on_action_db_quickfix_config(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    run_quickfix_with_preview(s,
                              tab,
                              "vitte-db.quickfix-config.preview",
                              "vitte-db.quickfix-config.apply",
                              "DB Config Quick-fix",
                              "DB config quick-fix unavailable");
}

void on_action_http_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/http/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-http.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "HTTP API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"http-mod-lint", "cd " + root.string() + " && python3 tools/lint_http_mod_contracts.py"},
            {"http-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_http_no_internal_exports.py"},
            {"http-contract-snapshots", "cd " + root.string() + " && tools/http_contract_snapshots.sh"},
            {"http-facade-snapshot", "cd " + root.string() + " && tools/http_facade_snapshot.sh"},
            {"http-compat-contracts", "cd " + root.string() + " && python3 tools/lint_http_compat_contracts.py"},
            {"http-security", "cd " + root.string() + " && python3 tools/lint_http_security.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "HTTP API Doctor", out);
    set_status(s, "HTTP API Doctor updated");
}

void on_action_http_client_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/http_client/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-http-client.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "HTTP Client API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"http-client-mod-lint", "cd " + root.string() + " && python3 tools/lint_http_client_mod_contracts.py"},
            {"http-client-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_http_no_internal_exports.py"},
            {"http-client-contract-snapshots", "cd " + root.string() + " && tools/http_client_contract_snapshots.sh"},
            {"http-client-facade-snapshot", "cd " + root.string() + " && tools/http_client_facade_snapshot.sh"},
            {"http-client-compat-contracts", "cd " + root.string() + " && python3 tools/lint_http_compat_contracts.py"},
            {"http-client-security", "cd " + root.string() + " && python3 tools/lint_http_security.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "HTTP Client API Doctor", out);
    set_status(s, "HTTP Client API Doctor updated");
}

void on_action_http_quickfix_config(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    run_quickfix_with_preview(s,
                              tab,
                              "vitte-http.quickfix-config.preview",
                              "vitte-http.quickfix-config.apply",
                              "HTTP Config Quick-fix",
                              "HTTP config quick-fix unavailable");
}

void on_action_http_client_quickfix_config(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    run_quickfix_with_preview(s,
                              tab,
                              "vitte-http-client.quickfix-config.preview",
                              "vitte-http-client.quickfix-config.apply",
                              "HTTP Client Config Quick-fix",
                              "HTTP client config quick-fix unavailable");
}

void on_action_process_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/process/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-process.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "Process API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"process-mod-lint", "cd " + root.string() + " && python3 tools/lint_process_mod_contracts.py"},
            {"process-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_process_no_internal_exports.py"},
            {"process-contract-snapshots", "cd " + root.string() + " && tools/process_contract_snapshots.sh"},
            {"process-facade-snapshot", "cd " + root.string() + " && tools/process_facade_snapshot.sh"},
            {"process-compat-contracts", "cd " + root.string() + " && python3 tools/lint_process_compat_contracts.py"},
            {"process-security", "cd " + root.string() + " && python3 tools/lint_process_security.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "Process API Doctor", out);
    set_status(s, "Process API Doctor updated");
}

void on_action_process_quickfix_config(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    run_quickfix_with_preview(s,
                              tab,
                              "vitte-process.quickfix-config.preview",
                              "vitte-process.quickfix-config.apply",
                              "Process Config Quick-fix",
                              "Process config quick-fix unavailable");
}

void on_action_test_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/test/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-test.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "Test API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"test-mod-lint", "cd " + root.string() + " && python3 tools/lint_test_mod_contracts.py"},
            {"test-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_test_no_internal_exports.py"},
            {"test-contract-snapshots", "cd " + root.string() + " && tools/test_contract_snapshots.sh"},
            {"test-facade-snapshot", "cd " + root.string() + " && tools/test_facade_snapshot.sh"},
            {"test-compat-contracts", "cd " + root.string() + " && python3 tools/lint_test_compat_contracts.py"},
            {"test-security", "cd " + root.string() + " && python3 tools/lint_test_security.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "Test API Doctor", out);
    set_status(s, "Test API Doctor updated");
}

void on_action_lint_api_doctor(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const std::string payload_file = (tab != nullptr) ? tab->path : (s->root + "/src/vitte/packages/lint/mod.vit");
    std::string out;
    if (!run_plugin_command_by_name(s, "vitte-lint.doctor", "file=" + payload_file, &out) || out.empty()) {
        const fs::path root = primary_workspace_root(s);
        std::ostringstream report;
        report << "Lint API Doctor\n\n";
        const std::vector<std::pair<std::string, std::string>> checks = {
            {"lint-mod-lint", "cd " + root.string() + " && python3 tools/lint_lint_mod_contracts.py"},
            {"lint-no-internal-exports", "cd " + root.string() + " && python3 tools/lint_lint_no_internal_exports.py"},
            {"lint-contract-snapshots", "cd " + root.string() + " && tools/lint_contract_snapshots_pkg.sh"},
            {"lint-facade-snapshot", "cd " + root.string() + " && tools/lint_facade_snapshot_pkg.sh"},
            {"lint-compat-contracts", "cd " + root.string() + " && python3 tools/lint_lint_compat_contracts.py"},
            {"lint-security", "cd " + root.string() + " && python3 tools/lint_lint_security.py"},
        };
        for (const auto& [name, cmd] : checks) {
            report << "## " << name << "\n";
            report << run_shell_capture(cmd) << "\n";
        }
        out = report.str();
    }
    show_tools_output(s, "Lint API Doctor", out);
    set_status(s, "Lint API Doctor updated");
}

void on_action_lint_quickfix_config(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    run_quickfix_with_preview(s,
                              tab,
                              "vitte-lint.quickfix-config.preview",
                              "vitte-lint.quickfix-config.apply",
                              "Lint Quick-fix",
                              "Lint quick-fix unavailable");
}

void on_action_api_contracts_panel(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const fs::path root = primary_workspace_root(s);
    const std::string cmd = "cd " + root.string() + " && make -s contracts-refresh diagnostics-index";
    std::string report = run_shell_capture(cmd);
    const fs::path idx = root / "target/reports/diagnostics_index.md";
    if (fs::exists(idx)) {
        report += "\n";
        report += read_file_text(idx);
    }
    show_tools_output(s, "API Contracts", report);
    set_status(s, "API Contracts updated");
}

void on_action_perf_doctor_panel(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const fs::path root = primary_workspace_root(s);
    const std::string cmd = "cd " + root.string() + " && make -s perf-regression";
    std::string report = run_shell_capture(cmd);
    show_tools_output(s, "Perf Doctor", report);
    set_status(s, "Perf Doctor updated");
}

void on_action_security_doctor_panel(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const fs::path root = primary_workspace_root(s);
    const std::string cmd = "cd " + root.string() + " && make -s security-gates-report security-hardening-gate";
    std::string report = run_shell_capture(cmd);
    const fs::path sec = root / "target/reports/security_gates.report";
    const fs::path hard = root / "target/reports/security_hardening.json";
    if (fs::exists(sec)) {
        report += "\n";
        report += read_file_text(sec);
    }
    if (fs::exists(hard)) {
        report += "\n";
        report += read_file_text(hard);
    }
    show_tools_output(s, "Security Doctor", report);
    set_status(s, "Security Doctor updated");
}

void on_action_release_doctor_panel(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const fs::path root = primary_workspace_root(s);
    const std::string cmd = "cd " + root.string() + " && make -s release-doctor";
    std::string report = run_shell_capture(cmd);
    const fs::path json_path = root / "target/reports/release_doctor.json";
    const fs::path md_path = root / "target/reports/release_doctor.md";
    if (fs::exists(json_path)) {
        const std::string j = read_file_text(json_path);
        s->release_doctor_last_failing_report = first_failing_report_from_release_doctor_json(j);
    } else {
        s->release_doctor_last_failing_report.clear();
    }
    if (fs::exists(md_path)) {
        report += "\n";
        report += read_file_text(md_path);
    }
    show_tools_output(s, "Release Doctor", report);
    set_status(s, "Release Doctor updated");
}

void on_action_open_failing_report(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    const fs::path root = primary_workspace_root(s);
    std::string rel = s->release_doctor_last_failing_report;
    if (rel.empty()) {
        const fs::path json_path = root / "target/reports/release_doctor.json";
        if (fs::exists(json_path)) {
            rel = first_failing_report_from_release_doctor_json(read_file_text(json_path));
        }
    }
    if (rel.empty()) {
        set_status(s, "Release Doctor: no failing report");
        return;
    }
    const fs::path p = root / rel;
    if (!fs::exists(p)) {
        set_status(s, "Failing report not found: " + p.string());
        return;
    }
    open_file_in_active_pane(s, p.string());
    set_status(s, "Opened failing report: " + rel);
}

void on_action_workspace_settings(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkWidget* d = gtk_dialog_new_with_buttons("Workspace Roots (one path per line)",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(scroll, 700, 260);
    GtkWidget* tv = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    GtkTextBuffer* b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    std::ostringstream roots_txt;
    std::vector<std::string> roots = s->workspace_roots;
    if (roots.empty()) {
        roots.push_back(s->root);
    }
    for (std::size_t i = 0; i < roots.size(); ++i) {
        roots_txt << roots[i];
        if (i + 1 < roots.size()) {
            roots_txt << '\n';
        }
    }
    gtk_text_buffer_set_text(b, roots_txt.str().c_str(), -1);
    gtk_container_add(GTK_CONTAINER(scroll), tv);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 6);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        GtkTextIter st;
        GtkTextIter en;
        gtk_text_buffer_get_start_iter(b, &st);
        gtk_text_buffer_get_end_iter(b, &en);
        char* txt = gtk_text_buffer_get_text(b, &st, &en, FALSE);
        std::stringstream ss(txt != nullptr ? txt : "");
        std::string line;
        std::vector<std::string> updated;
        while (std::getline(ss, line)) {
            line = trim_copy(line);
            if (!line.empty() && fs::exists(line)) {
                updated.push_back(fs::absolute(line).string());
            }
        }
        g_free(txt);
        if (!updated.empty()) {
            s->workspace_roots = updated;
            s->root = s->workspace_roots.front();
            build_project_tree(s);
            rebuild_search_index_async(s);
            load_project_tasks(s);
            save_session(s);
            set_status(s, "Workspace updated: " + std::to_string(s->workspace_roots.size()) + " root(s)");
        }
    }
    gtk_widget_destroy(d);
}

void add_debug_list_row(GtkListStore* store, const std::vector<std::string>& cols) {
    if (store == nullptr) {
        return;
    }
    GtkTreeIter it;
    gtk_list_store_append(store, &it);
    for (std::size_t i = 0; i < cols.size(); ++i) {
        gtk_list_store_set(store, &it, static_cast<int>(i), cols[i].c_str(), -1);
    }
}

void on_action_debug_add_breakpoint(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    int ln = 1;
    int col = 1;
    current_cursor_position(tab, &ln, &col);
    (void)col;
    add_debug_list_row(s->debug_breakpoints_store, {to_project_relative(s, tab->path), std::to_string(ln)});
    set_status(s, "Breakpoint added");
}

void on_action_debug_add_watch(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr || s->debug_watches_entry == nullptr) {
        return;
    }
    const std::string expr = trim_copy(gtk_entry_get_text(GTK_ENTRY(s->debug_watches_entry)));
    if (expr.empty()) {
        return;
    }
    add_debug_list_row(s->debug_watches_store, {expr, "?"});
    gtk_entry_set_text(GTK_ENTRY(s->debug_watches_entry), "");
}

void on_action_debug_refresh(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    gtk_list_store_clear(s->debug_callstack_store);
    gtk_list_store_clear(s->debug_vars_store);
    add_debug_list_row(s->debug_callstack_store, {"main()"});
    if (EditorTab* tab = current_tab(s); tab != nullptr) {
        int ln = 1;
        int col = 1;
        current_cursor_position(tab, &ln, &col);
        add_debug_list_row(s->debug_callstack_store, {fs::path(tab->path).filename().string() + ":" + std::to_string(ln)});
        add_debug_list_row(s->debug_vars_store, {"cursor_line", std::to_string(ln)});
        add_debug_list_row(s->debug_vars_store, {"cursor_col", std::to_string(col)});
    }
    GtkTreeModel* model = GTK_TREE_MODEL(s->debug_watches_store);
    GtkTreeIter it;
    gboolean ok = gtk_tree_model_get_iter_first(model, &it);
    while (ok) {
        gchar* expr = nullptr;
        gtk_tree_model_get(model, &it, DBG_VAR_NAME, &expr, -1);
        if (expr != nullptr) {
            gtk_list_store_set(s->debug_watches_store, &it, DBG_VAR_VALUE, "<pending>", -1);
            g_free(expr);
        }
        ok = gtk_tree_model_iter_next(model, &it);
    }
    set_status(s, "Debug views refreshed");
}

void on_action_ui_settings(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkWidget* d = gtk_dialog_new_with_buttons("IDE Appearance",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* theme_label = gtk_label_new("Theme");
    GtkWidget* theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "System");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Dark");
    if (s->theme_mode == "Light") {
        gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 1);
    } else if (s->theme_mode == "Dark") {
        gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 2);
    } else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), 0);
    }

    GtkWidget* font_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(font_entry), "Font family");
    gtk_entry_set_text(GTK_ENTRY(font_entry), s->font_family.c_str());
    GtkWidget* start_label = gtk_label_new("Window start");
    GtkWidget* start_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(start_combo), "auto");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(start_combo), "maximized");
    gtk_combo_box_set_active(GTK_COMBO_BOX(start_combo), s->window_start_mode == "maximized" ? 1 : 0);

    GtkWidget* font_size = gtk_spin_button_new_with_range(8, 30, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(font_size), s->font_size);
    GtkWidget* zoom_label = gtk_label_new("UI zoom (%)");
    GtkWidget* zoom_size = gtk_spin_button_new_with_range(70, 200, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(zoom_size), s->ui_zoom_percent);
    GtkWidget* contrast = gtk_check_button_new_with_label("High contrast");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(contrast), s->high_contrast);
    GtkWidget* compact = gtk_check_button_new_with_label("Compact mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compact), s->compact_mode);
    GtkWidget* tab_width = gtk_spin_button_new_with_range(1, 12, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tab_width), s->editor_tab_width);

    gtk_box_pack_start(GTK_BOX(box), theme_label, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), theme_combo, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), start_label, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), start_combo, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), font_entry, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), font_size, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), zoom_label, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), zoom_size, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), contrast, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), compact, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), tab_width, FALSE, FALSE, 4);
    gtk_widget_show_all(d);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const gchar* mode = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(theme_combo));
        s->theme_mode = mode != nullptr ? mode : "System";
        const gchar* start_mode = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(start_combo));
        s->window_start_mode = (start_mode != nullptr && std::string(start_mode) == "maximized") ? "maximized" : "auto";
        s->font_family = gtk_entry_get_text(GTK_ENTRY(font_entry));
        s->font_size = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(font_size));
        s->ui_zoom_percent = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(zoom_size));
        s->high_contrast = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(contrast));
        s->compact_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compact));
        s->editor_tab_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tab_width));
        apply_window_start_mode(s);
        apply_appearance(s);
        save_session(s);
        set_status(s, "Appearance updated");
    }
    gtk_widget_destroy(d);
}

void on_toggle_bottom_panel(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s == nullptr || s->bottom_panel == nullptr) {
        return;
    }
    s->bottom_panel_visible = !s->bottom_panel_visible;
    if (s->bottom_panel_visible) {
        gtk_widget_show(s->bottom_panel);
    } else {
        gtk_widget_hide(s->bottom_panel);
    }
}

std::string replace_identifier_in_line(const std::string& line,
                                       const std::string& old_name,
                                       const std::string& new_name,
                                       int* replacements) {
    std::string out;
    out.reserve(line.size());
    int local = 0;
    std::size_t i = 0;
    while (i < line.size()) {
        bool match = false;
        if (i + old_name.size() <= line.size() && line.compare(i, old_name.size(), old_name) == 0) {
            const bool left_ok = (i == 0) || !is_ident_char(line[i - 1]);
            const bool right_ok = (i + old_name.size() == line.size()) || !is_ident_char(line[i + old_name.size()]);
            match = left_ok && right_ok;
        }

        if (match) {
            out += new_name;
            i += old_name.size();
            ++local;
        } else {
            out.push_back(line[i]);
            ++i;
        }
    }
    if (replacements != nullptr) {
        *replacements += local;
    }
    return out;
}

std::vector<RenameHit> collect_rename_preview(const AppState* s,
                                              const std::string& old_name,
                                              const std::string& new_name,
                                              bool apply_changes,
                                              int* files_changed,
                                              int* total_hits) {
    std::vector<RenameHit> preview;
    int changed = 0;
    int hits = 0;

    for_each_workspace_source_file(s, [&](const fs::path& p) {
        std::ifstream in(p);
        if (!in.is_open()) {
            return;
        }

        std::vector<std::string> original_lines;
        std::vector<std::string> replaced_lines;
        std::string line;
        int ln = 0;
        bool file_touched = false;
        while (std::getline(in, line)) {
            ++ln;
            original_lines.push_back(line);
            int line_hits = 0;
            std::string replaced = replace_identifier_in_line(line, old_name, new_name, &line_hits);
            if (line_hits > 0) {
                file_touched = true;
                hits += line_hits;
                RenameHit hit;
                hit.file = to_project_relative(s, p.string());
                hit.line = ln;
                hit.before = line;
                hit.after = replaced;
                preview.push_back(hit);
            }
            replaced_lines.push_back(replaced);
        }

        if (file_touched) {
            ++changed;
            if (apply_changes) {
                std::ofstream out(p);
                for (std::size_t i = 0; i < replaced_lines.size(); ++i) {
                    out << replaced_lines[i];
                    if (i + 1 < replaced_lines.size()) {
                        out << '\n';
                    }
                }
                out << '\n';
            }
        }
    });

    if (files_changed != nullptr) {
        *files_changed = changed;
    }
    if (total_hits != nullptr) {
        *total_hits = hits;
    }
    return preview;
}

void on_action_rename_symbol(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }

    const std::string initial_old = current_symbol_under_cursor(tab);
    GtkWidget* d = gtk_dialog_new_with_buttons("Rename Symbol",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Preview",
                                                GTK_RESPONSE_OK,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* old_entry = gtk_entry_new();
    GtkWidget* new_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(old_entry), "Old symbol");
    gtk_entry_set_placeholder_text(GTK_ENTRY(new_entry), "New symbol");
    if (!initial_old.empty()) {
        gtk_entry_set_text(GTK_ENTRY(old_entry), initial_old.c_str());
    }
    gtk_box_pack_start(GTK_BOX(box), old_entry, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(box), new_entry, FALSE, FALSE, 6);

    GtkWidget* preview_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(preview_scroll, 860, 380);
    GtkWidget* preview_text = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(preview_text), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(preview_text), FALSE);
    gtk_container_add(GTK_CONTAINER(preview_scroll), preview_text);
    gtk_box_pack_start(GTK_BOX(box), preview_scroll, TRUE, TRUE, 6);
    gtk_widget_show_all(d);

    while (true) {
        const int resp = gtk_dialog_run(GTK_DIALOG(d));
        if (resp == GTK_RESPONSE_CANCEL || resp == GTK_RESPONSE_DELETE_EVENT) {
            break;
        }

        const std::string old_name = trim_copy(gtk_entry_get_text(GTK_ENTRY(old_entry)));
        const std::string new_name = trim_copy(gtk_entry_get_text(GTK_ENTRY(new_entry)));
        if (old_name.empty() || new_name.empty() || old_name == new_name) {
            set_status(s, "Rename invalid: provide old/new symbol names");
            continue;
        }

        const bool apply = (resp == GTK_RESPONSE_ACCEPT);
        int cur_line = 1;
        int cur_col = 1;
        current_cursor_position(tab, &cur_line, &cur_col);
        std::string plugin_out;
        const std::string payload = "file=" + tab->path + ";line=" + std::to_string(cur_line) + ";col=" +
                                    std::to_string(cur_col) + ";old=" + old_name + ";new=" + new_name;
        bool plugin_ok = false;
        if (apply) {
            plugin_ok = run_plugin_command_by_name(s, "vitte-refactor.rename.apply", payload, &plugin_out);
        } else {
            plugin_ok = run_plugin_command_by_name(s, "vitte-refactor.rename.preview", payload, &plugin_out);
        }

        if (plugin_ok) {
            GtkTextBuffer* pb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(preview_text));
            gtk_text_buffer_set_text(pb, plugin_out.c_str(), -1);
            if (!apply) {
                set_status(s, "Rename preview ready (plugin)");
                continue;
            }

            clear_problems(s);
            std::istringstream in(plugin_out);
            std::string ln;
            while (std::getline(in, ln)) {
                const std::size_t c = ln.find(':');
                if (c == std::string::npos || c == 0) {
                    continue;
                }
                bool line_ok = true;
                for (std::size_t i = c + 1; i < ln.size(); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(ln[i]))) {
                        line_ok = false;
                        break;
                    }
                }
                if (!line_ok) {
                    continue;
                }
                const std::string f = ln.substr(0, c);
                const int l = std::max(1, std::atoi(ln.substr(c + 1).c_str()));
                add_problem(s, f, l, 1, "info", "rename: " + old_name + " -> " + new_name);
            }

            for (EditorTab* t : s->tabs_primary) {
                reload_tab_from_disk(t);
            }
            for (EditorTab* t : s->tabs_secondary) {
                reload_tab_from_disk(t);
            }
            set_status(s, "Rename applied (plugin)");
            break;
        }

        int files_changed = 0;
        int total_hits = 0;
        std::vector<RenameHit> hits = collect_rename_preview(s, old_name, new_name, apply, &files_changed, &total_hits);
        std::ostringstream preview;
        preview << "rename " << old_name << " -> " << new_name << "\n";
        preview << "files: " << files_changed << ", hits: " << total_hits << "\n\n";
        const std::size_t max_lines = 240;
        for (std::size_t i = 0; i < hits.size() && i < max_lines; ++i) {
            const RenameHit& h = hits[i];
            preview << h.file << ":" << h.line << "\n";
            preview << "- " << h.before << "\n";
            preview << "+ " << h.after << "\n\n";
        }
        GtkTextBuffer* pb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(preview_text));
        gtk_text_buffer_set_text(pb, preview.str().c_str(), -1);
        if (!apply) {
            set_status(s, "Rename preview ready (fallback)");
            continue;
        }
        for (EditorTab* t : s->tabs_primary) {
            reload_tab_from_disk(t);
        }
        for (EditorTab* t : s->tabs_secondary) {
            reload_tab_from_disk(t);
        }
        set_status(s, "Rename applied (fallback)");
        break;
    }

    gtk_widget_destroy(d);
}

void on_action_profile_settings(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    BuildPreset effective = effective_preset_for_tab(s, tab);

    GtkWidget* d = gtk_dialog_new_with_buttons("Build/Run Profiles",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);

    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* profile_label = gtk_label_new("Build profile");
    GtkWidget* profile_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(profile_combo), "Debug");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(profile_combo), "Release");
    gtk_combo_box_set_active(GTK_COMBO_BOX(profile_combo), s->build_profile == "Release" ? 1 : 0);

    GtkWidget* debug_extra = gtk_entry_new();
    GtkWidget* release_extra = gtk_entry_new();
    GtkWidget* args_entry = gtk_entry_new();
    GtkWidget* env_entry = gtk_entry_new();
    GtkWidget* current_file_only = gtk_check_button_new_with_label("Apply to current file only");
    gtk_entry_set_placeholder_text(GTK_ENTRY(debug_extra), "Debug extra build flags (optional)");
    gtk_entry_set_placeholder_text(GTK_ENTRY(release_extra), "Release extra build flags (optional)");
    gtk_entry_set_placeholder_text(GTK_ENTRY(args_entry), "Run args preset (example: --port 8080 --verbose)");
    gtk_entry_set_placeholder_text(GTK_ENTRY(env_entry), "Run env preset (example: A=1 B=2)");
    gtk_entry_set_text(GTK_ENTRY(debug_extra), effective.debug_extra.c_str());
    gtk_entry_set_text(GTK_ENTRY(release_extra), effective.release_extra.c_str());
    gtk_entry_set_text(GTK_ENTRY(args_entry), effective.run_args.c_str());
    gtk_entry_set_text(GTK_ENTRY(env_entry), effective.run_env.c_str());
    gtk_widget_set_sensitive(current_file_only, tab != nullptr);
    if (tab == nullptr) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(current_file_only), FALSE);
    }

    gtk_box_pack_start(GTK_BOX(box), profile_label, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), profile_combo, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), debug_extra, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), release_extra, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), args_entry, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), env_entry, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), current_file_only, FALSE, FALSE, 4);
    gtk_widget_show_all(d);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const gchar* profile_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(profile_combo));
        s->build_profile = (profile_text != nullptr && std::string(profile_text) == "Release") ? "Release" : "Debug";
        const bool per_file = tab != nullptr && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(current_file_only));
        if (per_file) {
            BuildPreset p;
            p.debug_extra = gtk_entry_get_text(GTK_ENTRY(debug_extra));
            p.release_extra = gtk_entry_get_text(GTK_ENTRY(release_extra));
            p.run_args = gtk_entry_get_text(GTK_ENTRY(args_entry));
            p.run_env = gtk_entry_get_text(GTK_ENTRY(env_entry));
            s->file_presets[current_tab_key(s, tab)] = p;
        } else {
            s->build_extra_debug = gtk_entry_get_text(GTK_ENTRY(debug_extra));
            s->build_extra_release = gtk_entry_get_text(GTK_ENTRY(release_extra));
            s->run_args = gtk_entry_get_text(GTK_ENTRY(args_entry));
            s->run_env = gtk_entry_get_text(GTK_ENTRY(env_entry));
        }
        save_session(s);
        set_status(s, "Profiles updated (" + s->build_profile + ")");
    }

    gtk_widget_destroy(d);
}

void insert_text_at_cursor(EditorTab* tab, const std::string& text) {
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }
    GtkTextIter it;
    gtk_text_buffer_get_iter_at_mark(tab->buffer, &it, gtk_text_buffer_get_insert(tab->buffer));
    gtk_text_buffer_insert(tab->buffer, &it, text.c_str(), -1);
    tab->dirty = true;
    update_tab_label(tab);
}

void on_action_insert_snippet(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr) {
        return;
    }
    GtkWidget* d = gtk_dialog_new_with_buttons("Insert Vitte Snippet",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Insert",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* combo = gtk_combo_box_text_new();
    std::vector<std::string> names = {"entry main", "use module alias", "proc template", "trait template"};
    std::string list_out;
    if (run_plugin_command_by_name(s, "vitte-notebook.list", "file=" + tab->path, &list_out) && !list_out.empty()) {
        std::vector<std::string> from_plugin = split_csv_trimmed(list_out);
        if (!from_plugin.empty()) {
            names = from_plugin;
        }
    }
    for (const std::string& n : names) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), n.c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 6);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const int idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        std::string text;
        if (idx >= 0 && static_cast<std::size_t>(idx) < names.size()) {
            run_plugin_command_by_name(s, "vitte-notebook.expand", "file=" + tab->path + ";name=" + names[static_cast<std::size_t>(idx)], &text);
        }
        if (text.empty()) {
            if (idx == 1) {
                text = "use vitte/core as core_pkg\n";
            } else if (idx == 2) {
                text = "proc name(args: int) -> int {\n  give args\n}\n";
            } else if (idx == 3) {
                text = "trait Name {\n  proc run(self: Name) -> int\n}\n";
            } else {
                text = "entry main at core/app {\n  give 0\n}\n";
            }
        }
        insert_text_at_cursor(tab, text);
        set_status(s, "Snippet inserted");
    }
    gtk_widget_destroy(d);
}

void on_action_code_actions(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    if (tab == nullptr || tab->buffer == nullptr) {
        return;
    }

    std::string txt = get_buffer_text(tab->buffer);
    std::vector<std::string> actions;
    actions.push_back("Normalize trailing spaces + final newline");
    if (txt.find("core_pkg") != std::string::npos && txt.find("use vitte/core as core_pkg") == std::string::npos) {
        actions.push_back("Add missing import: use vitte/core as core_pkg");
    }
    if (txt.find("entry main") == std::string::npos) {
        actions.push_back("Insert entry main template");
    }

    GtkWidget* d = gtk_dialog_new_with_buttons("Code Actions",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* combo = gtk_combo_box_text_new();
    for (const std::string& a : actions) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), a.c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 6);
    gtk_widget_show_all(d);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        const int idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        if (idx >= 0 && static_cast<std::size_t>(idx) < actions.size()) {
            const std::string picked = actions[static_cast<std::size_t>(idx)];
            if (picked.find("Normalize") != std::string::npos) {
                format_buffer_basic(tab);
            } else if (picked.find("Add missing import") != std::string::npos) {
                GtkTextIter it;
                gtk_text_buffer_get_start_iter(tab->buffer, &it);
                gtk_text_buffer_insert(tab->buffer, &it, "use vitte/core as core_pkg\n", -1);
            } else if (picked.find("Insert entry main") != std::string::npos) {
                insert_text_at_cursor(tab, "\nentry main at core/app {\n  give 0\n}\n");
            }
            tab->dirty = true;
            update_tab_label(tab);
            set_status(s, "Code action applied");
        }
    }
    gtk_widget_destroy(d);
}

void on_action_language_settings(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    GtkWidget* d = gtk_dialog_new_with_buttons("Language / LSP / Formatter",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Apply",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* lsp_enable = gtk_check_button_new_with_label("Enable optional LSP bridge");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lsp_enable), s->lsp_bridge_enabled);
    GtkWidget* lsp_cmd = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(lsp_cmd), "LSP bridge command (supports --completion/--hover/--signature)");
    gtk_entry_set_text(GTK_ENTRY(lsp_cmd), s->lsp_bridge_cmd.c_str());
    GtkWidget* fmt_enable = gtk_check_button_new_with_label("Formatter on save");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fmt_enable), s->formatter_on_save);
    GtkWidget* fmt_cmd = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fmt_cmd), "Formatter command, ex: vitte fmt {file}");
    gtk_entry_set_text(GTK_ENTRY(fmt_cmd), s->formatter_cmd.c_str());
    GtkWidget* perf_enable = gtk_check_button_new_with_label("Perf mode (lazy parse + throttling + async index)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perf_enable), s->perf_mode_enabled);
    GtkWidget* telemetry_enable = gtk_check_button_new_with_label("Telemetry locale opt-in (anonymized perf/errors)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(telemetry_enable), s->telemetry_opt_in);
    GtkWidget* keymap_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(keymap_combo), "default");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(keymap_combo), "vim");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(keymap_combo), "emacs");
    if (s->keymap_mode == "vim") {
        gtk_combo_box_set_active(GTK_COMBO_BOX(keymap_combo), 1);
    } else if (s->keymap_mode == "emacs") {
        gtk_combo_box_set_active(GTK_COMBO_BOX(keymap_combo), 2);
    } else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(keymap_combo), 0);
    }

    gtk_box_pack_start(GTK_BOX(box), lsp_enable, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), lsp_cmd, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), fmt_enable, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), fmt_cmd, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), perf_enable, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), telemetry_enable, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), keymap_combo, FALSE, FALSE, 4);
    gtk_widget_show_all(d);

    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        s->lsp_bridge_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lsp_enable));
        s->lsp_bridge_cmd = gtk_entry_get_text(GTK_ENTRY(lsp_cmd));
        s->formatter_on_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fmt_enable));
        s->formatter_cmd = gtk_entry_get_text(GTK_ENTRY(fmt_cmd));
        s->perf_mode_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(perf_enable));
        s->telemetry_opt_in = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(telemetry_enable));
        const gchar* km = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(keymap_combo));
        s->keymap_mode = km != nullptr ? km : "default";
        if (s->perf_mode_enabled) {
            rebuild_search_index_async(s);
        }
        save_session(s);
        set_status(s, "Language settings updated");
    }
    gtk_widget_destroy(d);
}

void on_action_reload_plugins(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    load_plugins(s);
    set_status(s, "Plugins reloaded: " + std::to_string(s->plugin_names.size()));
}

void on_action_run_plugin_command(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    if (s->plugin_commands.empty()) {
        set_status(s, "No plugin commands available");
        return;
    }
    GtkWidget* d = gtk_dialog_new_with_buttons("Run Plugin Command",
                                                GTK_WINDOW(s->window),
                                                GTK_DIALOG_MODAL,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Run",
                                                GTK_RESPONSE_ACCEPT,
                                                nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(d));
    GtkWidget* combo = gtk_combo_box_text_new();
    for (const auto& [name, _] : s->plugin_commands) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), name.c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    GtkWidget* payload = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(payload), "Payload");
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(box), payload, FALSE, FALSE, 6);
    gtk_widget_show_all(d);
    if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
        gchar* cmd = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
        if (cmd != nullptr) {
            auto it = s->plugin_commands.find(cmd);
            if (it != s->plugin_commands.end()) {
                const std::string out = it->second(gtk_entry_get_text(GTK_ENTRY(payload)));
                show_simple_text_dialog(GTK_WINDOW(s->window), "Plugin Output", out.empty() ? "<empty>" : out);
                record_telemetry(s, "plugin_command", cmd);
            }
            g_free(cmd);
        }
    }
    gtk_widget_destroy(d);
}

void on_window_destroy(GtkWidget*, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    save_session(s);
    clear_recovery_snapshot(s);
    persist_search_index(s);
    if (s->deferred_outline_timer != 0) {
        g_source_remove(s->deferred_outline_timer);
        s->deferred_outline_timer = 0;
    }
    unload_plugins(s);
    if (s->css_provider != nullptr) {
        g_object_unref(s->css_provider);
        s->css_provider = nullptr;
    }
}

void cycle_tabs(AppState* s, int direction) {
    ensure_active_notebook(s);
    GtkNotebook* nb = GTK_NOTEBOOK(s->active_notebook);
    const int pages = gtk_notebook_get_n_pages(nb);
    if (pages <= 1) {
        return;
    }
    int page = gtk_notebook_get_current_page(nb);
    if (page < 0) {
        page = 0;
    }
    page = (page + direction + pages) % pages;
    gtk_notebook_set_current_page(nb, page);
}

void adjust_ui_zoom(AppState* s, int delta_percent) {
    if (s == nullptr) {
        return;
    }
    s->ui_zoom_percent = std::max(70, std::min(200, s->ui_zoom_percent + delta_percent));
    apply_appearance(s);
    save_session(s);
    set_status(s, "UI zoom: " + std::to_string(s->ui_zoom_percent) + "%");
}

void reset_ui_zoom(AppState* s) {
    if (s == nullptr) {
        return;
    }
    s->ui_zoom_percent = 100;
    apply_appearance(s);
    save_session(s);
    set_status(s, "UI zoom reset: 100%");
}

void toggle_compact_mode(AppState* s) {
    if (s == nullptr) {
        return;
    }
    s->compact_mode = !s->compact_mode;
    apply_appearance(s);
    save_session(s);
    set_status(s, s->compact_mode ? "Compact mode enabled" : "Compact mode disabled");
}

void toggle_high_contrast_mode(AppState* s) {
    if (s == nullptr) {
        return;
    }
    s->high_contrast = !s->high_contrast;
    apply_appearance(s);
    save_session(s);
    set_status(s, s->high_contrast ? "High contrast enabled" : "High contrast disabled");
}

gboolean on_window_key_press(GtkWidget*, GdkEventKey* event, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    EditorTab* tab = current_tab(s);
    const bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
    const bool shift = (event->state & GDK_SHIFT_MASK) != 0;

    if (!ctrl && tab != nullptr && tab->buffer != nullptr) {
        if (s->keymap_mode == "vim") {
            GtkTextIter it;
            gtk_text_buffer_get_iter_at_mark(tab->buffer, &it, gtk_text_buffer_get_insert(tab->buffer));
            if (event->keyval == GDK_KEY_h) {
                gtk_text_iter_backward_char(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
            if (event->keyval == GDK_KEY_l) {
                gtk_text_iter_forward_char(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
            if (event->keyval == GDK_KEY_j) {
                gtk_text_iter_forward_line(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
            if (event->keyval == GDK_KEY_k) {
                gtk_text_iter_backward_line(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
        }
    }

    if (ctrl) {
        const bool alt = (event->state & GDK_MOD1_MASK) != 0;
        if (!alt && (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal || event->keyval == GDK_KEY_KP_Add)) {
            adjust_ui_zoom(s, 10);
            return TRUE;
        }
        if (!alt && (event->keyval == GDK_KEY_minus || event->keyval == GDK_KEY_KP_Subtract)) {
            adjust_ui_zoom(s, -10);
            return TRUE;
        }
        if (!alt && (event->keyval == GDK_KEY_0 || event->keyval == GDK_KEY_KP_0)) {
            reset_ui_zoom(s);
            return TRUE;
        }
        if (alt && event->keyval == GDK_KEY_c) {
            toggle_compact_mode(s);
            return TRUE;
        }
        if (alt && event->keyval == GDK_KEY_h) {
            toggle_high_contrast_mode(s);
            return TRUE;
        }
        if (s->keymap_mode == "emacs" && tab != nullptr && tab->buffer != nullptr) {
            GtkTextIter it;
            gtk_text_buffer_get_iter_at_mark(tab->buffer, &it, gtk_text_buffer_get_insert(tab->buffer));
            if (event->keyval == GDK_KEY_a) {
                gtk_text_iter_set_line_offset(&it, 0);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
            if (event->keyval == GDK_KEY_e) {
                gtk_text_iter_forward_to_line_end(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
            if (event->keyval == GDK_KEY_p) {
                gtk_text_iter_backward_line(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
            if (event->keyval == GDK_KEY_n) {
                gtk_text_iter_forward_line(&it);
                gtk_text_buffer_place_cursor(tab->buffer, &it);
                return TRUE;
            }
        }
        if (event->keyval == GDK_KEY_s && !alt) {
            on_action_save(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_f) {
            if (shift) {
                on_action_find(nullptr, s);
            } else {
                on_action_find(nullptr, s);
            }
            return TRUE;
        }
        if (event->keyval == GDK_KEY_h && shift) {
            on_action_replace(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_p) {
            on_action_palette(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_space) {
            on_action_lsp_completion(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_k && !shift) {
            on_action_lsp_hover(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_k && shift) {
            on_action_lsp_signature(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_j) {
            on_action_insert_snippet(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_period && shift) {
            on_action_code_actions(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_l && alt) {
            on_action_language_settings(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_r && alt) {
            on_action_reload_plugins(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_w && shift) {
            on_action_workspace_settings(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_s && alt) {
            on_action_buffer_autosave_settings(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_r) {
            if (shift) {
                on_action_rename_symbol(nullptr, s);
            } else {
                on_action_outline_jump(nullptr, s);
            }
            return TRUE;
        }
        if (event->keyval == GDK_KEY_t && shift) {
            on_action_test_runner(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_d && shift) {
            on_action_doc_preview(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_v && shift) {
            on_action_validate_tasks(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_m && shift) {
            on_action_memory_snapshot(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_n && shift) {
            on_action_memory_diff(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_g && shift) {
            on_action_git_diff(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_b && shift) {
            on_action_git_blame(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_y && shift) {
            on_action_git_stage_hunk(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_a && shift) {
            on_action_terminal_actions(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_e && shift) {
            on_action_semantic_search(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_x && shift) {
            on_action_build_matrix(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_i && shift) {
            on_action_fix_core_imports(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_o && shift) {
            on_action_core_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_u && shift) {
            on_action_std_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_l && shift) {
            on_action_log_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_k && shift) {
            on_action_log_quickfix_config(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_f && shift) {
            on_action_fs_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_j && shift) {
            on_action_db_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_p && shift) {
            on_action_db_quickfix_config(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_h && shift) {
            on_action_http_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_c && shift) {
            on_action_http_client_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_q && shift) {
            on_action_http_quickfix_config(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_z && shift) {
            on_action_http_client_quickfix_config(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_m && alt) {
            on_action_process_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_x && alt) {
            on_action_process_quickfix_config(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_4 && alt) {
            on_action_test_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_5 && alt) {
            on_action_lint_api_doctor(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_6 && alt) {
            on_action_lint_quickfix_config(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_1 && alt) {
            on_action_api_contracts_panel(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_2 && alt) {
            on_action_perf_doctor_panel(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_3 && alt) {
            on_action_security_doctor_panel(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_7 && alt) {
            on_action_release_doctor_panel(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_8 && alt) {
            on_action_open_failing_report(nullptr, s);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_Tab) {
            const bool back = (event->state & GDK_SHIFT_MASK) != 0;
            cycle_tabs(s, back ? -1 : 1);
            return TRUE;
        }
    }

    if (event->keyval == GDK_KEY_F12) {
        if ((event->state & GDK_SHIFT_MASK) != 0) {
            on_action_find_references(nullptr, s);
        } else {
            on_action_go_to_definition(nullptr, s);
        }
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F4) {
        on_action_profile_settings(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F5) {
        on_action_run(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F8) {
        on_action_build(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F9) {
        on_action_check(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F6) {
        on_action_run_task(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F7) {
        rebuild_search_index_async(s);
        set_status(s, "Rebuilding search index...");
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F10) {
        apply_runtime_config_file(s, true);
        return TRUE;
    }

    if (event->keyval == GDK_KEY_v) {
        on_toggle_split(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_h) {
        on_toggle_split_orientation(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_t) {
        on_switch_active_pane(nullptr, s);
        return TRUE;
    }
    if (event->keyval == GDK_KEY_F2) {
        on_toggle_bottom_panel(nullptr, s);
        return TRUE;
    }

    return FALSE;
}

GtkWidget* make_toolbar_button(const char* icon_name, const char* tooltip, GCallback cb, AppState* s) {
    GtkWidget* btn = gtk_button_new();
    GtkWidget* img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(btn), img);
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    const int base = s != nullptr && s->compact_mode ? 24 : 28;
    const int zoom = s != nullptr ? std::max(70, std::min(200, s->ui_zoom_percent)) : 100;
    const int side = std::max(20, static_cast<int>(std::lround(static_cast<double>(base) * static_cast<double>(zoom) / 100.0)));
    gtk_widget_set_size_request(btn, side, side);
    gtk_widget_set_tooltip_text(btn, tooltip);
    g_signal_connect(btn, "clicked", cb, s);
    return btn;
}

void activate(GtkApplication* app, gpointer user_data) {
    auto* s = static_cast<AppState*>(user_data);
    s->app = app;

    s->window = gtk_application_window_new(app);
    int default_w = 1180;
    int default_h = 800;
    if (GdkDisplay* display = gdk_display_get_default(); display != nullptr) {
        GdkMonitor* monitor = gdk_display_get_primary_monitor(display);
        if (monitor == nullptr) {
            monitor = gdk_display_get_monitor(display, 0);
        }
        if (monitor != nullptr) {
            GdkRectangle geometry{};
            gdk_monitor_get_geometry(monitor, &geometry);
            // Keep first launch comfortable on large screens; user can still resize manually.
            default_w = std::max(920, std::min(1260, static_cast<int>(geometry.width * 0.82)));
            default_h = std::max(640, std::min(920, static_cast<int>(geometry.height * 0.9)));
        }
    }
    gtk_window_set_default_size(GTK_WINDOW(s->window), default_w, default_h);
    gtk_window_set_position(GTK_WINDOW(s->window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(s->window), "Vitte Editor");
    gtk_window_set_icon_name(GTK_WINDOW(s->window), "vitte");
    g_object_set_data(G_OBJECT(s->window), "app-state", s);

    if (fs::exists("toolchain/assets/vitte-logo-circle-blue.svg")) {
        gtk_window_set_icon_from_file(GTK_WINDOW(s->window), "toolchain/assets/vitte-logo-circle-blue.svg", nullptr);
    } else if (fs::exists("/usr/share/icons/hicolor/scalable/apps/vitte.svg")) {
        gtk_window_set_icon_from_file(GTK_WINDOW(s->window), "/usr/share/icons/hicolor/scalable/apps/vitte.svg", nullptr);
    }

    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(s->window), root);

    GtkWidget* toolbar_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(toolbar_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_box_pack_start(GTK_BOX(root), toolbar_scroll, FALSE, TRUE, 0);
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(toolbar, 6);
    gtk_widget_set_margin_end(toolbar, 6);
    gtk_widget_set_margin_top(toolbar, 6);
    gtk_widget_set_margin_bottom(toolbar, 6);
    gtk_widget_set_size_request(toolbar, default_w - 24, -1);
    gtk_container_add(GTK_CONTAINER(toolbar_scroll), toolbar);

    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("document-open", "Open", G_CALLBACK(on_action_open), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("folder-open", "Workspace roots (Ctrl+Shift+W)", G_CALLBACK(on_action_workspace_settings), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("document-save", "Save", G_CALLBACK(on_action_save), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("edit-find", "Find in files", G_CALLBACK(on_action_find), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("edit-find-replace", "Replace in files", G_CALLBACK(on_action_replace), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("system-search", "Ctrl+P palette", G_CALLBACK(on_action_palette), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("applications-education", "LSP completion (Ctrl+Space)", G_CALLBACK(on_action_lsp_completion), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("help-about", "LSP hover/signature (Ctrl+K / Ctrl+Shift+K)", G_CALLBACK(on_action_lsp_hover), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("insert-text", "Insert Vitte snippet (Ctrl+J)", G_CALLBACK(on_action_insert_snippet), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("edit-find", "Semantic search", G_CALLBACK(on_action_semantic_search), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("system-software-update", "Code actions (Ctrl+>)", G_CALLBACK(on_action_code_actions), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("applications-engineering", "Run plugin command", G_CALLBACK(on_action_run_plugin_command), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("x-office-document", "Doc preview (plugin)", G_CALLBACK(on_action_doc_preview), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("go-jump", "Go to definition (F12)", G_CALLBACK(on_action_go_to_definition), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("edit-select-all", "Find references (Shift+F12)", G_CALLBACK(on_action_find_references), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("edit-rename", "Rename symbol (Ctrl+Shift+R)", G_CALLBACK(on_action_rename_symbol), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("view-split-left-right", "Toggle split (v)", G_CALLBACK(on_toggle_split), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("view-refresh", "Split orientation (h)", G_CALLBACK(on_toggle_split_orientation), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("utilities-terminal", "Toggle bottom panel (F2)", G_CALLBACK(on_toggle_bottom_panel), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("preferences-desktop-theme", "Theme/Font settings", G_CALLBACK(on_action_ui_settings), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("preferences-other", "Language/LSP/Formatter settings", G_CALLBACK(on_action_language_settings), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("preferences-system", "Autosave settings", G_CALLBACK(on_action_autosave_settings), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("document-properties", "Buffer autosave (Ctrl+Alt+S)", G_CALLBACK(on_action_buffer_autosave_settings), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("applications-system", "Build/Run profiles (F4)", G_CALLBACK(on_action_profile_settings), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("view-list-details", "Run task (F6)", G_CALLBACK(on_action_run_task), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-warning", "Validate tasks.vitte.json", G_CALLBACK(on_action_validate_tasks), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("media-playlist-shuffle", "Test runner auto", G_CALLBACK(on_action_test_runner), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("vcs-diff", "Git quick diff", G_CALLBACK(on_action_git_diff), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("changes-prevent", "Git blame line", G_CALLBACK(on_action_git_blame), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("list-add", "Git stage hunk", G_CALLBACK(on_action_git_stage_hunk), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("applications-utilities", "Terminal -> code actions", G_CALLBACK(on_action_terminal_actions), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("system-run", "Build matrix", G_CALLBACK(on_action_build_matrix), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("edit-paste", "Fix imports core", G_CALLBACK(on_action_fix_core_imports), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "Core API Doctor", G_CALLBACK(on_action_core_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "Std API Doctor (Ctrl+Shift+U)", G_CALLBACK(on_action_std_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "Log API Doctor (Ctrl+Shift+L)", G_CALLBACK(on_action_log_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "FS API Doctor (Ctrl+Shift+F)", G_CALLBACK(on_action_fs_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "DB API Doctor (Ctrl+Shift+J)", G_CALLBACK(on_action_db_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "HTTP API Doctor (Ctrl+Shift+H)", G_CALLBACK(on_action_http_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "HTTP Client API Doctor (Ctrl+Shift+C)", G_CALLBACK(on_action_http_client_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "Process API Doctor (Alt+M)", G_CALLBACK(on_action_process_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "Test API Doctor (Alt+4)", G_CALLBACK(on_action_test_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("dialog-information", "Lint API Doctor (Alt+5)", G_CALLBACK(on_action_lint_api_doctor), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("tools-check-spelling", "Log config quick-fix (Ctrl+Shift+K)", G_CALLBACK(on_action_log_quickfix_config), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("tools-check-spelling", "DB config quick-fix (Ctrl+Shift+P)", G_CALLBACK(on_action_db_quickfix_config), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("tools-check-spelling", "HTTP config quick-fix (Ctrl+Shift+Q)", G_CALLBACK(on_action_http_quickfix_config), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("tools-check-spelling", "HTTP client config quick-fix (Ctrl+Shift+Z)", G_CALLBACK(on_action_http_client_quickfix_config), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("tools-check-spelling", "Process config quick-fix (Alt+X)", G_CALLBACK(on_action_process_quickfix_config), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("tools-check-spelling", "Lint quick-fix (Alt+6)", G_CALLBACK(on_action_lint_quickfix_config), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("view-list-tree", "API Contracts panel (Alt+1)", G_CALLBACK(on_action_api_contracts_panel), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("utilities-system-monitor", "Perf Doctor panel (Alt+2)", G_CALLBACK(on_action_perf_doctor_panel), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("security-high", "Security Doctor panel (Alt+3)", G_CALLBACK(on_action_security_doctor_panel), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("applications-system", "Release Doctor panel (Alt+7)", G_CALLBACK(on_action_release_doctor_panel), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("go-jump", "Open failing report (Alt+8)", G_CALLBACK(on_action_open_failing_report), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("security-high", "Security lint", G_CALLBACK(on_action_security_lint), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("accessories-text-editor", "Style enforcer", G_CALLBACK(on_action_style_enforcer), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("media-record", "Memory snapshot", G_CALLBACK(on_action_memory_snapshot), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("view-refresh", "Memory diff", G_CALLBACK(on_action_memory_diff), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("x-office-spreadsheet", "Profiler timeline", G_CALLBACK(on_action_profiler_timeline), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("utilities-system-monitor", "Profiler hotspots", G_CALLBACK(on_action_profiler_hotspots), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("media-record", "Debug refresh", G_CALLBACK(on_action_debug_refresh), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("view-refresh", "Reload plugins", G_CALLBACK(on_action_reload_plugins), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("system-run", "Check (F9)", G_CALLBACK(on_action_check), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("applications-development", "Build (F8)", G_CALLBACK(on_action_build), s), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), make_toolbar_button("media-playback-start", "Run (F5)", G_CALLBACK(on_action_run), s), FALSE, FALSE, 0);

    GtkWidget* main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(root), main_paned, TRUE, TRUE, 0);

    GtkWidget* left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(left_box, 300, -1);
    gtk_paned_pack1(GTK_PANED(main_paned), left_box, FALSE, FALSE);

    GtkWidget* logo = gtk_image_new_from_file("toolchain/assets/vitte-logo-circle-blue.svg");
    gtk_widget_set_margin_top(logo, 8);
    gtk_widget_set_margin_bottom(logo, 8);
    gtk_box_pack_start(GTK_BOX(left_box), logo, FALSE, FALSE, 0);

    GtkWidget* left_split = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(left_box), left_split, TRUE, TRUE, 0);

    s->project_store = gtk_tree_store_new(NUM_PROJECT_COLS, G_TYPE_STRING, G_TYPE_STRING);
    s->project_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->project_store));
    GtkCellRenderer* proj_r = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->project_view), gtk_tree_view_column_new_with_attributes("Project", proj_r, "text", COL_NAME, nullptr));

    GtkWidget* proj_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(proj_scroll), s->project_view);
    gtk_paned_pack1(GTK_PANED(left_split), proj_scroll, TRUE, FALSE);

    s->outline_store = gtk_tree_store_new(NUM_OUT_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
    s->outline_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->outline_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(s->outline_view), FALSE);
    GtkCellRenderer* out_icon = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer* out_r1 = gtk_cell_renderer_text_new();
    GtkCellRenderer* out_r2 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->outline_view),
                                gtk_tree_view_column_new_with_attributes("", out_icon, "icon-name", OUT_ICON, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->outline_view), gtk_tree_view_column_new_with_attributes("Kind", out_r1, "text", OUT_KIND, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->outline_view), gtk_tree_view_column_new_with_attributes("Outline", out_r2, "text", OUT_NAME, nullptr));

    GtkWidget* out_tabs = gtk_notebook_new();
    s->left_tabs = out_tabs;

    GtkWidget* out_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* out_search_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    s->outline_filter_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(s->outline_filter_entry), "Rechercher un symbole...");
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(s->outline_filter_entry), GTK_ENTRY_ICON_PRIMARY, "edit-find-symbolic");
    GtkWidget* out_clear = gtk_button_new_with_label("Effacer");
    g_signal_connect(out_clear, "clicked", G_CALLBACK(on_outline_filter_clear), s);
    gtk_box_pack_start(GTK_BOX(out_search_row), s->outline_filter_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(out_search_row), out_clear, FALSE, FALSE, 0);

    GtkWidget* out_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(out_scroll), s->outline_view);
    s->outline_empty_label = gtk_label_new("Aucun symbole trouvé");
    gtk_widget_set_halign(s->outline_empty_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(s->outline_empty_label, 6);
    gtk_widget_set_margin_bottom(s->outline_empty_label, 4);

    gtk_box_pack_start(GTK_BOX(out_page), out_search_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(out_page), out_scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(out_page), s->outline_empty_label, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(out_tabs), out_page, gtk_label_new("Symboles"));

    GtkWidget* docs_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    s->docs_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(s->docs_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(s->docs_view), TRUE);
    s->docs_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(s->docs_view));
    gtk_text_buffer_set_text(s->docs_buffer, "Doc preview panel.\nUse toolbar button to refresh for current symbol.", -1);
    gtk_container_add(GTK_CONTAINER(docs_scroll), s->docs_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(out_tabs), docs_scroll, gtk_label_new("Docs"));

    GtkWidget* tools_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    s->tools_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(s->tools_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(s->tools_view), TRUE);
    s->tools_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(s->tools_view));
    gtk_text_buffer_set_text(s->tools_buffer, "Tools panel.\nUse dedicated toolbar buttons (security/style/memory/profiler).", -1);
    gtk_container_add(GTK_CONTAINER(tools_scroll), s->tools_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(out_tabs), tools_scroll, gtk_label_new("Tools"));

    gtk_paned_pack2(GTK_PANED(left_split), out_tabs, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(left_split), 460);

    GtkWidget* right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_paned_pack2(GTK_PANED(main_paned), right_box, TRUE, FALSE);

    GtkWidget* editor_and_gutter = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(right_box), editor_and_gutter, TRUE, TRUE, 0);
    s->editor_split = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(editor_and_gutter), s->editor_split, TRUE, TRUE, 0);

    s->notebook_primary = gtk_notebook_new();
    s->notebook_secondary = gtk_notebook_new();
    s->active_notebook = s->notebook_primary;
    gtk_paned_pack1(GTK_PANED(s->editor_split), s->notebook_primary, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(s->editor_split), s->notebook_secondary, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(s->editor_split), 880);

    s->gutter_store = gtk_list_store_new(1, G_TYPE_STRING);
    s->gutter_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->gutter_store));
    GtkCellRenderer* gr = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->gutter_view), gtk_tree_view_column_new_with_attributes("Gutter", gr, "text", 0, nullptr));
    GtkWidget* gutter_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_widget_set_size_request(gutter_scroll, 90, -1);
    gtk_container_add(GTK_CONTAINER(gutter_scroll), s->gutter_view);
    gtk_box_pack_start(GTK_BOX(editor_and_gutter), gutter_scroll, FALSE, TRUE, 0);

    apply_split_layout(s);

    s->bottom_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(s->bottom_panel, -1, 245);
    gtk_box_pack_start(GTK_BOX(right_box), s->bottom_panel, FALSE, TRUE, 0);

    s->bottom_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(s->bottom_panel), s->bottom_notebook, TRUE, TRUE, 0);

    GtkWidget* problems_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* filter_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget* filter_lbl = gtk_label_new("Problems:");
    s->problems_filter = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(s->problems_filter), "All");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(s->problems_filter), "Errors");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(s->problems_filter), "Current File");
    gtk_combo_box_set_active(GTK_COMBO_BOX(s->problems_filter), 0);
    gtk_box_pack_start(GTK_BOX(filter_row), filter_lbl, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(filter_row), s->problems_filter, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(problems_box), filter_row, FALSE, FALSE, 2);

    s->problems_store = gtk_list_store_new(NUM_PROB_COLS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    s->problems_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->problems_store));
    GtkCellRenderer* pr1 = gtk_cell_renderer_text_new();
    GtkCellRenderer* pr2 = gtk_cell_renderer_text_new();
    GtkCellRenderer* pr3 = gtk_cell_renderer_text_new();
    GtkCellRenderer* pr4 = gtk_cell_renderer_text_new();
    GtkCellRenderer* pr5 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->problems_view), gtk_tree_view_column_new_with_attributes("File", pr1, "text", PROB_FILE, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->problems_view), gtk_tree_view_column_new_with_attributes("Line", pr2, "text", PROB_LINE, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->problems_view), gtk_tree_view_column_new_with_attributes("Col", pr3, "text", PROB_COL, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->problems_view), gtk_tree_view_column_new_with_attributes("Severity", pr4, "text", PROB_SEV, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(s->problems_view), gtk_tree_view_column_new_with_attributes("Message", pr5, "text", PROB_TEXT, nullptr));
    GtkWidget* prob_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(prob_scroll), s->problems_view);
    gtk_box_pack_start(GTK_BOX(problems_box), prob_scroll, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(s->bottom_notebook), problems_box, gtk_label_new("Problems"));

    GtkWidget* term_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* term_cmd_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    s->terminal_cmd_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(s->terminal_cmd_entry), "Command in project root (bash)");
    GtkWidget* term_run = gtk_button_new_with_label("Run");
    GtkWidget* term_clear = gtk_button_new_with_label("Clear");
    gtk_box_pack_start(GTK_BOX(term_cmd_row), s->terminal_cmd_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(term_cmd_row), term_run, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(term_cmd_row), term_clear, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(term_box), term_cmd_row, FALSE, FALSE, 0);
    GtkWidget* term_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    s->terminal_output_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(s->terminal_output_view), TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(s->terminal_output_view), FALSE);
    s->terminal_output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(s->terminal_output_view));
    gtk_container_add(GTK_CONTAINER(term_scroll), s->terminal_output_view);
    gtk_box_pack_start(GTK_BOX(term_box), term_scroll, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(s->bottom_notebook), term_box, gtk_label_new("Terminal"));

    GtkWidget* debug_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget* debug_toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget* dbg_add_bp = gtk_button_new_with_label("Add BP");
    GtkWidget* dbg_refresh = gtk_button_new_with_label("Refresh");
    s->debug_watches_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(s->debug_watches_entry), "watch expr");
    GtkWidget* dbg_add_watch = gtk_button_new_with_label("Add Watch");
    gtk_box_pack_start(GTK_BOX(debug_toolbar), dbg_add_bp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(debug_toolbar), dbg_refresh, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(debug_toolbar), s->debug_watches_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(debug_toolbar), dbg_add_watch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(debug_box), debug_toolbar, FALSE, FALSE, 0);

    GtkWidget* debug_split = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(debug_box), debug_split, TRUE, TRUE, 0);

    s->debug_callstack_store = gtk_list_store_new(NUM_DBG_STACK_COLS, G_TYPE_STRING);
    GtkWidget* callstack_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->debug_callstack_store));
    GtkCellRenderer* c1 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(callstack_view), gtk_tree_view_column_new_with_attributes("Callstack", c1, "text", DBG_STACK_FRAME, nullptr));
    GtkWidget* cs_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(cs_scroll), callstack_view);

    GtkWidget* debug_right = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack1(GTK_PANED(debug_split), cs_scroll, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(debug_split), debug_right, TRUE, FALSE);

    s->debug_vars_store = gtk_list_store_new(NUM_DBG_VAR_COLS, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget* vars_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->debug_vars_store));
    GtkCellRenderer* v1 = gtk_cell_renderer_text_new();
    GtkCellRenderer* v2 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(vars_view), gtk_tree_view_column_new_with_attributes("Var", v1, "text", DBG_VAR_NAME, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(vars_view), gtk_tree_view_column_new_with_attributes("Value", v2, "text", DBG_VAR_VALUE, nullptr));
    GtkWidget* vars_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(vars_scroll), vars_view);
    gtk_paned_pack1(GTK_PANED(debug_right), vars_scroll, TRUE, FALSE);

    GtkWidget* debug_bottom = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack2(GTK_PANED(debug_right), debug_bottom, TRUE, FALSE);
    s->debug_watches_store = gtk_list_store_new(NUM_DBG_VAR_COLS, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget* watches_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->debug_watches_store));
    GtkCellRenderer* w1 = gtk_cell_renderer_text_new();
    GtkCellRenderer* w2 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(watches_view), gtk_tree_view_column_new_with_attributes("Watch", w1, "text", DBG_VAR_NAME, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(watches_view), gtk_tree_view_column_new_with_attributes("Value", w2, "text", DBG_VAR_VALUE, nullptr));
    GtkWidget* wt_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(wt_scroll), watches_view);
    gtk_paned_pack1(GTK_PANED(debug_bottom), wt_scroll, TRUE, FALSE);

    s->debug_breakpoints_store = gtk_list_store_new(NUM_DBG_BP_COLS, G_TYPE_STRING, G_TYPE_INT);
    GtkWidget* bp_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(s->debug_breakpoints_store));
    GtkCellRenderer* b1 = gtk_cell_renderer_text_new();
    GtkCellRenderer* b2 = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(bp_view), gtk_tree_view_column_new_with_attributes("BP File", b1, "text", DBG_BP_FILE, nullptr));
    gtk_tree_view_append_column(GTK_TREE_VIEW(bp_view), gtk_tree_view_column_new_with_attributes("Line", b2, "text", DBG_BP_LINE, nullptr));
    GtkWidget* bp_scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(bp_scroll), bp_view);
    gtk_paned_pack2(GTK_PANED(debug_bottom), bp_scroll, TRUE, FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(s->bottom_notebook), debug_box, gtk_label_new("Debug"));

    s->status_label = gtk_label_new("Ready");
    gtk_widget_set_margin_start(s->status_label, 8);
    gtk_widget_set_margin_end(s->status_label, 8);
    gtk_widget_set_margin_top(s->status_label, 4);
    gtk_widget_set_margin_bottom(s->status_label, 4);
    gtk_box_pack_start(GTK_BOX(root), s->status_label, FALSE, FALSE, 0);

    load_session(s);
    apply_runtime_config_file(s, false);
    apply_window_start_mode(s);

    g_signal_connect(s->project_view, "row-activated", G_CALLBACK(on_project_row_activated), s);
    g_signal_connect(s->outline_view, "row-activated", G_CALLBACK(on_outline_row_activated), s);
    if (s->outline_filter_entry != nullptr) {
        g_signal_connect(s->outline_filter_entry, "changed", G_CALLBACK(on_outline_filter_changed), s);
    }
    g_signal_connect(s->problems_view, "row-activated", G_CALLBACK(on_problem_row_activated), s);
    g_signal_connect(s->gutter_view, "row-activated", G_CALLBACK(on_gutter_row_activated), s);
    g_signal_connect(s->problems_filter, "changed", G_CALLBACK(on_problems_filter_changed), s);
    g_signal_connect(s->terminal_cmd_entry, "activate", G_CALLBACK(on_terminal_entry_activate), s);
    g_signal_connect(term_run, "clicked", G_CALLBACK(on_terminal_run), s);
    g_signal_connect(term_clear, "clicked", G_CALLBACK(on_terminal_clear), s);
    g_signal_connect(dbg_add_bp, "clicked", G_CALLBACK(on_action_debug_add_breakpoint), s);
    g_signal_connect(dbg_refresh, "clicked", G_CALLBACK(on_action_debug_refresh), s);
    g_signal_connect(dbg_add_watch, "clicked", G_CALLBACK(on_action_debug_add_watch), s);
    g_signal_connect(s->debug_watches_entry, "activate", G_CALLBACK(on_action_debug_add_watch), s);

    g_signal_connect(s->notebook_primary, "switch-page", G_CALLBACK(on_notebook_switch_page), s);
    g_signal_connect(s->notebook_secondary, "switch-page", G_CALLBACK(on_notebook_switch_page), s);
    g_signal_connect(s->notebook_primary, "focus-in-event", G_CALLBACK(on_notebook_focus), s);
    g_signal_connect(s->notebook_secondary, "focus-in-event", G_CALLBACK(on_notebook_focus), s);

    g_signal_connect(s->window, "destroy", G_CALLBACK(on_window_destroy), s);
    g_signal_connect(s->window, "key-press-event", G_CALLBACK(on_window_key_press), s);

    apply_appearance(s);
    start_autosave_timer(s);
    append_terminal_text(s, "Integrated terminal ready. Commands run in project root.\n");
    gtk_widget_show_all(s->window);
    apply_split_layout(s);
    if (!s->bottom_panel_visible && s->bottom_panel != nullptr) {
        gtk_widget_hide(s->bottom_panel);
    }
    g_idle_add(run_deferred_startup, s);
}

}  // namespace

int main(int argc, char** argv) {
    AppState state;
    state.root = fs::current_path().string();
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]) != "") {
        state.root = fs::absolute(argv[1]).string();
    }
    state.workspace_roots.push_back(state.root);

    GtkApplication* app = gtk_application_new("dev.vitte.ide.gtk", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &state);

    const int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    for (EditorTab* t : state.tabs_primary) {
        delete t;
    }
    for (EditorTab* t : state.tabs_secondary) {
        delete t;
    }

    if (state.autosave_timer != 0) {
        g_source_remove(state.autosave_timer);
        state.autosave_timer = 0;
    }
    if (state.completion_prefetch_timer != 0) {
        g_source_remove(state.completion_prefetch_timer);
        state.completion_prefetch_timer = 0;
    }

    return status;
}
