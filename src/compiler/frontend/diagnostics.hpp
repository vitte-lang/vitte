#pragma once
#include <string>

namespace vitte::frontend {

void error(const std::string& msg);
void warning(const std::string& msg);
void note(const std::string& msg);

}
