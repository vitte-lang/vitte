#pragma once
#include <string>
#include <vector>

namespace vitte::linker {

std::vector<std::string> collect_objects(const std::string& dir);

}