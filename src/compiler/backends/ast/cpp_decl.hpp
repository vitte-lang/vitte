#pragma once
#include <string>
#include <vector>
#include <optional>

namespace vitte::backend::ast::cpp {

/* ----------------------------------------
 * Forward declarations
 * ---------------------------------------- */
struct CppType;
struct CppExpr;
struct CppStmt;

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
    CppType type;
    std::string name;
};

/* ----------------------------------------
 * Function declaration / definition
 * ---------------------------------------- */
struct CppFunction {
    std::string name;
    CppType return_type;
    std::vector<CppParam> params;
    std::vector<CppStmt> body;

    bool is_inline = false;
    bool is_extern = false;
    Linkage linkage = Linkage::External;
};

/* ----------------------------------------
 * Global variable
 * ---------------------------------------- */
struct CppGlobal {
    CppType type;
    std::string name;
    std::optional<CppExpr> init;
    bool is_const = false;
    Linkage linkage = Linkage::External;
};

/* ----------------------------------------
 * Struct / class field
 * ---------------------------------------- */
struct CppField {
    CppType type;
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
