#pragma once
#include <string>
#include <vector>
#include <optional>

namespace vitte::backend::ast::cpp {

/* ----------------------------------------
 * C++ type qualifiers
 * ---------------------------------------- */
enum class CppQualifier {
    Const,
    Volatile
};

/* ----------------------------------------
 * C++ type kind
 * ---------------------------------------- */
enum class CppTypeKind {
    Builtin,
    Pointer,
    Reference,
    Array,
    Function,
    Struct,
    Enum,
    Alias
};

/* ----------------------------------------
 * Base type representation
 * ---------------------------------------- */
struct CppType {
    CppTypeKind kind;

    /* Builtin / Struct / Enum / Alias */
    std::string name;

    /* Qualifiers */
    std::vector<CppQualifier> qualifiers;

    /* Pointer / Reference */
    CppType* pointee = nullptr;

    /* Array */
    std::optional<size_t> array_size;

    /* Function type */
    std::vector<CppType*> param_types;
    CppType* return_type = nullptr;

    explicit CppType(CppTypeKind k)
        : kind(k) {}

    /* ----------------------------------------
     * Factory helpers
     * ---------------------------------------- */
    static CppType builtin(std::string n) {
        CppType t(CppTypeKind::Builtin);
        t.name = std::move(n);
        return t;
    }

    static CppType pointer(CppType* base) {
        CppType t(CppTypeKind::Pointer);
        t.pointee = base;
        return t;
    }

    static CppType reference(CppType* base) {
        CppType t(CppTypeKind::Reference);
        t.pointee = base;
        return t;
    }

    static CppType array(CppType* base, size_t size) {
        CppType t(CppTypeKind::Array);
        t.pointee = base;
        t.array_size = size;
        return t;
    }

    static CppType function(
        CppType* ret,
        std::vector<CppType*> params
    ) {
        CppType t(CppTypeKind::Function);
        t.return_type = ret;
        t.param_types = std::move(params);
        return t;
    }

    static CppType user(std::string n, CppTypeKind k) {
        CppType t(k);
        t.name = std::move(n);
        return t;
    }

    /* ----------------------------------------
     * Qualifier helpers
     * ---------------------------------------- */
    void add_const() { qualifiers.push_back(CppQualifier::Const); }
    void add_volatile() { qualifiers.push_back(CppQualifier::Volatile); }

    bool is_const() const {
        for (auto q : qualifiers)
            if (q == CppQualifier::Const)
                return true;
        return false;
    }
};

} // namespace vitte::backend::ast::cpp
