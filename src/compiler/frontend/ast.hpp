#pragma once
#include <string>
#include <vector>
#include <memory>

namespace vitte::frontend {

/* =================================================
 * AST – Abstract Syntax Tree (frontend)
 * =================================================
 *
 * Ce fichier définit un AST volontairement simple,
 * servant de base au frontend Vitte.
 * Il est destiné à être enrichi progressivement
 * (types, spans, visitors, lowering HIR).
 */

/* ---------------------------------------------
 * AST node kind
 * --------------------------------------------- */
enum class AstKind {
    Program,
    Function,
    Block,
    Return,
    Literal,
    Identifier
};

/* ---------------------------------------------
 * Base AST node
 * --------------------------------------------- */
struct AstNode {
    AstKind kind;
    explicit AstNode(AstKind k) : kind(k) {}
    virtual ~AstNode() = default;
};

/* ---------------------------------------------
 * Program (root)
 * --------------------------------------------- */
struct AstProgram : AstNode {
    std::vector<std::unique_ptr<AstNode>> items;

    AstProgram()
        : AstNode(AstKind::Program) {}
};

/* ---------------------------------------------
 * Identifier
 * --------------------------------------------- */
struct AstIdentifier : AstNode {
    std::string name;

    explicit AstIdentifier(std::string n)
        : AstNode(AstKind::Identifier),
          name(std::move(n)) {}
};

/* ---------------------------------------------
 * Literal
 * --------------------------------------------- */
struct AstLiteral : AstNode {
    std::string value; // textual literal

    explicit AstLiteral(std::string v)
        : AstNode(AstKind::Literal),
          value(std::move(v)) {}
};

/* ---------------------------------------------
 * Return statement
 * --------------------------------------------- */
struct AstReturn : AstNode {
    std::unique_ptr<AstNode> expr;

    explicit AstReturn(std::unique_ptr<AstNode> e)
        : AstNode(AstKind::Return),
          expr(std::move(e)) {}
};

/* ---------------------------------------------
 * Block
 * --------------------------------------------- */
struct AstBlock : AstNode {
    std::vector<std::unique_ptr<AstNode>> stmts;

    AstBlock()
        : AstNode(AstKind::Block) {}
};

/* ---------------------------------------------
 * Function
 * --------------------------------------------- */
struct AstFunction : AstNode {
    std::string name;
    std::unique_ptr<AstBlock> body;

    explicit AstFunction(std::string n)
        : AstNode(AstKind::Function),
          name(std::move(n)),
          body(std::make_unique<AstBlock>()) {}
};

} // namespace vitte::frontend
