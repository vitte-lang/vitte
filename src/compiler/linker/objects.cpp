#include "objects.hpp"
#include <filesystem>

namespace vitte::linker {

std::vector<std::string> collect_objects(const std::string& dir) {
    std::vector<std::string> objs;
    for (auto& e : std::filesystem::directory_iterator(dir)) {
        if (e.path().extension() == ".o")
            objs.push_back(e.path().string());
    }
    return objs;
}

}