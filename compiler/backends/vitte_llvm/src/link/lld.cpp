#include <string>
#include <vector>

namespace vitte::llvm {

int linkWithLLD(const std::vector<std::string>& objects,
                const std::string& output) {
    // call lld::elf / lld::coff / lld::macho
    return 0;
}

}
