// ============================================================
// archive.hpp — Vitte Compiler
// Static archive (.a) interface for linker
// ============================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vitte::linker {

// ------------------------------------------------------------
// Archive member
// ------------------------------------------------------------
//
// Représente un membre brut d’une archive statique.
// Aucune interprétation du contenu (ELF/Mach-O/COFF).
//

struct ArchiveMember {
    std::string name;
    std::vector<std::uint8_t> data;

    ArchiveMember(std::string name,
                  std::vector<std::uint8_t> data);
};

// ------------------------------------------------------------
// Archive
// ------------------------------------------------------------
//
// Format supporté :
//  - System V ar (UNIX .a)
//  - lecture / écriture simple
//
// Volontairement exclu :
//  - table des symboles
//  - index GNU/BSD
//  - résolution de symboles
//

class Archive {
public:
    Archive();

    // --------------------------------------------------------
    // I/O
    // --------------------------------------------------------

    bool load_from_file(const std::string& path,
                        std::string& error);

    bool write_to_file(const std::string& path,
                       std::string& error) const;

    // --------------------------------------------------------
    // Access
    // --------------------------------------------------------

    const std::vector<ArchiveMember>& members() const;

    void add_member(ArchiveMember member);

private:
    std::vector<ArchiveMember> members_;
};

} // namespace vitte::linker