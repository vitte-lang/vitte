#include "cpp_backend.hpp"

#include "lower/lower_mir.hpp"
#include "emit/emit.hpp"
#include "context/cpp_context.hpp"
#include "toolchain/clang.hpp"
#include "toolchain/arduino.hpp"

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace vitte::backend {

static bool is_arduino_target(const std::string& target) {
    return target.rfind("arduino-", 0) == 0;
}

static bool is_kernel_target(const std::string& target) {
    return target.rfind("kernel-", 0) == 0;
}

static std::string trim_copy(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto start = s.find_first_not_of(ws);
    if (start == std::string::npos) return {};
    const auto end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

static std::unordered_map<std::string, std::string> load_board_map(
    const std::filesystem::path& base
) {
    std::unordered_map<std::string, std::string> map;
    std::filesystem::path file = base / "target/arduino/boards.txt";
    std::ifstream in(file);
    if (!in.is_open()) {
        return map;
    }
    std::string line;
    while (std::getline(in, line)) {
        auto t = trim_copy(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        auto pos = t.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = trim_copy(t.substr(0, pos));
        std::string val = trim_copy(t.substr(pos + 1));
        if (!key.empty() && !val.empty()) {
            map[key] = val;
        }
    }
    return map;
}

static context::CppContext build_context(const CppBackendOptions& options) {
    context::CppContext ctx;
    ctx.set_debug(options.debug);
    ctx.set_optimize(options.optimize);
    if (is_arduino_target(options.target)) {
        ctx.set_entry_mode(context::CppContext::EntryMode::Arduino);
    }
    if (is_kernel_target(options.target)) {
        ctx.set_entry_mode(context::CppContext::EntryMode::Freestanding);
    }
    ctx.add_include("<cstdint>");
    ctx.add_include("<cstdlib>");
    ctx.add_include("\"vitte_runtime.hpp\"");
    return ctx;
}

static ast::cpp::CppTranslationUnit lower_to_cpp(
    const vitte::ir::MirModule& mir_module,
    context::CppContext& ctx
) {
    return lower::lower_mir(mir_module, ctx);
}

/* -------------------------------------------------
 * Compile MIR to native executable via C++
 * ------------------------------------------------- */
bool compile_cpp_backend(
    const vitte::ir::MirModule& mir_module,
    const std::string& output_exe,
    const CppBackendOptions& options
) {
    context::CppContext ctx = build_context(options);
    ast::cpp::CppTranslationUnit tu = lower_to_cpp(mir_module, ctx);

    /* ---------------------------------------------
     * Emit C++ file
     * --------------------------------------------- */
    std::filesystem::path cpp_path =
        std::filesystem::path(options.work_dir) / "vitte_out.cpp";

    if (!emit::emit_file(cpp_path.string(), tu, ctx)) {
        std::cerr << "[cpp-backend] failed to emit C++ file\n";
        return false;
    }

    if (is_arduino_target(options.target)) {
        toolchain::ArduinoOptions arduino_opts;
        arduino_opts.verbose = options.verbose;
        arduino_opts.work_dir = options.work_dir;
        arduino_opts.upload = options.arduino_upload;
        arduino_opts.port = options.arduino_port;
        const char* cli = std::getenv("ARDUINO_CLI");
        if (cli && *cli) {
            arduino_opts.cli_path = cli;
        }
        if (!options.arduino_fqbn.empty()) {
            arduino_opts.fqbn = options.arduino_fqbn;
        } else {
            const char* fqbn = std::getenv("ARDUINO_FQBN");
            if (fqbn && *fqbn) {
                arduino_opts.fqbn = fqbn;
            } else {
                const char* root = std::getenv("VITTE_ROOT");
                std::filesystem::path base = root && *root
                    ? std::filesystem::path(root)
                    : std::filesystem::current_path();
                auto board_map = load_board_map(base);
                auto it = board_map.find(options.target);
                if (it != board_map.end()) {
                    arduino_opts.fqbn = it->second;
                } else if (options.target == "arduino-esp32") {
                    arduino_opts.fqbn = "esp32:esp32:esp32";
                } else if (options.target == "arduino-esp8266") {
                    arduino_opts.fqbn = "esp8266:esp8266:nodemcuv2";
                } else if (options.target == "arduino-stm32") {
                    arduino_opts.fqbn = "stm32duino:STM32:GenF1";
                }
            }
        }

        const char* root = std::getenv("VITTE_ROOT");
        std::filesystem::path base = root && *root
            ? std::filesystem::path(root)
            : std::filesystem::current_path();

        std::filesystem::path runtime_hdr = base / "target/arduino/include/vitte_runtime.hpp";
        std::filesystem::path runtime_cpp = base / "target/arduino/runtime/vitte_runtime.cpp";

        std::vector<std::string> extra_sources;
        if (std::filesystem::exists(runtime_cpp)) {
            extra_sources.push_back(runtime_cpp.string());
        }

        if (!toolchain::invoke_arduino_cli(
                cpp_path.string(),
                extra_sources,
                runtime_hdr.string(),
                output_exe,
                arduino_opts
            )) {
            std::cerr << "[cpp-backend] arduino-cli invocation failed\n";
            return false;
        }
    } else if (options.target == "kernel-x86_64-uefi") {
        toolchain::ClangOptions clang_opts;
        clang_opts.debug = options.debug;
        clang_opts.optimize = options.optimize;
        clang_opts.opt_level = options.opt_level;
        clang_opts.verbose = options.verbose;
        clang_opts.freestanding = true;
        clang_opts.target = "x86_64-unknown-windows";
        clang_opts.cxx_flags.push_back("-fshort-wchar");
        clang_opts.cxx_flags.push_back("-mno-red-zone");
        clang_opts.cxx_flags.push_back("-fno-pic");
        clang_opts.cxx_flags.push_back("-fno-pie");
        clang_opts.ld_flags.push_back("-fuse-ld=lld");
        clang_opts.ld_flags.push_back("-Wl,/entry:efi_main");
        clang_opts.ld_flags.push_back("-Wl,/subsystem:efi_application");
        clang_opts.ld_flags.push_back("-Wl,/nodefaultlib");

        const char* root = std::getenv("VITTE_ROOT");
        std::filesystem::path base = root && *root
            ? std::filesystem::path(root)
            : std::filesystem::current_path();

        std::filesystem::path rt_inc = base / "target/kernel/x86_64/uefi/include";
        std::filesystem::path rt_cpp = base / "target/kernel/x86_64/uefi/runtime/vitte_runtime.cpp";
        if (std::filesystem::exists(rt_inc)) {
            clang_opts.include_dirs.push_back(rt_inc.string());
        }
        if (std::filesystem::exists(rt_cpp)) {
            clang_opts.extra_sources.push_back(rt_cpp.string());
        }

        if (!toolchain::invoke_clang(
                cpp_path.string(),
                output_exe,
                clang_opts
            )) {
            std::cerr << "[cpp-backend] kernel (uefi) clang invocation failed\n";
            return false;
        }
    } else if (options.target == "kernel-x86_64-grub") {
        toolchain::ClangOptions clang_opts;
        clang_opts.debug = options.debug;
        clang_opts.optimize = options.optimize;
        clang_opts.opt_level = options.opt_level;
        clang_opts.verbose = options.verbose;
        clang_opts.freestanding = true;
        clang_opts.target = "x86_64-elf";
        clang_opts.cxx_flags.push_back("-mno-red-zone");
        clang_opts.cxx_flags.push_back("-fno-pic");
        clang_opts.cxx_flags.push_back("-fno-pie");
        clang_opts.ld_flags.push_back("-fuse-ld=lld");
        clang_opts.ld_flags.push_back("-nostdlib");
        clang_opts.ld_flags.push_back("-Wl,-z,notext");
        clang_opts.ld_flags.push_back("-Wl,-z,max-page-size=0x1000");

        const char* root = std::getenv("VITTE_ROOT");
        std::filesystem::path base = root && *root
            ? std::filesystem::path(root)
            : std::filesystem::current_path();

        std::filesystem::path rt_inc = base / "target/kernel/x86_64/grub/include";
        std::filesystem::path rt_cpp = base / "target/kernel/x86_64/grub/runtime/vitte_runtime.cpp";
        std::filesystem::path rt_int = base / "target/kernel/x86_64/grub/runtime/interrupts.cpp";
        std::filesystem::path rt_int_s = base / "target/kernel/x86_64/grub/runtime/interrupts.s";
        std::filesystem::path rt_gdt = base / "target/kernel/x86_64/grub/runtime/gdt.cpp";
        std::filesystem::path rt_paging = base / "target/kernel/x86_64/grub/runtime/paging.cpp";
        std::filesystem::path rt_start = base / "target/kernel/x86_64/grub/runtime/start.s";
        std::filesystem::path ld_script = base / "target/kernel/x86_64/grub/linker/linker.ld";

        if (std::filesystem::exists(rt_inc)) {
            clang_opts.include_dirs.push_back(rt_inc.string());
        }
        if (std::filesystem::exists(rt_cpp)) {
            clang_opts.extra_sources.push_back(rt_cpp.string());
        }
        if (std::filesystem::exists(rt_int)) {
            clang_opts.extra_sources.push_back(rt_int.string());
        }
        if (std::filesystem::exists(rt_int_s)) {
            clang_opts.extra_sources.push_back(rt_int_s.string());
        }
        if (std::filesystem::exists(rt_gdt)) {
            clang_opts.extra_sources.push_back(rt_gdt.string());
        }
        if (std::filesystem::exists(rt_paging)) {
            clang_opts.extra_sources.push_back(rt_paging.string());
        }
        if (std::filesystem::exists(rt_start)) {
            clang_opts.extra_sources.push_back(rt_start.string());
        }
        if (std::filesystem::exists(ld_script)) {
            clang_opts.ld_flags.push_back("-Wl,-T," + ld_script.string());
        }

        if (!toolchain::invoke_clang(
                cpp_path.string(),
                output_exe,
                clang_opts
            )) {
            std::cerr << "[cpp-backend] kernel (grub) clang invocation failed\n";
            return false;
        }
    } else {
        /* ---------------------------------------------
         * Invoke clang++
         * --------------------------------------------- */
        toolchain::ClangOptions clang_opts;
        clang_opts.debug = options.debug;
        clang_opts.optimize = options.optimize;
        clang_opts.opt_level = options.opt_level;
        clang_opts.verbose = options.verbose;

        clang_opts.libraries.push_back("stdc++");
        clang_opts.libraries.push_back("ssl");
        clang_opts.libraries.push_back("crypto");
        clang_opts.libraries.push_back("curl");

        std::filesystem::path work_dir_path = options.work_dir;
        if (std::filesystem::exists(work_dir_path / "vitte_runtime.hpp")) {
            clang_opts.include_dirs.push_back(work_dir_path.string());
        }

        if (!options.runtime_include.empty()) {
            std::filesystem::path p = options.runtime_include;
            if (std::filesystem::exists(p / "vitte_runtime.hpp")) {
                clang_opts.include_dirs.push_back(p.string());
            }
        }

        const char* openssl_dir = std::getenv("OPENSSL_DIR");
        if (openssl_dir && *openssl_dir) {
            std::filesystem::path base = openssl_dir;
            std::filesystem::path inc = base / "include";
            std::filesystem::path lib = base / "lib";
            if (std::filesystem::exists(inc)) {
                clang_opts.include_dirs.push_back(inc.string());
            }
            if (std::filesystem::exists(lib)) {
                clang_opts.library_dirs.push_back(lib.string());
            }
        }

        const char* curl_dir = std::getenv("CURL_DIR");
        if (curl_dir && *curl_dir) {
            std::filesystem::path base = curl_dir;
            std::filesystem::path inc = base / "include";
            std::filesystem::path lib = base / "lib";
            if (std::filesystem::exists(inc)) {
                clang_opts.include_dirs.push_back(inc.string());
            }
            if (std::filesystem::exists(lib)) {
                clang_opts.library_dirs.push_back(lib.string());
            }
        }

        const char* runtime_inc = std::getenv("VITTE_RUNTIME_INCLUDE");
        if (runtime_inc && *runtime_inc) {
            std::filesystem::path p = runtime_inc;
            if (std::filesystem::exists(p / "vitte_runtime.hpp")) {
                clang_opts.include_dirs.push_back(p.string());
            }
        } else {
            const char* root = std::getenv("VITTE_ROOT");
            std::filesystem::path base = root && *root
                ? std::filesystem::path(root)
                : std::filesystem::current_path();

            std::filesystem::path src_rt = base / "src/compiler/backends/runtime";
            if (std::filesystem::exists(src_rt / "vitte_runtime.hpp")) {
                clang_opts.include_dirs.push_back(src_rt.string());
            }

            std::filesystem::path tgt_inc = base / "target/include";
            if (std::filesystem::exists(tgt_inc / "vitte_runtime.hpp")) {
                clang_opts.include_dirs.push_back(tgt_inc.string());
            }
        }

        const char* root = std::getenv("VITTE_ROOT");
        std::filesystem::path base = root && *root
            ? std::filesystem::path(root)
            : std::filesystem::current_path();

        std::filesystem::path runtime_cpp = base / "src/compiler/backends/runtime/vitte_runtime.cpp";
        if (std::filesystem::exists(runtime_cpp)) {
            clang_opts.extra_sources.push_back(runtime_cpp.string());
        }

        if (!toolchain::invoke_clang(
                cpp_path.string(),
                output_exe,
                clang_opts
            )) {
            std::cerr << "[cpp-backend] clang invocation failed\n";
            return false;
        }
    }

    return true;
}

/* -------------------------------------------------
 * Emit MIR as C++ to an output stream
 * ------------------------------------------------- */
bool emit_cpp_backend(
    const vitte::ir::MirModule& mir_module,
    std::ostream& os,
    const CppBackendOptions& options
) {
    context::CppContext ctx = build_context(options);
    ast::cpp::CppTranslationUnit tu = lower_to_cpp(mir_module, ctx);
    emit::emit_translation_unit(os, tu, ctx);
    return true;
}

} // namespace vitte::backend
