#include "driver.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>

using vitte::driver::run;
using vitte::driver::CrashStage;

namespace {

struct CrashContext {
    std::string invoked_as;
    std::string command;
    std::string cwd;
    std::string version = "unknown";
    std::vector<std::string> args;
};

CrashContext g_ctx;

static std::string base_name(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    return std::filesystem::path(path).filename().string();
}

static std::string read_version_from_file(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in.is_open()) {
        return {};
    }
    std::string line;
    while (std::getline(in, line)) {
        const auto q1 = line.find('"');
        if (q1 != std::string::npos) {
            const auto q2 = line.find('"', q1 + 1);
            if (q2 != std::string::npos && q2 > q1 + 1) {
                return line.substr(q1 + 1, q2 - q1 - 1);
            }
        }
        if (!line.empty() && line.find("Version") == std::string::npos) {
            return line;
        }
    }
    return {};
}

static std::string detect_version() {
    if (const char* env = std::getenv("VITTE_VERSION"); env && *env) {
        return env;
    }
    std::vector<std::filesystem::path> candidates;
    if (const char* root = std::getenv("VITTE_ROOT"); root && *root) {
        candidates.emplace_back(std::filesystem::path(root) / "version");
    }
    std::error_code ec;
    candidates.emplace_back(std::filesystem::current_path(ec) / "version");
    candidates.emplace_back(std::filesystem::path("/usr/local/share/vitte/version"));

    for (const auto& c : candidates) {
        const std::string v = read_version_from_file(c);
        if (!v.empty()) {
            return v;
        }
    }
    return "unknown";
}

static std::string join_argv(int argc, char** argv) {
    std::string out;
    for (int i = 0; i < argc; ++i) {
        if (i > 0) out += " ";
        out += argv[i] ? argv[i] : "";
    }
    return out;
}

static std::string infer_input_file(const std::vector<std::string>& args) {
    std::string last_positional;
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a.empty() || a[0] == '-') {
            continue;
        }
        if (a == "parse" || a == "check" || a == "emit" || a == "build" || a == "profile" ||
            a == "doctor" || a == "help" || a == "mod" || a == "grammar" || a == "reduce" || a == "clean-cache" ||
            a == "init" || a == "explain") {
            continue;
        }
        last_positional = a;
    }
    return last_positional;
}

static std::string infer_reduce_stage(const std::vector<std::string>& args) {
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--stage" && i + 1 < args.size()) {
            return args[i + 1];
        }
        if (args[i].rfind("--stage=", 0) == 0) {
            return args[i].substr(std::string("--stage=").size());
        }
    }
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "parse") return "parse";
        if (args[i] == "check") return "ir";
        if (args[i] == "emit" || args[i] == "build" || args[i] == "profile") return "backend";
    }
    return "backend";
}

static bool contains_arg(const std::vector<std::string>& args, const char* needle) {
    for (const auto& a : args) {
        if (a == needle) {
            return true;
        }
    }
    return false;
}

static bool is_wrapper_invocation(const std::string& name) {
    return name == "vittec" || name == "vitte-linker";
}

static bool invoked_via_compat_wrapper(const std::string& invoked) {
    if (!is_wrapper_invocation(invoked)) {
        return false;
    }
#if !defined(_WIN32)
    std::error_code ec;
    const auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return base_name(exe.string()) == "vitte";
    }
#endif
    return true;
}

static void print_env_line(const char* key) {
    const char* value = std::getenv(key);
    std::cerr << "  env." << key << ": " << ((value && *value) ? value : "<unset>") << "\n";
}

static void print_crash_report(const std::string& reason,
                               int exit_code,
                               int sig = 0,
                               const char* sig_name = nullptr) {
    std::cerr << "[vittec][crash-report]\n";
    std::cerr << "  reason: " << reason << "\n";
    std::cerr << "  version: " << g_ctx.version << "\n";
    std::cerr << "  invoked-as: " << g_ctx.invoked_as << "\n";
    std::cerr << "  command: " << g_ctx.command << "\n";
    std::cerr << "  cwd: " << g_ctx.cwd << "\n";
    std::cerr << "  stage: " << vitte::driver::crash_stage_name(vitte::driver::get_crash_stage()) << "\n";
    std::cerr << "  exit-code: " << exit_code << "\n";
    if (sig != 0) {
        std::cerr << "  signal: " << (sig_name ? sig_name : "UNKNOWN") << " (" << sig << ")\n";
    }
    print_env_line("VITTE_ROOT");
    print_env_line("LLD_PATH");
    print_env_line("OPENSSL_DIR");
    print_env_line("CURL_DIR");
    print_env_line("CC");
    print_env_line("CXX");
    print_env_line("CFLAGS");
    print_env_line("CXXFLAGS");
    print_env_line("LDFLAGS");
    print_env_line("LANG");
    print_env_line("LC_ALL");

    const std::string input = infer_input_file(g_ctx.args);
    if (!input.empty()) {
        const std::string stage = infer_reduce_stage(g_ctx.args);
        std::cerr << "  repro: vitte reduce --stage " << stage << " " << input << "\n";
    }
}

static void maybe_trigger_test_trap() {
    const char* trap = std::getenv("VITTE_TEST_TRAP");
    if (!trap || !*trap) {
        return;
    }
    std::string v(trap);
    if (v == "SIGABRT") {
        std::raise(SIGABRT);
    } else if (v == "SIGSEGV") {
        std::raise(SIGSEGV);
    } else if (v == "SIGFPE") {
        std::raise(SIGFPE);
    } else if (v == "SIGILL") {
        std::raise(SIGILL);
    }
}

} // namespace

/* -------------------------------------------------
 * Signal handling
 * ------------------------------------------------- */

static void signal_handler(int sig) {
    const char* name = nullptr;

    switch (sig) {
        case SIGSEGV: name = "SIGSEGV"; break;
        case SIGABRT: name = "SIGABRT"; break;
        case SIGFPE:  name = "SIGFPE";  break;
        case SIGILL:  name = "SIGILL";  break;
        default:      name = "UNKNOWN"; break;
    }

    std::cerr
        << "[vittec] fatal signal: "
        << name << " (" << sig << ")\n";
    print_crash_report("fatal signal", 128 + sig, sig, name);

    std::_Exit(128 + sig);
}

static void install_signal_handlers() {
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGFPE,  signal_handler);
    std::signal(SIGILL,  signal_handler);
}

/* -------------------------------------------------
 * Entry point
 * ------------------------------------------------- */

int main(int argc, char** argv) {
    g_ctx.invoked_as = (argc > 0 && argv[0]) ? base_name(argv[0]) : "vitte";
    g_ctx.command = join_argv(argc, argv);
    g_ctx.version = detect_version();
    std::error_code ec;
    g_ctx.cwd = std::filesystem::current_path(ec).string();
    if (ec) {
        g_ctx.cwd = "<unknown>";
    }
    g_ctx.args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        g_ctx.args.push_back(argv[i] ? argv[i] : "");
    }

    if (invoked_via_compat_wrapper(g_ctx.invoked_as)) {
        std::cerr << "[compat] " << g_ctx.invoked_as
                  << " mode via vitte " << g_ctx.version << "\n";
    }

    install_signal_handlers();
    maybe_trigger_test_trap();

    try {
        const int rc = run(argc, argv);
        const bool doctor_mode = contains_arg(g_ctx.args, "doctor");
        if (rc != 0 && !doctor_mode && vitte::driver::get_crash_stage() == CrashStage::Backend) {
            print_crash_report("backend/linker failure", rc);
        }
        return rc;
    }
    catch (const std::exception& e) {
        std::cerr
            << "[vittec] uncaught C++ exception: "
            << e.what() << "\n";
        print_crash_report("uncaught exception", EXIT_FAILURE);
    }
    catch (...) {
        std::cerr
            << "[vittec] unknown fatal error\n";
        print_crash_report("unknown fatal error", EXIT_FAILURE);
    }

    return EXIT_FAILURE;
}
