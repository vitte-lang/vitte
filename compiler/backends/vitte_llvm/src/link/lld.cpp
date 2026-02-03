// ============================================================
// vitte::llvm::link::lld
// Édition de liens via LLVM lld
// ============================================================

#include "vitte/llvm/context.hpp"

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <lld/Common/Driver.h>

#include <vector>
#include <string>
#include <system_error>

namespace vitte::llvm::link {


// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

enum class LinkKind {
    Executable,
    SharedLib,
    StaticLib
};


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static std::string default_linker_flavor()
{
#if defined(_WIN32)
    return "link";      // COFF
#elif defined(__APPLE__)
    return "darwin";    // Mach-O
#else
    return "elf";       // ELF
#endif
}


static std::string default_output_name(LinkKind kind)
{
#if defined(_WIN32)
    return (kind == LinkKind::Executable) ? "a.exe" : "a.dll";
#elif defined(__APPLE__)
    return (kind == LinkKind::Executable) ? "a.out" : "liba.dylib";
#else
    return (kind == LinkKind::Executable) ? "a.out" : "liba.so";
#endif
}


// ------------------------------------------------------------
// API principale
// ------------------------------------------------------------

bool link_with_lld(const vitte::llvm::Context& ctx,
                   const std::vector<std::string>& objects,
                   const std::vector<std::string>& libraries,
                   const std::string& output_path,
                   LinkKind kind)
{
    // --------------------------------------------------------
    // Préconditions
    // --------------------------------------------------------

    if (objects.empty()) {
        ::llvm::errs()
            << "[vitte-llvm][lld] no object files provided\n";
        return false;
    }

    // --------------------------------------------------------
    // Construction des arguments LLD
    // --------------------------------------------------------

    std::vector<const char*> args;

    // argv[0] : nom du programme
    args.push_back("lld");

    // Type de sortie
    if (kind == LinkKind::SharedLib) {
#if !defined(_WIN32)
        args.push_back("-shared");
#endif
    }

    // Fichier de sortie
    args.push_back("-o");
    args.push_back(output_path.c_str());

    // Objets
    for (const auto& obj : objects) {
        args.push_back(obj.c_str());
    }

    // Bibliothèques
    for (const auto& lib : libraries) {
        args.push_back(lib.c_str());
    }

    // --------------------------------------------------------
    // Sélection du flavor
    // --------------------------------------------------------

    const std::string flavor = default_linker_flavor();

    bool ok = false;

    if (flavor == "elf") {
        ok = lld::elf::link(
            args,
            /*canExit=*/false,
            ::llvm::outs(),
            ::llvm::errs()
        );
    }
#if defined(__APPLE__)
    else if (flavor == "darwin") {
        ok = lld::mach_o::link(
            args,
            /*canExit=*/false,
            ::llvm::outs(),
            ::llvm::errs()
        );
    }
#endif
#if defined(_WIN32)
    else if (flavor == "link") {
        ok = lld::coff::link(
            args,
            /*canExit=*/false,
            ::llvm::outs(),
            ::llvm::errs()
        );
    }
#endif
    else {
        ::llvm::errs()
            << "[vitte-llvm][lld] unsupported linker flavor\n";
        return false;
    }

    if (!ok) {
        ::llvm::errs()
            << "[vitte-llvm][lld] linking failed\n";
        return false;
    }

    return true;
}

} // namespace vitte::llvm::link
