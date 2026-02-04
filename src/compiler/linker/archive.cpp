// ============================================================
// archive.cpp — Vitte Compiler
// Static archive (.a) handling for linker
// ============================================================

#include "archive.hpp"

#include <cassert>
#include <cstring>
#include <fstream>
#include <sstream>

namespace vitte::linker {

// ------------------------------------------------------------
// Constants (ar format)
// ------------------------------------------------------------
//
// Reference: System V ar format
//

static constexpr const char* AR_MAGIC = "!<arch>\n";
static constexpr std::size_t AR_MAGIC_LEN = 8;

static constexpr std::size_t AR_HEADER_SIZE = 60;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && s[start] == ' ') {
        ++start;
    }

    std::size_t end = s.size();
    while (end > start && s[end - 1] == ' ') {
        --end;
    }

    return s.substr(start, end - start);
}

static std::string read_field(
    const char* buf,
    std::size_t offset,
    std::size_t size)
{
    return trim(std::string(buf + offset, size));
}

// ------------------------------------------------------------
// ArchiveMember
// ------------------------------------------------------------

ArchiveMember::ArchiveMember(
    std::string n,
    std::vector<std::uint8_t> d)
    : name(std::move(n)),
      data(std::move(d)) {}

// ------------------------------------------------------------
// Archive
// ------------------------------------------------------------

Archive::Archive() = default;

// ------------------------------------------------------------
// Reading
// ------------------------------------------------------------

bool Archive::load_from_file(const std::string& path,
                             std::string& error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "unable to open archive: " + path;
        return false;
    }

    char magic[AR_MAGIC_LEN];
    in.read(magic, AR_MAGIC_LEN);
    if (in.gcount() != AR_MAGIC_LEN ||
        std::strncmp(magic, AR_MAGIC, AR_MAGIC_LEN) != 0) {
        error = "invalid archive magic";
        return false;
    }

    while (in) {
        char header[AR_HEADER_SIZE];
        in.read(header, AR_HEADER_SIZE);
        if (in.gcount() == 0) {
            break; // end of archive
        }
        if (in.gcount() != AR_HEADER_SIZE) {
            error = "truncated archive header";
            return false;
        }

        std::string name = read_field(header, 0, 16);
        std::string size_str = read_field(header, 48, 10);

        std::size_t size = 0;
        try {
            size = static_cast<std::size_t>(std::stoul(size_str));
        } catch (...) {
            error = "invalid member size";
            return false;
        }

        std::vector<std::uint8_t> data(size);
        in.read(reinterpret_cast<char*>(data.data()), size);
        if (static_cast<std::size_t>(in.gcount()) != size) {
            error = "truncated archive member";
            return false;
        }

        // members are 2-byte aligned
        if (size & 1) {
            in.get();
        }

        members_.emplace_back(
            std::move(name),
            std::move(data));
    }

    return true;
}

// ------------------------------------------------------------
// Writing
// ------------------------------------------------------------

bool Archive::write_to_file(const std::string& path,
                            std::string& error) const
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "unable to create archive: " + path;
        return false;
    }

    out.write(AR_MAGIC, AR_MAGIC_LEN);

    for (const auto& m : members_) {
        char header[AR_HEADER_SIZE];
        std::memset(header, ' ', AR_HEADER_SIZE);

        auto write_field = [&](std::size_t off,
                               std::size_t len,
                               const std::string& v) {
            std::memcpy(header + off,
                        v.c_str(),
                        std::min(len, v.size()));
        };

        write_field(0, 16, m.name);
        write_field(48, 10, std::to_string(m.data.size()));
        write_field(58, 2, "`\n");

        out.write(header, AR_HEADER_SIZE);
        out.write(reinterpret_cast<const char*>(m.data.data()),
                  m.data.size());

        if (m.data.size() & 1) {
            out.put('\n');
        }
    }

    return true;
}

// ------------------------------------------------------------
// Accessors
// ------------------------------------------------------------

const std::vector<ArchiveMember>& Archive::members() const {
    return members_;
}

void Archive::add_member(ArchiveMember member) {
    members_.push_back(std::move(member));
}

} // namespace vitte::linker