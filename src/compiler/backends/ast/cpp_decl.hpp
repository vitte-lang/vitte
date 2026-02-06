#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cpp_expr.hpp"
#include "cpp_stmt.hpp"
#include "cpp_type.hpp"

namespace vitte::backend::ast::cpp {

/* ----------------------------------------
 * Visibility / linkage
 * ---------------------------------------- */
enum class Linkage {
    Internal,
    External
};

/* ----------------------------------------
 * Function parameter
 * ---------------------------------------- */
struct CppParam {
    CppType* type = nullptr;
    std::string name;
};

/* ----------------------------------------
 * Function declaration / definition
 * ---------------------------------------- */
struct CppFunction {
    std::string name;
    CppType* return_type = nullptr;
    std::vector<CppParam> params;
    std::vector<std::unique_ptr<CppStmt>> body;

    bool is_inline = false;
    bool is_extern = false;
    Linkage linkage = Linkage::External;
    std::optional<std::string> abi;
    std::optional<std::string> link_name;
};

/* ----------------------------------------
 * Global variable
 * ---------------------------------------- */
struct CppGlobal {
    CppType* type = nullptr;
    std::string name;
    std::optional<std::unique_ptr<CppExpr>> init;
    bool is_const = false;
    Linkage linkage = Linkage::External;
    bool is_extern = false;
    std::optional<std::string> abi;
    std::optional<std::string> link_name;
};

/* ----------------------------------------
 * Struct / class field
 * ---------------------------------------- */
struct CppField {
    CppType* type = nullptr;
    std::string name;
};

/* ----------------------------------------
 * Struct / class declaration
 * ---------------------------------------- */
struct CppStruct {
    std::string name;
    std::vector<CppField> fields;
    bool is_pod = true;
};

/* ----------------------------------------
 * Enum declaration
 * ---------------------------------------- */
struct CppEnumItem {
    std::string name;
    std::optional<int64_t> value;
};

struct CppEnum {
    std::string name;
    std::vector<CppEnumItem> items;
    bool scoped = true; // enum class
};

/* ----------------------------------------
 * Namespace
 * ---------------------------------------- */
struct CppNamespace {
    std::string name;
    std::vector<CppStruct> structs;
    std::vector<CppEnum> enums;
    std::vector<CppGlobal> globals;
    std::vector<CppFunction> functions;
};

/* ----------------------------------------
 * Translation unit (one .cpp / .hpp)
 * ---------------------------------------- */
struct CppTranslationUnit {
    std::vector<std::string> includes;
    std::vector<CppNamespace> namespaces;
    std::vector<CppStruct> structs;
    std::vector<CppEnum> enums;
    std::vector<CppGlobal> globals;
    std::vector<CppFunction> functions;
};

} // namespace vitte::backend::ast::cpp
