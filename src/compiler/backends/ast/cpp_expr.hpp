#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace vitte::backend::ast::cpp {

/* ----------------------------------------
 * Forward declarations
 * ---------------------------------------- */
struct CppType;

/* ----------------------------------------
 * Expression base
 * ---------------------------------------- */
struct CppExpr {
    enum class Kind {
        Literal,
        Variable,
        Unary,
        Binary,
        Call,
        Cast,
        Member,
        Index
    };

    Kind kind;
    explicit CppExpr(Kind k) : kind(k) {}
    virtual ~CppExpr() = default;
};

/* ----------------------------------------
 * Literals
 * ---------------------------------------- */
struct CppLiteral : CppExpr {
    std::string value; // already formatted C++ literal

    explicit CppLiteral(std::string v)
        : CppExpr(Kind::Literal), value(std::move(v)) {}
};

/* ----------------------------------------
 * Variable reference
 * ---------------------------------------- */
struct CppVar : CppExpr {
    std::string name;

    explicit CppVar(std::string n)
        : CppExpr(Kind::Variable), name(std::move(n)) {}
};

/* ----------------------------------------
 * Unary expression
 * ---------------------------------------- */
struct CppUnary : CppExpr {
    std::string op;
    std::unique_ptr<CppExpr> expr;

    CppUnary(std::string o, std::unique_ptr<CppExpr> e)
        : CppExpr(Kind::Unary), op(std::move(o)), expr(std::move(e)) {}
};

/* ----------------------------------------
 * Binary expression
 * ---------------------------------------- */
struct CppBinary : CppExpr {
    std::string op;
    std::unique_ptr<CppExpr> lhs;
    std::unique_ptr<CppExpr> rhs;

    CppBinary(
        std::string o,
        std::unique_ptr<CppExpr> l,
        std::unique_ptr<CppExpr> r
    )
        : CppExpr(Kind::Binary),
          op(std::move(o)),
          lhs(std::move(l)),
          rhs(std::move(r)) {}
};

/* ----------------------------------------
 * Function / method call
 * ---------------------------------------- */
struct CppCall : CppExpr {
    std::string callee;
    std::vector<std::unique_ptr<CppExpr>> args;

    explicit CppCall(std::string c)
        : CppExpr(Kind::Call), callee(std::move(c)) {}
};

/* ----------------------------------------
 * Cast expression
 * ---------------------------------------- */
struct CppCast : CppExpr {
    CppType* target_type;
    std::unique_ptr<CppExpr> expr;

    CppCast(CppType* t, std::unique_ptr<CppExpr> e)
        : CppExpr(Kind::Cast), target_type(t), expr(std::move(e)) {}
};

/* ----------------------------------------
 * Member access (a.b or a->b)
 * ---------------------------------------- */
struct CppMember : CppExpr {
    std::unique_ptr<CppExpr> base;
    std::string member;
    bool pointer = false; // true => -> , false => .

    CppMember(
        std::unique_ptr<CppExpr> b,
        std::string m,
        bool ptr = false
    )
        : CppExpr(Kind::Member),
          base(std::move(b)),
          member(std::move(m)),
          pointer(ptr) {}
};

/* ----------------------------------------
 * Indexing (a[i])
 * ---------------------------------------- */
struct CppIndex : CppExpr {
    std::unique_ptr<CppExpr> base;
    std::unique_ptr<CppExpr> index;

    CppIndex(
        std::unique_ptr<CppExpr> b,
        std::unique_ptr<CppExpr> i
    )
        : CppExpr(Kind::Index),
          base(std::move(b)),
          index(std::move(i)) {}
};

} // namespace vitte::backend::ast::cpp
