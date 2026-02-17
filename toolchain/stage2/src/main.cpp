// ============================================================
// vittec — stage2 wrapper
// Delegates to bin/vitte when available
// ============================================================

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

static std::string find_vitte(const char* argv0) {
    if (const char* env = std::getenv("VITTE_BIN")) {
        return std::string(env);
    }

    namespace fs = std::filesystem;

    fs::path cwd = fs::current_path();
    fs::path candidate = cwd / "bin" / "vitte";
    if (fs::exists(candidate)) {
        return candidate.string();
    }

    if (argv0) {
        fs::path exe = fs::absolute(argv0);
        if (exe.has_parent_path()) {
            fs::path root = exe.parent_path().parent_path();
            fs::path alt = root / "bin" / "vitte";
            if (fs::exists(alt)) {
                return alt.string();
            }
        }
    }

    return {};
}

static int exec_vitte(const std::string& vitte_path, int argc, char** argv) {
#if defined(__unix__) || defined(__APPLE__)
    std::vector<char*> args;
    args.reserve(static_cast<std::size_t>(argc) + 1);
    args.push_back(const_cast<char*>(vitte_path.c_str()));
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    args.push_back(nullptr);
    execvp(args[0], args.data());
    return 127;
#else
    std::string cmd = "\"" + vitte_path + "\"";
    for (int i = 1; i < argc; ++i) {
        cmd.append(" \"").append(argv[i]).append("\"");
    }
    return std::system(cmd.c_str());
#endif
}

static std::optional<std::string> pick_input_from_src(const std::string& src_dir) {
    namespace fs = std::filesystem;
    fs::path dir(src_dir);
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return std::nullopt;
    }
    fs::path hello = dir / "hello.vit";
    if (fs::exists(hello)) {
        return hello.string();
    }
    std::optional<std::string> found;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".vit") {
            if (found) {
                return std::nullopt;
            }
            found = entry.path().string();
        }
    }
    return found;
}

static std::optional<std::string> output_from_out_dir(const std::string& out_dir, const std::string& input) {
    namespace fs = std::filesystem;
    fs::path out(out_dir);
    fs::path in(input);
    if (in.has_stem()) {
        return (out / in.stem()).string();
    }
    return std::nullopt;
}

static int forward_to_vitte(const std::string& vitte_path, int argc, char** argv) {
    if (argc < 2) {
        return exec_vitte(vitte_path, argc, argv);
    }

    std::string cmd = argv[1];
    if (cmd != "build" && cmd != "parse") {
        return exec_vitte(vitte_path, argc, argv);
    }

    std::string src_dir;
    std::string out_dir;
    std::string out_override;
    std::string opt_level;
    bool debug = false;
    std::vector<std::string> passthrough;
    std::string input;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--src" && i + 1 < argc) {
            src_dir = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            out_override = argv[++i];
        } else if (arg == "--out" && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (arg == "--opt" && i + 1 < argc) {
            opt_level = argv[++i];
        } else if (arg == "--debug") {
            debug = true;
        } else if (arg == "-o" && i + 1 < argc) {
            passthrough.push_back(arg);
            passthrough.push_back(argv[++i]);
        } else if (arg == "--stage" && i + 1 < argc) {
            passthrough.push_back(arg);
            passthrough.push_back(argv[++i]);
        } else if (arg == "--release") {
            // ignore
        } else if (!arg.empty() && arg[0] == '-') {
            passthrough.push_back(arg);
        } else {
            input = arg;
        }
    }

    if (input.empty() && !src_dir.empty()) {
        auto picked = pick_input_from_src(src_dir);
        if (picked) {
            input = *picked;
        }
    }

    if (input.empty()) {
        std::cerr << "vittec: missing input file\n";
        return 1;
    }

    std::vector<std::string> args;
    args.push_back(vitte_path);
    args.push_back(cmd);

    if (!out_override.empty()) {
        args.push_back("-o");
        args.push_back(out_override);
    } else if (!out_dir.empty()) {
        auto out = output_from_out_dir(out_dir, input);
        if (out) {
            args.push_back("-o");
            args.push_back(*out);
        }
    }

    if (!opt_level.empty()) {
        args.push_back("-O" + opt_level);
    }
    if (debug) {
        args.push_back("--debug");
    }
    for (const auto& p : passthrough) {
        args.push_back(p);
    }
    args.push_back(input);

#if defined(__unix__) || defined(__APPLE__)
    std::vector<char*> cargs;
    cargs.reserve(args.size() + 1);
    for (auto& s : args) {
        cargs.push_back(const_cast<char*>(s.c_str()));
    }
    cargs.push_back(nullptr);
    execvp(cargs[0], cargs.data());
    return 127;
#else
    std::string cmdline;
    for (const auto& s : args) {
        if (!cmdline.empty()) {
            cmdline.push_back(' ');
        }
        cmdline.append("\"").append(s).append("\"");
    }
    return std::system(cmdline.c_str());
#endif
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--version") {
        std::cout << "vittec stage2 wrapper\n";
        return 0;
    }

    if (argc < 2) {
        std::cerr << "vittec: expected command (parse/build)\n";
        return 1;
    }

    std::string vitte_path = find_vitte(argv[0]);
    if (vitte_path.empty()) {
        std::cerr << "vittec: cannot find bin/vitte. Build the project first.\n";
        return 1;
    }

    return forward_to_vitte(vitte_path, argc, argv);
}
