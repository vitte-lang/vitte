#pragma once
#include <string>
#include <vector>

namespace vitte::linker {

enum class LinkerMode {
    Archive,
    Executable
};

struct LinkerOptions {
    LinkerMode mode = LinkerMode::Archive;

    std::string output;
    std::vector<std::string> objects;

    bool prefer_llvm = true;
    bool verbose = false;
};

bool run_linker(const LinkerOptions& opts);

}
