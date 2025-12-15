#ifndef STEEL_PARSER_H
#define STEEL_PARSER_H

#include "ast.h"
#include "lexer.h"

/* Parse tokens into an AST. Caller owns returned AST and must call ast_free. */
ASTNode *parser_parse(Token *tokens);
void parser_free_ast(ASTNode *ast);

#endif
