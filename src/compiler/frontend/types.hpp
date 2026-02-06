#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace vitte::frontend::types {

enum class TypeKind {
    Builtin,
    Named,
    Generic,
};

struct TypeInfo {
    TypeKind kind;
    std::string name;
};

using TypeId = std::size_t;

class TypeTable {
public:
    TypeId add_builtin(std::string name);
    TypeId add_named(std::string name);
    const TypeInfo* get(TypeId id) const;
    TypeId lookup(const std::string& name) const;
    const std::vector<TypeInfo>& all() const { return types_; }

private:
    std::vector<TypeInfo> types_;
    std::unordered_map<std::string, TypeId> by_name_;
};

} // namespace vitte::frontend::types
