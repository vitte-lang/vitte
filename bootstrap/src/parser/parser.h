#ifndef VITTE_BOOTSTRAP_PARSER_H
#define VITTE_BOOTSTRAP_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../ast/ast.h"
#include "../diagnostic/diagnostic.h"
#include "../lexer/lexer.h"
#include "../module/module.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_PARSER_DEFAULT_MAX_DEPTH ((size_t)256u)

typedef struct vitte_parser_options {
    size_t max_depth;
    bool recover_errors;
    bool require_semicolons;
    vitte_lexer_options_t lexer_options;
} vitte_parser_options_t;

typedef struct vitte_parser_stats {
    size_t token_count;
    size_t decl_count;
    size_t stmt_count;
    size_t expr_count;
    size_t error_count;
    size_t recovery_count;
    size_t max_depth_reached;
} vitte_parser_stats_t;

typedef struct vitte_parser_result {
    vitte_status_t status;
    vitte_ast_module_t *root;
    size_t token_count;
    size_t decl_count;
    size_t stmt_count;
    size_t expr_count;
    size_t error_count;
    vitte_error_t last_error;
} vitte_parser_result_t;

typedef struct vitte_parser {
    bool initialized;
    vitte_module_t *module;
    vitte_ast_t *ast;
    vitte_diagnostic_bag_t *diagnostics;
    vitte_parser_options_t options;
    vitte_parser_stats_t stats;
    vitte_ast_builder_t builder;
    vitte_lexer_t lexer;
    vitte_token_t current;
    vitte_token_t previous;
    size_t depth;
    bool suppress_record_literals;
    vitte_error_t last_error;
} vitte_parser_t;

void vitte_parser_options_init(vitte_parser_options_t *options);
void vitte_parser_stats_init(vitte_parser_stats_t *stats);
void vitte_parser_result_init(vitte_parser_result_t *result);

vitte_status_t vitte_parser_init(
    vitte_parser_t *parser,
    vitte_ast_t *ast,
    const char *source_name,
    const char *source,
    size_t source_size,
    const vitte_parser_options_t *options,
    vitte_diagnostic_bag_t *diagnostics
);

vitte_status_t vitte_parser_init_module(
    vitte_parser_t *parser,
    vitte_module_t *module,
    vitte_ast_t *ast,
    const vitte_parser_options_t *options,
    vitte_diagnostic_bag_t *diagnostics
);

void vitte_parser_destroy(vitte_parser_t *parser);
bool vitte_parser_is_initialized(const vitte_parser_t *parser);
const vitte_error_t *vitte_parser_last_error(const vitte_parser_t *parser);
const vitte_parser_stats_t *vitte_parser_stats(const vitte_parser_t *parser);

vitte_status_t vitte_parser_parse_module(vitte_parser_t *parser, vitte_parser_result_t *result);
vitte_ast_decl_t *vitte_parser_parse_decl(vitte_parser_t *parser);
vitte_ast_stmt_t *vitte_parser_parse_stmt(vitte_parser_t *parser);
vitte_ast_expr_t *vitte_parser_parse_expr(vitte_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_PARSER_H */
