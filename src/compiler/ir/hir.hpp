#pragma once
#include <string>
#include <vector>
#include <memory>

namespace vitte::ir {

/* =================================================
 * HIR – High-level Intermediate Representation
 * =================================================
 *
 * Le HIR est une représentation normalisée,
 * indépendante de la syntaxe source, mais encore
 * proche du langage (fonctions, blocs, expressions).
 */

/* ---------------------------------------------
 * HIR node kind
 * --------------------------------------------- */
enum class HirKind {
    Function,
    Block,
    Return,
    Literal,
    Variable
};

/* ---------------------------------------------
 * Base HIR node
 * --------------------------------------------- */
struct HirNode {
    HirKind kind;
    explicit HirNode(HirKind k) : kind(k) {}
    virtual ~HirNode() = default;
};

/* ---------------------------------------------
 * Expressions
 * --------------------------------------------- */
struct HirExpr : HirNode {
    using HirNode::HirNode;
};

struct HirLiteral : HirExpr {
    std::string value;

    explicit HirLiteral(std::string v)
        : HirExpr(HirKind::Literal),
          value(std::move(v)) {}
};

struct HirVariable : HirExpr {
    std::string name;

    explicit HirVariable(std::string n)
        : HirExpr(HirKind::Variable),
          name(std::move(n)) {}
};

/* ---------------------------------------------
 * Statements
 * --------------------------------------------- */
struct HirStmt : HirNode {
    using HirNode::HirNode;
};

struct HirReturn : HirStmt {
    std::unique_ptr<HirExpr> expr;

    explicit HirReturn(std::unique_ptr<HirExpr> e)
        : HirStmt(HirKind::Return),
          expr(std::move(e)) {}
};

/* ---------------------------------------------
 * Block
 * --------------------------------------------- */
struct HirBlock : HirNode {
    std::vector<std::unique_ptr<HirStmt>> stmts;

    HirBlock()
        : HirNode(HirKind::Block) {}
};

/* ---------------------------------------------
 * Function
 * --------------------------------------------- */
struct HirFunction : HirNode {
    std::string name;
    std::unique_ptr<HirBlock> body;

    explicit HirFunction(std::string n)
        : HirNode(HirKind::Function),
          name(std::move(n)),
          body(std::make_unique<HirBlock>()) {}
};

} // namespace vitte::ir
