#include "types.hpp"

namespace vitte::frontend::types {

TypeId TypeTable::add_builtin(std::string name) {
    auto it = by_name_.find(name);
    if (it != by_name_.end()) {
        return it->second;
    }
    TypeId id = types_.size();
    types_.push_back({TypeKind::Builtin, std::move(name)});
    by_name_[types_.back().name] = id;
    return id;
}

TypeId TypeTable::add_named(std::string name) {
    auto it = by_name_.find(name);
    if (it != by_name_.end()) {
        return it->second;
    }
    TypeId id = types_.size();
    types_.push_back({TypeKind::Named, std::move(name)});
    by_name_[types_.back().name] = id;
    return id;
}

const TypeInfo* TypeTable::get(TypeId id) const {
    if (id >= types_.size()) {
        return nullptr;
    }
    return &types_[id];
}

TypeId TypeTable::lookup(const std::string& name) const {
    auto it = by_name_.find(name);
    if (it == by_name_.end()) {
        return static_cast<TypeId>(-1);
    }
    return it->second;
}

} // namespace vitte::frontend::types
