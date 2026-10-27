#ifndef VITTE_HIR_LOWER_H
#define VITTE_HIR_LOWER_H

#include "hir.h"
#include "parser/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lower one validated parser AST into a sealed, validated HIR context. */
bool vitte_hir_lower_ast(vitte_hir_t *hir, const vitte_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif
