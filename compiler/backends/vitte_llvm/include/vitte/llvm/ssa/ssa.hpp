#pragma once
#include <vector>
#include <string>

namespace vitte::llvm::ssa {

struct Value {
    int id;
    std::string ty;
};

struct Block {
    int id;
    std::vector<Value> values;
};

struct Function {
    std::string name;
    std::vector<Block> blocks;
};

}
