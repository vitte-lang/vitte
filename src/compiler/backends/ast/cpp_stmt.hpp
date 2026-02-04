#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace vitte::backend::ast::cpp {

/* ----------------------------------------
 * Forward declarations
 * ---------------------------------------- */
struct CppExpr;
struct CppType;

/* ----------------------------------------
 * Statement base
 * ---------------------------------------- */
struct CppStmt {
    enum class Kind {
        Expr,
        Decl,
        Assign,
        Return,
        If,
        While,
        For,
        Break,
        Continue,
        Block
    };

    Kind kind;
    explicit CppStmt(Kind k) : kind(k) {}
    virtual ~CppStmt() = default;
};

/* ----------------------------------------
 * Expression statement (e.g. call;)
 * ---------------------------------------- */
struct CppExprStmt : CppStmt {
    std::unique_ptr<CppExpr> expr;

    explicit CppExprStmt(std::unique_ptr<CppExpr> e)
        : CppStmt(Kind::Expr), expr(std::move(e)) {}
};

/* ----------------------------------------
 * Variable declaration (type name = init;)
 * ---------------------------------------- */
struct CppVarDecl : CppStmt {
    CppType* type;
    std::string name;
    std::optional<std::unique_ptr<CppExpr>> init;
    bool is_const = false;

    CppVarDecl(CppType* t, std::string n)
        : CppStmt(Kind::Decl), type(t), name(std::move(n)) {}
};

/* ----------------------------------------
 * Assignment (lhs = rhs;)
 * ---------------------------------------- */
struct CppAssign : CppStmt {
    std::unique_ptr<CppExpr> lhs;
    std::unique_ptr<CppExpr> rhs;

    CppAssign(
        std::unique_ptr<CppExpr> l,
        std::unique_ptr<CppExpr> r
    )
        : CppStmt(Kind::Assign),
          lhs(std::move(l)),
          rhs(std::move(r)) {}
};

/* ----------------------------------------
 * Return statement
 * ---------------------------------------- */
struct CppReturn : CppStmt {
    std::optional<std::unique_ptr<CppExpr>> value;

    CppReturn()
        : CppStmt(Kind::Return) {}

    explicit CppReturn(std::unique_ptr<CppExpr> v)
        : CppStmt(Kind::Return), value(std::move(v)) {}
};

/* ----------------------------------------
 * If / Else
 * ---------------------------------------- */
struct CppIf : CppStmt {
    std::unique_ptr<CppExpr> condition;
    std::vector<std::unique_ptr<CppStmt>> then_body;
    std::vector<std::unique_ptr<CppStmt>> else_body;

    explicit CppIf(std::unique_ptr<CppExpr> cond)
        : CppStmt(Kind::If), condition(std::move(cond)) {}
};

/* ----------------------------------------
 * While loop
 * ---------------------------------------- */
struct CppWhile : CppStmt {
    std::unique_ptr<CppExpr> condition;
    std::vector<std::unique_ptr<CppStmt>> body;

    explicit CppWhile(std::unique_ptr<CppExpr> cond)
        : CppStmt(Kind::While), condition(std::move(cond)) {}
};

/* ----------------------------------------
 * For loop (C-style)
 * ---------------------------------------- */
struct CppFor : CppStmt {
    std::unique_ptr<CppStmt> init;
    std::unique_ptr<CppExpr> condition;
    std::unique_ptr<CppStmt> step;
    std::vector<std::unique_ptr<CppStmt>> body;

    CppFor()
        : CppStmt(Kind::For) {}
};

/* ----------------------------------------
 * Break
 * ---------------------------------------- */
struct CppBreak : CppStmt {
    CppBreak() : CppStmt(Kind::Break) {}
};

/* ----------------------------------------
 * Continue
 * ---------------------------------------- */
struct CppContinue : CppStmt {
    CppContinue() : CppStmt(Kind::Continue) {}
};

/* ----------------------------------------
 * Block { ... }
 * ---------------------------------------- */
struct CppBlock : CppStmt {
    std::vector<std::unique_ptr<CppStmt>> stmts;

    CppBlock()
        : CppStmt(Kind::Block) {}
};

} // namespace vitte::backend::ast::cpp
