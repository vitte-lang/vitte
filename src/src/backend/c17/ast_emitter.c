#if defined(__APPLE__) || defined(__unix__)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "ast_emitter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) || defined(__unix__)
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
#endif

typedef struct vitte_c17_ast_emitter {
    FILE *output;
    const vitte_parser_t *parser;
    const vitte_ast_node_t *function;
    const vitte_ast_node_t *space_stack[128];
    size_t space_depth;
    const char *source_path;
    bool debug_info;
    bool debug_runtime;
    bool emit_line_directives;
    struct {
        const char *lexeme;
        size_t length;
    } debug_scalar_names[128];
    size_t debug_scalar_name_count;
    bool failed;
} vitte_c17_ast_emitter_t;

static const vitte_ast_node_t *
vitte_c17_ast_node(
    const vitte_c17_ast_emitter_t *emitter,
    vitte_ast_node_id_t id)
{
    return vitte_parser_get_node(emitter->parser, id);
}

static const vitte_token_t *
vitte_c17_ast_token(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    if (node == NULL ||
        node->token_index >= emitter->parser->token_count) {
        return NULL;
    }

    return &emitter->parser->tokens[node->token_index];
}

static void
vitte_c17_ast_write(
    vitte_c17_ast_emitter_t *emitter,
    const char *text)
{
    if (!emitter->failed &&
        fputs(text, emitter->output) == EOF) {
        emitter->failed = true;
    }
}

static void
vitte_c17_ast_emit_source_line(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_token_t *token;
    size_t line;
    size_t index;

    if (emitter == NULL ||
        !emitter->emit_line_directives ||
        emitter->source_path == NULL ||
        emitter->source_path[0] == '\0' ||
        node == NULL) {
        return;
    }

    line = node->span.line;
    token = vitte_c17_ast_token(emitter, node);
    if (line == 0u && token != NULL) {
        line = token->line;
    }
    if (line == 0u) {
        return;
    }

    if (fprintf(
            emitter->output,
            "#line %zu \"",
            line) < 0) {
        emitter->failed = true;
        return;
    }

    for (index = 0u; emitter->source_path[index] != '\0'; ++index) {
        unsigned char character;

        character = (unsigned char)emitter->source_path[index];
        if (character == '\\' || character == '"') {
            if (fputc('\\', emitter->output) == EOF) {
                emitter->failed = true;
                return;
            }
        } else if (character == '\n') {
            if (fputs("\\n", emitter->output) == EOF) {
                emitter->failed = true;
                return;
            }
            continue;
        } else if (character == '\r') {
            if (fputs("\\r", emitter->output) == EOF) {
                emitter->failed = true;
                return;
            }
            continue;
        }
        if (fputc((int)character, emitter->output) == EOF) {
            emitter->failed = true;
            return;
        }
    }

    if (fputs("\"\n", emitter->output) == EOF) {
        emitter->failed = true;
    }
}

static void
vitte_c17_ast_emit_debug_runtime(
    vitte_c17_ast_emitter_t *emitter)
{
    vitte_c17_ast_write(
        emitter,
        "#if defined(VITTE_DEBUG_RUNTIME) && "
        "(defined(__APPLE__) || defined(__unix__))\n"
        "typedef struct vitte_debug_value {\n"
        "    bool active;\n"
        "    char function[64];\n"
        "    char name[64];\n"
        "    int64_t value;\n"
        "} vitte_debug_value_t;\n"
        "static vitte_debug_value_t vitte_debug_values[128];\n"
        "static int vitte_debug_socket = -1;\n"
        "static FILE *vitte_debug_stream = NULL;\n"
        "static void vitte_debug_copy_name(\n"
        "    char *target, size_t capacity, const char *source) {\n"
        "    size_t length;\n"
        "    if (target == NULL || capacity == 0u) { return; }\n"
        "    if (source == NULL) { target[0] = '\\0'; return; }\n"
        "    length = strlen(source);\n"
        "    if (length >= capacity) { length = capacity - 1u; }\n"
        "    memcpy(target, source, length);\n"
        "    target[length] = '\\0';\n"
        "}\n");
    vitte_c17_ast_write(
        emitter,
        "static void vitte_debug_set_i64(\n"
        "    const char *function, const char *name, int64_t value) {\n"
        "    size_t index;\n"
        "    vitte_debug_value_t *slot;\n"
        "    slot = NULL;\n"
        "    for (index = 0u; index < 128u; ++index) {\n"
        "        if (vitte_debug_values[index].active &&\n"
        "            strcmp(vitte_debug_values[index].function,\n"
        "                   function != NULL ? function : \"main\") == 0 &&\n"
        "            strcmp(vitte_debug_values[index].name,\n"
        "                   name != NULL ? name : \"value\") == 0) {\n"
        "            slot = &vitte_debug_values[index];\n"
        "            break;\n"
        "        }\n"
        "    }\n"
        "    if (slot == NULL) {\n"
        "        for (index = 0u; index < 128u; ++index) {\n"
        "            if (!vitte_debug_values[index].active) {\n"
        "                slot = &vitte_debug_values[index];\n"
        "                slot->active = true;\n"
        "                break;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    if (slot == NULL) { return; }\n"
        "    vitte_debug_copy_name(slot->function, sizeof(slot->function),\n"
        "                          function != NULL ? function : \"main\");\n"
        "    vitte_debug_copy_name(slot->name, sizeof(slot->name),\n"
        "                          name != NULL ? name : \"value\");\n"
        "    slot->value = value;\n"
        "}\n"
        "static void vitte_debug_send_values(const char *function) {\n"
        "    size_t index;\n"
        "    const char *current;\n"
        "    current = function != NULL ? function : \"main\";\n"
        "    if (fprintf(vitte_debug_stream, \"vars %s\\n\", current) < 0) {\n"
        "        return;\n"
        "    }\n"
        "    for (index = 0u; index < 128u; ++index) {\n"
        "        if (vitte_debug_values[index].active &&\n"
        "            strcmp(vitte_debug_values[index].function, current) == 0) {\n"
        "            (void)fprintf(vitte_debug_stream, \"var %s %\" PRId64 \"\\n\",\n"
        "                          vitte_debug_values[index].name,\n"
        "                          vitte_debug_values[index].value);\n"
        "        }\n"
        "    }\n"
        "}\n"
        "static bool vitte_debug_connect(void) {\n"
        "    const char *port_text;\n"
        "    char *port_end;\n"
        "    long port;\n"
        "    struct sockaddr_in address;\n"
        "    int socket_fd;\n"
        "    port_text = getenv(\"VITTE_DEBUG_PORT\");\n"
        "    if (port_text == NULL) { return false; }\n"
        "    port = strtol(port_text, &port_end, 10);\n"
        "    if (port_end == port_text || *port_end != '\\0' ||\n"
        "        port < 1L || port > 65535L) { return false; }\n"
        "    socket_fd = socket(AF_INET, SOCK_STREAM, 0);\n"
        "    if (socket_fd < 0) { return false; }\n"
        "    memset(&address, 0, sizeof(address));\n"
        "    address.sin_family = AF_INET;\n"
        "    address.sin_addr.s_addr = htonl(UINT32_C(0x7f000001));\n"
        "    address.sin_port = htons((uint16_t)port);\n"
        "    if (connect(socket_fd, (struct sockaddr *)&address,\n"
        "                sizeof(address)) != 0) {\n"
        "        (void)close(socket_fd);\n"
        "        return false;\n"
        "    }\n"
        "    vitte_debug_stream = fdopen(socket_fd, \"r+\");\n"
        "    if (vitte_debug_stream == NULL) {\n"
        "        (void)close(socket_fd);\n"
        "        return false;\n"
        "    }\n"
        "    vitte_debug_socket = socket_fd;\n"
        "    (void)setvbuf(vitte_debug_stream, NULL, _IOLBF, 0);\n"
        "    return true;\n"
        "}\n"
        "static void vitte_debug_probe(size_t line, const char *function) {\n"
        "    char message[96];\n"
        "    if (vitte_debug_stream == NULL && !vitte_debug_connect()) {\n"
        "        return;\n"
        "    }\n"
        "    vitte_debug_send_values(function);\n"
        "    if (fprintf(vitte_debug_stream, \"line %zu %s\\n\", line,\n"
        "                function != NULL ? function : \"main\") < 0 ||\n"
        "        fflush(vitte_debug_stream) != 0) {\n"
        "        (void)fclose(vitte_debug_stream);\n"
        "        vitte_debug_stream = NULL;\n"
        "        vitte_debug_socket = -1;\n"
        "        return;\n"
        "    }\n"
        "    while (fgets(message, sizeof(message), vitte_debug_stream) != NULL) {\n"
        "        if (strncmp(message, \"continue\", 8u) == 0) {\n"
        "            return;\n"
        "        }\n"
        "        if (strncmp(message, \"stop\", 4u) == 0) {\n"
        "            while (fgets(message, sizeof(message), vitte_debug_stream) != NULL) {\n"
        "                if (strncmp(message, \"continue\", 8u) == 0) {\n"
        "                    return;\n"
        "                }\n"
        "            }\n"
        "            return;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "#else\n"
        "static void vitte_debug_set_i64(\n"
        "    const char *function, const char *name, int64_t value) {\n"
        "    (void)function;\n"
        "    (void)name;\n"
        "    (void)value;\n"
        "}\n"
        "static void vitte_debug_probe(size_t line, const char *function) {\n"
        "    (void)line;\n"
        "    (void)function;\n"
        "}\n"
        "#endif\n\n");
}

static void
vitte_c17_ast_write_token(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_token_t *token;

    token = vitte_c17_ast_token(emitter, node);
    if (token == NULL ||
        token->lexeme == NULL ||
        emitter->failed) {
        emitter->failed = true;
        return;
    }

    if (fwrite(token->lexeme, 1u, token->length, emitter->output) !=
        token->length) {
        emitter->failed = true;
    }
}

static bool
vitte_c17_ast_token_is(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    const char *text)
{
    const vitte_token_t *token;
    size_t length;

    token = vitte_c17_ast_token(emitter, node);
    if (token == NULL || text == NULL) {
        return false;
    }

    length = strlen(text);
    return token->length == length &&
           memcmp(token->lexeme, text, length) == 0;
}

static const vitte_ast_node_t *
vitte_c17_ast_child(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    size_t index)
{
    if (node == NULL || index >= node->child_count) {
        return NULL;
    }

    return vitte_c17_ast_node(emitter, node->children[index]);
}

static void
vitte_c17_ast_emit_expression(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node);

static bool
vitte_c17_ast_is_string_literal_expression(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    if (node == NULL) {
        return false;
    }

    if (node->kind == VITTE_AST_NODE_STRING_LITERAL) {
        return true;
    }
    if (node->kind == VITTE_AST_NODE_GROUP_EXPR) {
        return vitte_c17_ast_is_string_literal_expression(
            emitter,
            vitte_c17_ast_child(emitter, node, 0u));
    }
    return node->kind == VITTE_AST_NODE_BINARY_EXPR &&
           node->operator_kind == VITTE_TOKEN_PLUS &&
           vitte_c17_ast_is_string_literal_expression(
               emitter,
               vitte_c17_ast_child(emitter, node, 0u)) &&
           vitte_c17_ast_is_string_literal_expression(
               emitter,
               vitte_c17_ast_child(emitter, node, 1u));
}

static void
vitte_c17_ast_emit_string_literal_sequence(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    if (node == NULL) {
        emitter->failed = true;
        return;
    }

    if (node->kind == VITTE_AST_NODE_GROUP_EXPR) {
        vitte_c17_ast_emit_string_literal_sequence(
            emitter,
            vitte_c17_ast_child(emitter, node, 0u));
    } else if (node->kind == VITTE_AST_NODE_BINARY_EXPR &&
               node->operator_kind == VITTE_TOKEN_PLUS) {
        vitte_c17_ast_emit_string_literal_sequence(
            emitter,
            vitte_c17_ast_child(emitter, node, 0u));
        vitte_c17_ast_write(emitter, " ");
        vitte_c17_ast_emit_string_literal_sequence(
            emitter,
            vitte_c17_ast_child(emitter, node, 1u));
    } else if (node->kind == VITTE_AST_NODE_STRING_LITERAL) {
        vitte_c17_ast_write_token(emitter, node);
    } else {
        emitter->failed = true;
    }
}

static void
vitte_c17_ast_emit_condition(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node);

static void
vitte_c17_ast_emit_path(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node);

static bool
vitte_c17_ast_token_equal(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *left,
    const vitte_ast_node_t *right)
{
    const vitte_token_t *left_token;
    const vitte_token_t *right_token;

    left_token = vitte_c17_ast_token(emitter, left);
    right_token = vitte_c17_ast_token(emitter, right);
    return left_token != NULL &&
           right_token != NULL &&
           left_token->length == right_token->length &&
           memcmp(
               left_token->lexeme,
               right_token->lexeme,
               left_token->length) == 0;
}

static void
vitte_c17_ast_emit_function_name(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *name)
{
    size_t space_index;

    for (space_index = 0u;
         space_index < emitter->space_depth;
         ++space_index) {
        const vitte_ast_node_t *space_path;

        space_path =
            vitte_c17_ast_child(
                emitter,
                emitter->space_stack[space_index],
                0u);
        if (space_path == NULL ||
            space_path->kind != VITTE_AST_NODE_PATH ||
            space_path->child_count == 0u) {
            emitter->failed = true;
            return;
        }
        vitte_c17_ast_emit_path(emitter, space_path);
        vitte_c17_ast_write(emitter, "_");
    }
    vitte_c17_ast_write_token(emitter, name);
}

static bool
vitte_c17_ast_space_declares_function(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *space,
    const vitte_ast_node_t *name)
{
    const vitte_ast_node_t *body;
    size_t index;

    body = vitte_c17_ast_child(emitter, space, 1u);
    if (body == NULL ||
        body->kind != VITTE_AST_NODE_BLOCK) {
        return false;
    }
    for (index = 0u; index < body->child_count; ++index) {
        const vitte_ast_node_t *declaration;

        declaration = vitte_c17_ast_child(emitter, body, index);
        if (declaration == NULL ||
            (declaration->kind != VITTE_AST_NODE_PROC_DECL &&
             declaration->kind != VITTE_AST_NODE_EXTERN_PROC_DECL)) {
            continue;
        }
        if (vitte_c17_ast_token_equal(
                emitter,
                vitte_c17_ast_child(emitter, declaration, 0u),
                name)) {
            return true;
        }
    }
    return false;
}

static bool
vitte_c17_ast_is_local_space_function(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *name)
{
    return emitter->space_depth != 0u &&
           vitte_c17_ast_space_declares_function(
               emitter,
               emitter->space_stack[emitter->space_depth - 1u],
               name);
}

static void
vitte_c17_ast_emit_type(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name;

    if (node == NULL) {
        emitter->failed = true;
        return;
    }

    switch (node->kind) {
        case VITTE_AST_NODE_REFERENCE_TYPE:
        case VITTE_AST_NODE_POINTER_TYPE:
            vitte_c17_ast_emit_type(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, " *");
            return;

        case VITTE_AST_NODE_TYPE_EXPR:
            name = vitte_c17_ast_child(emitter, node, 0u);
            if (name == NULL ||
                name->kind != VITTE_AST_NODE_PATH ||
                name->child_count == 0u) {
                emitter->failed = true;
                return;
            }
            {
                const vitte_ast_node_t *path;

                path = name;
                name = vitte_c17_ast_child(
                    emitter,
                    path,
                    path->child_count - 1u);
                if (path->child_count == 1u &&
                    vitte_c17_ast_token_is(emitter, name, "void")) {
                    vitte_c17_ast_write(emitter, "void");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "bool")) {
                    vitte_c17_ast_write(emitter, "bool");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "i64")) {
                    vitte_c17_ast_write(emitter, "int64_t");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "u64")) {
                    vitte_c17_ast_write(emitter, "uint64_t");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "u8")) {
                    vitte_c17_ast_write(emitter, "uint8_t");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "usize")) {
                    vitte_c17_ast_write(emitter, "size_t");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "f64")) {
                    vitte_c17_ast_write(emitter, "double");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "char")) {
                    vitte_c17_ast_write(emitter, "char");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "string")) {
                    vitte_c17_ast_write(emitter, "const char *");
                } else if (path->child_count == 1u &&
                           vitte_c17_ast_token_is(emitter, name, "int")) {
                    vitte_c17_ast_write(emitter, "int32_t");
                } else {
                    vitte_c17_ast_write_token(emitter, name);
                }
            }
            return;

        case VITTE_AST_NODE_ARRAY_TYPE:
        default:
            emitter->failed = true;
            return;
    }
}

static void
vitte_c17_ast_emit_declarator(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *type,
    const vitte_ast_node_t *name)
{
    if (type == NULL || name == NULL) {
        emitter->failed = true;
        return;
    }

    if (type->kind == VITTE_AST_NODE_ARRAY_TYPE) {
        if (type->child_count != 2u) {
            emitter->failed = true;
            return;
        }
        vitte_c17_ast_emit_type(
            emitter,
            vitte_c17_ast_child(emitter, type, 1u));
        vitte_c17_ast_write(emitter, " ");
        vitte_c17_ast_write_token(emitter, name);
        vitte_c17_ast_write(emitter, "[");
        vitte_c17_ast_emit_expression(
            emitter,
            vitte_c17_ast_child(emitter, type, 0u));
        vitte_c17_ast_write(emitter, "]");
        return;
    }

    vitte_c17_ast_emit_type(emitter, type);
    vitte_c17_ast_write(emitter, " ");
    vitte_c17_ast_write_token(emitter, name);
}

static const char *
vitte_c17_ast_inferred_type(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *expression)
{
    const vitte_ast_node_t *left;

    if (expression == NULL) {
        return NULL;
    }

    switch (expression->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            return "int64_t";
        case VITTE_AST_NODE_FLOAT_LITERAL:
            return "double";
        case VITTE_AST_NODE_BOOL_LITERAL:
            return "bool";
        case VITTE_AST_NODE_STRING_LITERAL:
            return "const char *";
        case VITTE_AST_NODE_CHARACTER_LITERAL:
            return "char";
        case VITTE_AST_NODE_GROUP_EXPR:
            return vitte_c17_ast_inferred_type(
                emitter,
                vitte_c17_ast_child(
                    emitter,
                    expression,
                    0u));
        case VITTE_AST_NODE_UNARY_EXPR:
            if (expression->operator_kind == VITTE_TOKEN_BANG ||
                expression->operator_kind == VITTE_TOKEN_KW_NOT) {
                return "bool";
            }
            return vitte_c17_ast_inferred_type(
                emitter,
                vitte_c17_ast_child(
                    emitter,
                    expression,
                    0u));
        case VITTE_AST_NODE_BINARY_EXPR:
            switch (expression->operator_kind) {
                case VITTE_TOKEN_EQUAL_EQUAL:
                case VITTE_TOKEN_BANG_EQUAL:
                case VITTE_TOKEN_LESS:
                case VITTE_TOKEN_LESS_EQUAL:
                case VITTE_TOKEN_GREATER:
                case VITTE_TOKEN_GREATER_EQUAL:
                case VITTE_TOKEN_AMP_AMP:
                case VITTE_TOKEN_PIPE_PIPE:
                case VITTE_TOKEN_KW_AND:
                case VITTE_TOKEN_KW_OR:
                    return "bool";
                default:
                    break;
            }
            left =
                vitte_c17_ast_child(
                    emitter,
                    expression,
                    0u);
            return vitte_c17_ast_inferred_type(
                emitter,
                left);
        default:
            return NULL;
    }
}

static bool
vitte_c17_ast_is_reference_parameter(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *name)
{
    size_t index;

    if (emitter->function == NULL || name == NULL) {
        return false;
    }

    for (index = 1u;
         index < emitter->function->child_count;
         ++index) {
        const vitte_ast_node_t *parameter;
        const vitte_ast_node_t *parameter_name;
        const vitte_ast_node_t *parameter_type;
        const vitte_token_t *left;
        const vitte_token_t *right;

        parameter =
            vitte_c17_ast_child(
                emitter,
                emitter->function,
                index);
        if (parameter == NULL ||
            parameter->kind != VITTE_AST_NODE_PARAMETER) {
            continue;
        }

        parameter_name =
            vitte_c17_ast_child(emitter, parameter, 0u);
        parameter_type =
            vitte_c17_ast_child(emitter, parameter, 1u);
        if (parameter_name == NULL ||
            parameter_type == NULL ||
            (parameter_type->kind != VITTE_AST_NODE_REFERENCE_TYPE &&
             parameter_type->kind != VITTE_AST_NODE_POINTER_TYPE)) {
            continue;
        }

        left = vitte_c17_ast_token(emitter, parameter_name);
        right = vitte_c17_ast_token(emitter, name);
        if (left != NULL && right != NULL &&
            left->length == right->length &&
            memcmp(left->lexeme, right->lexeme, left->length) == 0) {
            return true;
        }
    }

    return false;
}

static bool
vitte_c17_ast_is_pointer_type_node(
    const vitte_ast_node_t *type)
{
    return type != NULL &&
           (type->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
            type->kind == VITTE_AST_NODE_POINTER_TYPE);
}

static bool
vitte_c17_ast_is_address_expression(
    const vitte_ast_node_t *expression)
{
    return expression != NULL &&
           expression->kind == VITTE_AST_NODE_UNARY_EXPR &&
           expression->operator_kind == VITTE_TOKEN_AMP;
}

static bool
vitte_c17_ast_function_has_pointer_local(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *name)
{
    size_t index;

    if (node == NULL || name == NULL) {
        return false;
    }

    if (node->kind == VITTE_AST_NODE_LET_STMT &&
        node->child_count >= 2u) {
        const vitte_ast_node_t *local_name;
        const vitte_ast_node_t *type_or_initializer;
        const vitte_ast_node_t *initializer;

        local_name = vitte_c17_ast_child(emitter, node, 0u);
        type_or_initializer = vitte_c17_ast_child(emitter, node, 1u);
        if (local_name != NULL &&
            vitte_c17_ast_token_equal(emitter, local_name, name)) {
            if (vitte_c17_ast_is_pointer_type_node(type_or_initializer)) {
                return true;
            }
            initializer = type_or_initializer;
            if (type_or_initializer->kind == VITTE_AST_NODE_TYPE_EXPR &&
                node->child_count >= 3u) {
                initializer = vitte_c17_ast_child(emitter, node, 2u);
            }
            if (vitte_c17_ast_is_address_expression(initializer)) {
                return true;
            }
        }
    }

    for (index = 0u; index < node->child_count; ++index) {
        if (vitte_c17_ast_function_has_pointer_local(
                emitter,
                vitte_c17_ast_child(emitter, node, index),
                name)) {
            return true;
        }
    }

    return false;
}

static bool
vitte_c17_ast_expression_is_pointer(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *expression)
{
    const vitte_ast_node_t *name;

    if (expression == NULL) {
        return false;
    }

    if (expression->kind == VITTE_AST_NODE_GROUP_EXPR) {
        return vitte_c17_ast_expression_is_pointer(
            emitter,
            vitte_c17_ast_child(emitter, expression, 0u));
    }

    if (expression->kind != VITTE_AST_NODE_PATH &&
        expression->kind != VITTE_AST_NODE_IDENTIFIER) {
        return false;
    }

    name = expression->kind == VITTE_AST_NODE_PATH
        ? (expression->child_count == 1u
            ? vitte_c17_ast_child(emitter, expression, 0u)
            : NULL)
        : expression;
    if (name == NULL) {
        return false;
    }

    return vitte_c17_ast_is_reference_parameter(emitter, name) ||
           (emitter->function != NULL &&
            vitte_c17_ast_function_has_pointer_local(
                emitter,
                emitter->function,
                name));
}

static bool
vitte_c17_ast_call_parameter_is_reference(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *callee,
    size_t parameter_index)
{
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *callee_name;
    const vitte_token_t *callee_token;
    size_t index;

    if (callee == NULL) {
        return false;
    }
    if (callee->kind == VITTE_AST_NODE_PATH &&
        callee->child_count == 1u) {
        callee_name = vitte_c17_ast_child(emitter, callee, 0u);
    } else if (callee->kind == VITTE_AST_NODE_IDENTIFIER) {
        callee_name = callee;
    } else {
        return false;
    }
    callee_token = vitte_c17_ast_token(emitter, callee_name);
    if (callee_token == NULL) {
        return false;
    }

    root = vitte_c17_ast_node(
        emitter,
        vitte_parser_root(emitter->parser));
    if (root == NULL) {
        return false;
    }
    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *declaration;
        const vitte_ast_node_t *name;
        const vitte_token_t *name_token;
        size_t child_index;
        size_t current_parameter;

        declaration = vitte_c17_ast_child(emitter, root, index);
        if (declaration == NULL ||
            (declaration->kind != VITTE_AST_NODE_PROC_DECL &&
             declaration->kind != VITTE_AST_NODE_EXTERN_PROC_DECL)) {
            continue;
        }
        name = vitte_c17_ast_child(emitter, declaration, 0u);
        name_token = vitte_c17_ast_token(emitter, name);
        if (name_token == NULL ||
            name_token->length != callee_token->length ||
            memcmp(
                name_token->lexeme,
                callee_token->lexeme,
                name_token->length) != 0) {
            continue;
        }

        current_parameter = 0u;
        for (child_index = 1u;
             child_index < declaration->child_count;
             ++child_index) {
            const vitte_ast_node_t *parameter;
            const vitte_ast_node_t *type;

            parameter =
                vitte_c17_ast_child(
                    emitter,
                    declaration,
                    child_index);
            if (parameter == NULL ||
                parameter->kind != VITTE_AST_NODE_PARAMETER) {
                continue;
            }
            if (current_parameter++ != parameter_index) {
                continue;
            }
            type = vitte_c17_ast_child(emitter, parameter, 1u);
            return type != NULL &&
                   (type->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
                    type->kind == VITTE_AST_NODE_POINTER_TYPE);
        }
        return false;
    }

    return false;
}

static void
vitte_c17_ast_emit_call_argument(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *call,
    size_t argument_index)
{
    const vitte_ast_node_t *callee;
    const vitte_ast_node_t *argument;
    const vitte_ast_node_t *name;

    callee = vitte_c17_ast_child(emitter, call, 0u);
    argument =
        vitte_c17_ast_child(
            emitter,
            call,
            argument_index + 1u);
    name = argument;
    if (argument != NULL &&
        argument->kind == VITTE_AST_NODE_PATH &&
        argument->child_count == 1u) {
        name = vitte_c17_ast_child(emitter, argument, 0u);
    }

    if (name != NULL &&
        vitte_c17_ast_is_reference_parameter(emitter, name) &&
        vitte_c17_ast_call_parameter_is_reference(
            emitter,
            callee,
            argument_index)) {
        vitte_c17_ast_write_token(emitter, name);
        return;
    }

    vitte_c17_ast_emit_expression(emitter, argument);
}

static bool
vitte_c17_ast_emit_string_builtin_callee(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *callee)
{
    const vitte_ast_node_t *name;

    name = callee;
    if (callee != NULL &&
        (callee->kind == VITTE_AST_NODE_PATH ||
         callee->kind == VITTE_AST_NODE_TYPE_EXPR) &&
        callee->child_count != 0u) {
        name = vitte_c17_ast_child(
            emitter,
            callee,
            callee->child_count - 1u);
    }
    if (vitte_c17_ast_token_is(emitter, name, "len")) {
        vitte_c17_ast_write(emitter, "vitte_string_length");
        return true;
    }
    if (vitte_c17_ast_token_is(emitter, name, "trim")) {
        vitte_c17_ast_write(emitter, "vitte_string_trim");
        return true;
    }
    if (vitte_c17_ast_token_is(emitter, name, "starts_with")) {
        vitte_c17_ast_write(emitter, "vitte_string_starts_with");
        return true;
    }
    if (vitte_c17_ast_token_is(emitter, name, "ends_with")) {
        vitte_c17_ast_write(emitter, "vitte_string_ends_with");
        return true;
    }
    return false;
}

static bool
vitte_c17_ast_expression_is_pointer(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *expression);

static bool
vitte_c17_ast_member_uses_pointer(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *base)
{
    const vitte_ast_node_t *identifier;
    if (base == NULL) {
        return false;
    }

    if (base->kind == VITTE_AST_NODE_PATH ||
        base->kind == VITTE_AST_NODE_IDENTIFIER) {
        identifier = base->kind == VITTE_AST_NODE_PATH
            ? vitte_c17_ast_child(emitter, base, 0u)
            : base;
        if (identifier == NULL) {
            return false;
        }
        return vitte_c17_ast_is_reference_parameter(
                   emitter,
                   identifier) ||
               vitte_c17_ast_expression_is_pointer(
                   emitter,
                   base);
    }

    return false;
}

static void
vitte_c17_ast_emit_path(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    size_t index;

    if (node == NULL || node->child_count == 0u) {
        emitter->failed = true;
        return;
    }

    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *segment;

        segment = vitte_c17_ast_child(emitter, node, index);
        if (segment == NULL) {
            emitter->failed = true;
            return;
        }

        if (index != 0u) {
            vitte_c17_ast_write(emitter, "_");
        }

        if (index == 0u &&
            node->child_count == 1u &&
            vitte_c17_ast_token_is(emitter, segment, "main")) {
            vitte_c17_ast_write(emitter, "vitte_entry_main");
        } else {
            vitte_c17_ast_write_token(emitter, segment);
        }
    }
}

static void
vitte_c17_ast_emit_operator(
    vitte_c17_ast_emitter_t *emitter,
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_PLUS: vitte_c17_ast_write(emitter, "+"); break;
        case VITTE_TOKEN_MINUS: vitte_c17_ast_write(emitter, "-"); break;
        case VITTE_TOKEN_STAR: vitte_c17_ast_write(emitter, "*"); break;
        case VITTE_TOKEN_SLASH: vitte_c17_ast_write(emitter, "/"); break;
        case VITTE_TOKEN_PERCENT: vitte_c17_ast_write(emitter, "%"); break;
        case VITTE_TOKEN_AMP: vitte_c17_ast_write(emitter, "&"); break;
        case VITTE_TOKEN_PIPE: vitte_c17_ast_write(emitter, "|"); break;
        case VITTE_TOKEN_CARET: vitte_c17_ast_write(emitter, "^"); break;
        case VITTE_TOKEN_TILDE: vitte_c17_ast_write(emitter, "~"); break;
        case VITTE_TOKEN_BANG: vitte_c17_ast_write(emitter, "!"); break;
        case VITTE_TOKEN_EQUAL: vitte_c17_ast_write(emitter, "="); break;
        case VITTE_TOKEN_PLUS_EQUAL: vitte_c17_ast_write(emitter, "+="); break;
        case VITTE_TOKEN_MINUS_EQUAL: vitte_c17_ast_write(emitter, "-="); break;
        case VITTE_TOKEN_STAR_EQUAL: vitte_c17_ast_write(emitter, "*="); break;
        case VITTE_TOKEN_SLASH_EQUAL: vitte_c17_ast_write(emitter, "/="); break;
        case VITTE_TOKEN_PERCENT_EQUAL: vitte_c17_ast_write(emitter, "%="); break;
        case VITTE_TOKEN_EQUAL_EQUAL: vitte_c17_ast_write(emitter, "=="); break;
        case VITTE_TOKEN_BANG_EQUAL: vitte_c17_ast_write(emitter, "!="); break;
        case VITTE_TOKEN_LESS: vitte_c17_ast_write(emitter, "<"); break;
        case VITTE_TOKEN_LESS_EQUAL: vitte_c17_ast_write(emitter, "<="); break;
        case VITTE_TOKEN_GREATER: vitte_c17_ast_write(emitter, ">"); break;
        case VITTE_TOKEN_GREATER_EQUAL: vitte_c17_ast_write(emitter, ">="); break;
        case VITTE_TOKEN_SHIFT_LEFT: vitte_c17_ast_write(emitter, "<<"); break;
        case VITTE_TOKEN_SHIFT_RIGHT: vitte_c17_ast_write(emitter, ">>"); break;
        case VITTE_TOKEN_AMP_AMP:
        case VITTE_TOKEN_KW_AND: vitte_c17_ast_write(emitter, "&&"); break;
        case VITTE_TOKEN_PIPE_PIPE:
        case VITTE_TOKEN_KW_OR: vitte_c17_ast_write(emitter, "||"); break;
        case VITTE_TOKEN_KW_NOT: vitte_c17_ast_write(emitter, "!"); break;
        default: emitter->failed = true; break;
    }
}

static const vitte_ast_node_t *
vitte_c17_ast_find_procedure_named(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *name)
{
    size_t index;

    if (node == NULL || name == NULL) {
        return NULL;
    }

    if ((node->kind == VITTE_AST_NODE_PROC_DECL ||
         node->kind == VITTE_AST_NODE_EXTERN_PROC_DECL) &&
        node->child_count != 0u &&
        vitte_c17_ast_token_equal(
            emitter,
            vitte_c17_ast_child(emitter, node, 0u),
            name)) {
        return node;
    }

    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *found;

        found = vitte_c17_ast_find_procedure_named(
            emitter,
            vitte_c17_ast_child(emitter, node, index),
            name);
        if (found != NULL) {
            return found;
        }
    }

    return NULL;
}

static const vitte_ast_node_t *
vitte_c17_ast_call_return_type(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *call)
{
    const vitte_ast_node_t *callee;
    const vitte_ast_node_t *callee_name;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *procedure;
    size_t index;

    if (call == NULL ||
        call->kind != VITTE_AST_NODE_CALL_EXPR ||
        call->child_count == 0u) {
        return NULL;
    }

    callee = vitte_c17_ast_child(emitter, call, 0u);
    if (callee == NULL) {
        return NULL;
    }
    if (callee->kind == VITTE_AST_NODE_PATH &&
        callee->child_count != 0u) {
        callee_name = vitte_c17_ast_child(
            emitter,
            callee,
            callee->child_count - 1u);
    } else if (callee->kind == VITTE_AST_NODE_IDENTIFIER) {
        callee_name = callee;
    } else {
        return NULL;
    }

    root = vitte_c17_ast_node(
        emitter,
        vitte_parser_root(emitter->parser));
    procedure = vitte_c17_ast_find_procedure_named(
        emitter,
        root,
        callee_name);
    if (procedure == NULL) {
        return NULL;
    }

    for (index = 1u; index < procedure->child_count; ++index) {
        const vitte_ast_node_t *child;

        child = vitte_c17_ast_child(emitter, procedure, index);
        if (child != NULL && child->kind == VITTE_AST_NODE_TYPE_EXPR) {
            return child;
        }
    }

    return NULL;
}

static bool
vitte_c17_ast_emit_address_of_rvalue(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *operand)
{
    const vitte_ast_node_t *return_type;

    if (operand == NULL || operand->kind != VITTE_AST_NODE_CALL_EXPR) {
        return false;
    }

    return_type = vitte_c17_ast_call_return_type(emitter, operand);
    if (return_type == NULL) {
        return false;
    }

    /*
     * C does not permit `&function_returning_struct()`.  A one-element
     * compound array gives the temporary object a real lifetime for the
     * duration of the full expression while preserving the returned value.
     */
    vitte_c17_ast_write(emitter, "&((");
    vitte_c17_ast_emit_type(emitter, return_type);
    vitte_c17_ast_write(emitter, "[]){");
    vitte_c17_ast_emit_expression(emitter, operand);
    vitte_c17_ast_write(emitter, "})[0]");
    return !emitter->failed;
}

static void
vitte_c17_ast_emit_expression(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *left;
    const vitte_ast_node_t *right;
    size_t index;

    if (node == NULL || emitter->failed) {
        emitter->failed = true;
        return;
    }

    switch (node->kind) {
        case VITTE_AST_NODE_IF_EXPR:
            if (node->child_count != 3u) {
                emitter->failed = true;
                return;
            }
            {
                const vitte_ast_node_t *then_branch;
                const vitte_ast_node_t *else_branch;
                then_branch = vitte_c17_ast_child(emitter, node, 1u);
                else_branch = vitte_c17_ast_child(emitter, node, 2u);
                if (then_branch == NULL ||
                    then_branch->kind != VITTE_AST_NODE_BLOCK_EXPR ||
                    then_branch->child_count != 1u ||
                    else_branch == NULL ||
                    (else_branch->kind ==
                         VITTE_AST_NODE_BLOCK_EXPR &&
                     else_branch->child_count != 1u)) {
                    emitter->failed = true;
                    return;
                }
                vitte_c17_ast_write(emitter, "(");
                vitte_c17_ast_emit_condition(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u));
                vitte_c17_ast_write(emitter, " ? ");
                vitte_c17_ast_emit_expression(
                    emitter,
                    vitte_c17_ast_child(emitter, then_branch, 0u));
                vitte_c17_ast_write(emitter, " : ");
                if (else_branch->kind == VITTE_AST_NODE_BLOCK_EXPR) {
                    vitte_c17_ast_emit_expression(
                        emitter,
                        vitte_c17_ast_child(emitter, else_branch, 0u));
                } else {
                    vitte_c17_ast_emit_expression(emitter, else_branch);
                }
                vitte_c17_ast_write(emitter, ")");
            }
            return;

        case VITTE_AST_NODE_BLOCK_EXPR:
            if (node->child_count != 1u) {
                emitter->failed = true;
                return;
            }
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            return;

        case VITTE_AST_NODE_FORM_EXPR:
            if (node->child_count == 0u) {
                emitter->failed = true;
                return;
            }
            vitte_c17_ast_write(emitter, "(");
            left = vitte_c17_ast_child(emitter, node, 0u);
            if (left != NULL &&
                left->kind == VITTE_AST_NODE_PATH &&
                left->child_count != 0u) {
                vitte_c17_ast_write_token(
                    emitter,
                    vitte_c17_ast_child(
                        emitter,
                        left,
                        left->child_count - 1u));
            } else {
                emitter->failed = true;
            }
            vitte_c17_ast_write(emitter, "){ ");
            for (index = 1u; index < node->child_count; ++index) {
                const vitte_ast_node_t *field;
                field = vitte_c17_ast_child(emitter, node, index);
                if (field == NULL ||
                    field->kind != VITTE_AST_NODE_FIELD_INIT ||
                    field->child_count != 2u) {
                    emitter->failed = true;
                    return;
                }
                if (index != 1u) {
                    vitte_c17_ast_write(emitter, ", ");
                }
                vitte_c17_ast_write(emitter, ".");
                vitte_c17_ast_write_token(
                    emitter,
                    vitte_c17_ast_child(emitter, field, 0u));
                vitte_c17_ast_write(emitter, " = ");
                vitte_c17_ast_emit_expression(
                    emitter,
                    vitte_c17_ast_child(emitter, field, 1u));
            }
            vitte_c17_ast_write(emitter, " }");
            return;

        case VITTE_AST_NODE_PATH:
            if (node->child_count == 1u &&
                vitte_c17_ast_is_local_space_function(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u))) {
                vitte_c17_ast_emit_function_name(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u));
            } else if (node->child_count == 3u) {
                vitte_c17_ast_emit_path(emitter, node);
            } else if (node->child_count == 2u) {
                vitte_c17_ast_emit_path(emitter, node);
            } else if (node->child_count == 1u) {
                left = vitte_c17_ast_child(emitter, node, 0u);
                if (vitte_c17_ast_is_reference_parameter(
                        emitter,
                        left)) {
                    vitte_c17_ast_write(emitter, "(*");
                    vitte_c17_ast_write_token(emitter, left);
                    vitte_c17_ast_write(emitter, ")");
                } else {
                    vitte_c17_ast_emit_path(emitter, node);
                }
            } else {
                emitter->failed = true;
            }
            return;

        case VITTE_AST_NODE_IDENTIFIER:
            if (vitte_c17_ast_is_reference_parameter(emitter, node)) {
                vitte_c17_ast_write(emitter, "(*");
                vitte_c17_ast_write_token(emitter, node);
                vitte_c17_ast_write(emitter, ")");
            } else {
                vitte_c17_ast_write_token(emitter, node);
            }
            return;

        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
            vitte_c17_ast_write_token(emitter, node);
            return;

        case VITTE_AST_NODE_BOOL_LITERAL:
            vitte_c17_ast_write(
                emitter,
                node->token_index < emitter->parser->token_count &&
                    emitter->parser->tokens[node->token_index].kind ==
                        VITTE_TOKEN_KW_TRUE
                    ? "true"
                    : "false");
            return;

        case VITTE_AST_NODE_NULL_LITERAL:
            vitte_c17_ast_write(emitter, "NULL");
            return;

        case VITTE_AST_NODE_GROUP_EXPR:
            vitte_c17_ast_write(emitter, "(");
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, ")");
            return;

        case VITTE_AST_NODE_ARRAY_EXPR:
            vitte_c17_ast_write(emitter, "{");
            for (index = 0u; index < node->child_count; ++index) {
                if (index != 0u) {
                    vitte_c17_ast_write(emitter, ", ");
                }
                vitte_c17_ast_emit_expression(
                    emitter,
                    vitte_c17_ast_child(emitter, node, index));
            }
            vitte_c17_ast_write(emitter, "}");
            return;

        case VITTE_AST_NODE_UNARY_EXPR:
            if (node->operator_kind == VITTE_TOKEN_AMP &&
                vitte_c17_ast_emit_address_of_rvalue(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u))) {
                return;
            }
            if (node->operator_kind == VITTE_TOKEN_STAR) {
                const vitte_ast_node_t *operand;
                const vitte_ast_node_t *name;

                operand = vitte_c17_ast_child(emitter, node, 0u);
                name = operand;
                if (operand != NULL &&
                    operand->kind == VITTE_AST_NODE_PATH &&
                    operand->child_count == 1u) {
                    name = vitte_c17_ast_child(
                        emitter,
                        operand,
                        0u);
                }
                if (vitte_c17_ast_is_reference_parameter(
                        emitter,
                        name)) {
                    vitte_c17_ast_write(emitter, "(*");
                    vitte_c17_ast_write_token(emitter, name);
                    vitte_c17_ast_write(emitter, ")");
                    return;
                }
            }
            vitte_c17_ast_emit_operator(emitter, node->operator_kind);
            vitte_c17_ast_write(emitter, "(");
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, ")");
            return;

        case VITTE_AST_NODE_BINARY_EXPR:
        case VITTE_AST_NODE_ASSIGN_EXPR:
            left = vitte_c17_ast_child(emitter, node, 0u);
            right = vitte_c17_ast_child(emitter, node, 1u);
            if (node->kind == VITTE_AST_NODE_BINARY_EXPR &&
                node->operator_kind == VITTE_TOKEN_PLUS &&
                vitte_c17_ast_is_string_literal_expression(
                    emitter,
                    left) &&
                vitte_c17_ast_is_string_literal_expression(
                    emitter,
                    right)) {
                vitte_c17_ast_write(emitter, "(");
                vitte_c17_ast_emit_string_literal_sequence(
                    emitter,
                    node);
                vitte_c17_ast_write(emitter, ")");
                return;
            }
            vitte_c17_ast_write(emitter, "(");
            vitte_c17_ast_emit_expression(emitter, left);
            vitte_c17_ast_write(emitter, " ");
            vitte_c17_ast_emit_operator(emitter, node->operator_kind);
            vitte_c17_ast_write(emitter, " ");
            vitte_c17_ast_emit_expression(emitter, right);
            vitte_c17_ast_write(emitter, ")");
            return;

        case VITTE_AST_NODE_CAST_EXPR:
            if (node->child_count != 2u) {
                emitter->failed = true;
                return;
            }
            vitte_c17_ast_write(emitter, "(");
            vitte_c17_ast_emit_type(
                emitter,
                vitte_c17_ast_child(emitter, node, 1u));
            vitte_c17_ast_write(emitter, ")(");
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, ")");
            return;

        case VITTE_AST_NODE_CALL_EXPR:
            if (!vitte_c17_ast_emit_string_builtin_callee(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u))) {
                vitte_c17_ast_emit_expression(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u));
            }
            vitte_c17_ast_write(emitter, "(");
            for (index = 1u; index < node->child_count; ++index) {
                if (index != 1u) {
                    vitte_c17_ast_write(emitter, ", ");
                }
                vitte_c17_ast_emit_call_argument(
                    emitter,
                    node,
                    index - 1u);
            }
            vitte_c17_ast_write(emitter, ")");
            return;

        case VITTE_AST_NODE_MEMBER_EXPR:
            left = vitte_c17_ast_child(emitter, node, 0u);
            right = vitte_c17_ast_child(emitter, node, 1u);
            if (vitte_c17_ast_member_uses_pointer(emitter, left)) {
                const vitte_ast_node_t *base_name;

                base_name = vitte_c17_ast_child(emitter, left, 0u);
                vitte_c17_ast_write_token(emitter, base_name);
                vitte_c17_ast_write(emitter, "->");
            } else {
                vitte_c17_ast_emit_expression(emitter, left);
                vitte_c17_ast_write(emitter, ".");
            }
            vitte_c17_ast_write_token(emitter, right);
            return;

        case VITTE_AST_NODE_INDEX_EXPR:
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, "[");
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 1u));
            vitte_c17_ast_write(emitter, "]");
            return;

        default:
            emitter->failed = true;
            return;
    }
}

static void
vitte_c17_ast_emit_statement(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    unsigned int indent);

static void
vitte_c17_ast_indent(
    vitte_c17_ast_emitter_t *emitter,
    unsigned int indent);

static void
vitte_c17_ast_emit_if_expression_into(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *target,
    unsigned int indent);

static void
vitte_c17_ast_emit_block_expression_into(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *block,
    const vitte_ast_node_t *target,
    unsigned int indent)
{
    size_t index;
    const vitte_ast_node_t *tail;

    if (block == NULL ||
        block->kind != VITTE_AST_NODE_BLOCK_EXPR ||
        block->child_count == 0u ||
        target == NULL) {
        emitter->failed = true;
        return;
    }
    for (index = 0u; index + 1u < block->child_count; ++index) {
        vitte_c17_ast_emit_statement(
            emitter,
            vitte_c17_ast_child(emitter, block, index),
            indent);
    }
    tail = vitte_c17_ast_child(
        emitter,
        block,
        block->child_count - 1u);
    if (tail == NULL) {
        emitter->failed = true;
        return;
    }
    if (tail->kind == VITTE_AST_NODE_IF_EXPR) {
        vitte_c17_ast_emit_if_expression_into(
            emitter,
            tail,
            target,
            indent);
        return;
    }
    vitte_c17_ast_indent(emitter, indent);
    vitte_c17_ast_write_token(emitter, target);
    vitte_c17_ast_write(emitter, " = ");
    vitte_c17_ast_emit_expression(emitter, tail);
    vitte_c17_ast_write(emitter, ";\n");
}

static void
vitte_c17_ast_emit_if_expression_into(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *target,
    unsigned int indent)
{
    const vitte_ast_node_t *then_branch;
    const vitte_ast_node_t *else_branch;

    if (node == NULL ||
        node->kind != VITTE_AST_NODE_IF_EXPR ||
        node->child_count != 3u ||
        target == NULL) {
        emitter->failed = true;
        return;
    }
    then_branch = vitte_c17_ast_child(emitter, node, 1u);
    else_branch = vitte_c17_ast_child(emitter, node, 2u);
    vitte_c17_ast_indent(emitter, indent);
    vitte_c17_ast_write(emitter, "if ");
    vitte_c17_ast_emit_condition(
        emitter,
        vitte_c17_ast_child(emitter, node, 0u));
    vitte_c17_ast_write(emitter, " {\n");
    vitte_c17_ast_emit_block_expression_into(
        emitter,
        then_branch,
        target,
        indent + 1u);
    vitte_c17_ast_indent(emitter, indent);
    vitte_c17_ast_write(emitter, "}");
    if (else_branch != NULL &&
        else_branch->kind == VITTE_AST_NODE_IF_EXPR) {
        vitte_c17_ast_write(emitter, " else {\n");
        vitte_c17_ast_emit_if_expression_into(
            emitter,
            else_branch,
            target,
            indent + 1u);
        vitte_c17_ast_indent(emitter, indent);
        vitte_c17_ast_write(emitter, "}\n");
    } else {
        vitte_c17_ast_write(emitter, " else {\n");
        vitte_c17_ast_emit_block_expression_into(
            emitter,
            else_branch,
            target,
            indent + 1u);
        vitte_c17_ast_indent(emitter, indent);
        vitte_c17_ast_write(emitter, "}\n");
    }
}

static void
vitte_c17_ast_emit_condition(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    if (node == NULL) {
        emitter->failed = true;
        return;
    }

    vitte_c17_ast_write(emitter, "(");
    if (node->kind == VITTE_AST_NODE_BINARY_EXPR ||
        node->kind == VITTE_AST_NODE_ASSIGN_EXPR) {
        vitte_c17_ast_emit_expression(
            emitter,
            vitte_c17_ast_child(emitter, node, 0u));
        vitte_c17_ast_write(emitter, " ");
        vitte_c17_ast_emit_operator(emitter, node->operator_kind);
        vitte_c17_ast_write(emitter, " ");
        vitte_c17_ast_emit_expression(
            emitter,
            vitte_c17_ast_child(emitter, node, 1u));
    } else {
        vitte_c17_ast_emit_expression(emitter, node);
    }
    vitte_c17_ast_write(emitter, ")");
}

static void
vitte_c17_ast_indent(
    vitte_c17_ast_emitter_t *emitter,
    unsigned int indent)
{
    unsigned int index;

    for (index = 0u; index < indent; ++index) {
        vitte_c17_ast_write(emitter, "    ");
    }
}

static void
vitte_c17_ast_emit_block_contents(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *block,
    unsigned int indent)
{
    size_t index;

    if (block == NULL || block->kind != VITTE_AST_NODE_BLOCK) {
        emitter->failed = true;
        return;
    }

    for (index = 0u; index < block->child_count; ++index) {
        vitte_c17_ast_emit_statement(
            emitter,
            vitte_c17_ast_child(emitter, block, index),
            indent);
    }
}

static bool
vitte_c17_ast_debug_scalar_type_name(
    const char *name)
{
    return name != NULL &&
           (strcmp(name, "i64") == 0 ||
            strcmp(name, "u64") == 0 ||
            strcmp(name, "u8") == 0 ||
            strcmp(name, "usize") == 0 ||
            strcmp(name, "int") == 0 ||
            strcmp(name, "char") == 0 ||
            strcmp(name, "bool") == 0 ||
            strcmp(name, "int64_t") == 0 ||
            strcmp(name, "uint64_t") == 0 ||
            strcmp(name, "uint8_t") == 0 ||
            strcmp(name, "size_t") == 0 ||
            strcmp(name, "int32_t") == 0);
}

static bool
vitte_c17_ast_debug_scalar_let(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *type;
    const vitte_ast_node_t *path;
    const vitte_ast_node_t *name;
    const vitte_ast_node_t *initializer;
    const char *inferred_type;

    if (node == NULL ||
        node->kind != VITTE_AST_NODE_LET_STMT ||
        node->child_count < 2u) {
        return false;
    }

    type = vitte_c17_ast_child(emitter, node, 1u);
    if (type != NULL && type->kind == VITTE_AST_NODE_TYPE_EXPR) {
        if (node->child_count < 3u) {
            return false;
        }
        path = vitte_c17_ast_child(emitter, type, 0u);
        if (path == NULL || path->kind != VITTE_AST_NODE_PATH ||
            path->child_count == 0u) {
            return false;
        }
        name = vitte_c17_ast_child(
            emitter,
            path,
            path->child_count - 1u);
        return vitte_c17_ast_token_is(emitter, name, "i64") ||
               vitte_c17_ast_token_is(emitter, name, "u64") ||
               vitte_c17_ast_token_is(emitter, name, "u8") ||
               vitte_c17_ast_token_is(emitter, name, "usize") ||
               vitte_c17_ast_token_is(emitter, name, "int") ||
               vitte_c17_ast_token_is(emitter, name, "char") ||
               vitte_c17_ast_token_is(emitter, name, "bool");
    }

    if (type == NULL || type->kind == VITTE_AST_NODE_ARRAY_TYPE ||
        type->kind == VITTE_AST_NODE_POINTER_TYPE ||
        type->kind == VITTE_AST_NODE_REFERENCE_TYPE) {
        return false;
    }

    initializer = type;
    inferred_type = vitte_c17_ast_inferred_type(emitter, initializer);
    return vitte_c17_ast_debug_scalar_type_name(inferred_type);
}

static bool
vitte_c17_ast_debug_name_is_known(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *name)
{
    const vitte_token_t *token;
    size_t index;

    token = vitte_c17_ast_token(emitter, name);
    if (token == NULL) {
        return false;
    }
    for (index = 0u; index < emitter->debug_scalar_name_count; ++index) {
        if (emitter->debug_scalar_names[index].length == token->length &&
            memcmp(
                emitter->debug_scalar_names[index].lexeme,
                token->lexeme,
                token->length) == 0) {
            return true;
        }
    }
    return false;
}

static void
vitte_c17_ast_debug_remember_name(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *name)
{
    const vitte_token_t *token;

    if (emitter == NULL ||
        emitter->debug_scalar_name_count >=
            sizeof(emitter->debug_scalar_names) /
                sizeof(emitter->debug_scalar_names[0])) {
        return;
    }
    token = vitte_c17_ast_token(emitter, name);
    if (token == NULL || vitte_c17_ast_debug_name_is_known(emitter, name)) {
        return;
    }
    emitter->debug_scalar_names[emitter->debug_scalar_name_count].lexeme =
        token->lexeme;
    emitter->debug_scalar_names[emitter->debug_scalar_name_count].length =
        token->length;
    emitter->debug_scalar_name_count++;
}

static const vitte_ast_node_t *
vitte_c17_ast_debug_assignment_name(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *expression)
{
    const vitte_ast_node_t *left;

    if (expression == NULL ||
        expression->kind != VITTE_AST_NODE_ASSIGN_EXPR) {
        return NULL;
    }
    left = vitte_c17_ast_child(emitter, expression, 0u);
    if (left == NULL) {
        return NULL;
    }
    if (left->kind == VITTE_AST_NODE_IDENTIFIER) {
        return left;
    }
    if (left->kind == VITTE_AST_NODE_PATH && left->child_count == 1u) {
        return vitte_c17_ast_child(emitter, left, 0u);
    }
    return NULL;
}

static void
vitte_c17_ast_emit_debug_scalar_update(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *name,
    unsigned int indent,
    bool remember)
{
    const vitte_ast_node_t *function_name;

    if (emitter == NULL || !emitter->debug_runtime || name == NULL) {
        return;
    }
    if (remember) {
        vitte_c17_ast_debug_remember_name(emitter, name);
    }
    if (!vitte_c17_ast_debug_name_is_known(emitter, name)) {
        return;
    }

    function_name = emitter->function != NULL
        ? vitte_c17_ast_child(emitter, emitter->function, 0u)
        : NULL;
    vitte_c17_ast_indent(emitter, indent);
    vitte_c17_ast_write(emitter, "vitte_debug_set_i64(\"");
    if (function_name != NULL) {
        vitte_c17_ast_write_token(emitter, function_name);
    } else {
        vitte_c17_ast_write(emitter, "main");
    }
    vitte_c17_ast_write(emitter, "\", \"");
    vitte_c17_ast_write_token(emitter, name);
    vitte_c17_ast_write(emitter, "\", (int64_t)(");
    vitte_c17_ast_write_token(emitter, name);
    vitte_c17_ast_write(emitter, "));\n");
}

static void
vitte_c17_ast_emit_statement(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    unsigned int indent)
{
    const vitte_ast_node_t *child;
    size_t index;

    if (node == NULL || emitter->failed) {
        emitter->failed = true;
        return;
    }

    vitte_c17_ast_emit_source_line(emitter, node);
    if (emitter->debug_runtime && node->span.line != 0u) {
        const vitte_ast_node_t *function_name;

        function_name = emitter->function != NULL
            ? vitte_c17_ast_child(emitter, emitter->function, 0u)
            : NULL;
        vitte_c17_ast_indent(emitter, indent);
        vitte_c17_ast_write(emitter, "vitte_debug_probe(");
        if (fprintf(
                emitter->output,
                "%zuu, \"",
                node->span.line) < 0) {
            emitter->failed = true;
            return;
        }
        if (function_name != NULL) {
            vitte_c17_ast_write_token(emitter, function_name);
        } else {
            vitte_c17_ast_write(emitter, "main");
        }
        vitte_c17_ast_write(emitter, "\");\n");
        if (emitter->failed) {
            emitter->failed = true;
            return;
        }
    }
    vitte_c17_ast_indent(emitter, indent);
    switch (node->kind) {
        case VITTE_AST_NODE_BLOCK:
            vitte_c17_ast_write(emitter, "{\n");
            vitte_c17_ast_emit_block_contents(
                emitter,
                node,
                indent + 1u);
            vitte_c17_ast_indent(emitter, indent);
            vitte_c17_ast_write(emitter, "}\n");
            return;

        case VITTE_AST_NODE_LET_STMT:
            child = vitte_c17_ast_child(emitter, node, 0u);
            if (child == NULL) {
                emitter->failed = true;
                return;
            }
            if (node->child_count > 1u &&
                vitte_c17_ast_child(emitter, node, 1u) != NULL &&
                vitte_c17_ast_child(emitter, node, 1u)->kind ==
                    VITTE_AST_NODE_ARRAY_TYPE) {
                const vitte_ast_node_t *array_type;

                array_type = vitte_c17_ast_child(emitter, node, 1u);
                if (array_type->child_count != 2u) {
                    emitter->failed = true;
                    return;
                }
                vitte_c17_ast_emit_type(
                    emitter,
                    vitte_c17_ast_child(emitter, array_type, 1u));
                vitte_c17_ast_write(emitter, " ");
                vitte_c17_ast_write_token(emitter, child);
                vitte_c17_ast_write(emitter, "[");
                vitte_c17_ast_emit_expression(
                    emitter,
                    vitte_c17_ast_child(emitter, array_type, 0u));
                vitte_c17_ast_write(emitter, "]");
                index = 2u;
            } else if (node->child_count > 1u &&
                vitte_c17_ast_child(emitter, node, 1u) != NULL &&
                (vitte_c17_ast_child(emitter, node, 1u)->kind ==
                     VITTE_AST_NODE_TYPE_EXPR ||
                 vitte_c17_ast_child(emitter, node, 1u)->kind ==
                     VITTE_AST_NODE_POINTER_TYPE ||
                 vitte_c17_ast_child(emitter, node, 1u)->kind ==
                     VITTE_AST_NODE_REFERENCE_TYPE)) {
                vitte_c17_ast_emit_type(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 1u));
                vitte_c17_ast_write(emitter, " ");
                vitte_c17_ast_write_token(emitter, child);
                index = 2u;
            } else if (node->child_count > 1u) {
                const vitte_ast_node_t *initializer;
                const char *inferred_type;

                initializer =
                    vitte_c17_ast_child(
                        emitter,
                        node,
                        1u);
                inferred_type =
                    vitte_c17_ast_inferred_type(
                        emitter,
                        initializer);
                if (inferred_type == NULL) {
                    emitter->failed = true;
                    return;
                }
                vitte_c17_ast_write(emitter, inferred_type);
                vitte_c17_ast_write(emitter, " ");
                vitte_c17_ast_write_token(emitter, child);
                index = 1u;
            } else {
                emitter->failed = true;
                return;
            }
            if (index < node->child_count) {
                const vitte_ast_node_t *initializer;
                initializer = vitte_c17_ast_child(emitter, node, index);
                if (initializer != NULL &&
                    initializer->kind == VITTE_AST_NODE_IF_EXPR) {
                    vitte_c17_ast_write(emitter, ";\n");
                    vitte_c17_ast_emit_if_expression_into(
                        emitter,
                        initializer,
                        child,
                        indent);
                    if (vitte_c17_ast_debug_scalar_let(emitter, node)) {
                        vitte_c17_ast_emit_debug_scalar_update(
                            emitter,
                            child,
                            indent,
                            true);
                    }
                    return;
                }
                vitte_c17_ast_write(emitter, " = ");
                vitte_c17_ast_emit_expression(
                    emitter,
                    initializer);
            }
            vitte_c17_ast_write(emitter, ";\n");
            if (vitte_c17_ast_debug_scalar_let(emitter, node)) {
                vitte_c17_ast_emit_debug_scalar_update(
                    emitter,
                    child,
                    indent,
                    true);
            }
            return;

        case VITTE_AST_NODE_EXPR_STMT:
        case VITTE_AST_NODE_RETURN_STMT:
            if (node->kind == VITTE_AST_NODE_RETURN_STMT) {
                vitte_c17_ast_write(emitter, "return");
                if (node->child_count != 0u) {
                    vitte_c17_ast_write(emitter, " ");
                }
            }
            if (node->child_count != 0u) {
                vitte_c17_ast_emit_expression(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u));
            }
            vitte_c17_ast_write(emitter, ";\n");
            if (node->kind == VITTE_AST_NODE_EXPR_STMT &&
                node->child_count != 0u) {
                const vitte_ast_node_t *assigned_name;

                assigned_name = vitte_c17_ast_debug_assignment_name(
                    emitter,
                    vitte_c17_ast_child(emitter, node, 0u));
                vitte_c17_ast_emit_debug_scalar_update(
                    emitter,
                    assigned_name,
                    indent,
                    false);
            }
            return;

        case VITTE_AST_NODE_IF_STMT:
            vitte_c17_ast_write(emitter, "if ");
            vitte_c17_ast_emit_condition(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, " ");
            child = vitte_c17_ast_child(emitter, node, 1u);
            if (child == NULL || child->kind != VITTE_AST_NODE_BLOCK) {
                emitter->failed = true;
                return;
            }
            vitte_c17_ast_write(emitter, "{\n");
            vitte_c17_ast_emit_block_contents(
                emitter,
                child,
                indent + 1u);
            vitte_c17_ast_indent(emitter, indent);
            vitte_c17_ast_write(emitter, "}");
            if (node->child_count > 2u) {
                child = vitte_c17_ast_child(emitter, node, 2u);
                vitte_c17_ast_write(emitter, " else ");
                if (child != NULL &&
                    child->kind == VITTE_AST_NODE_IF_STMT) {
                    vitte_c17_ast_write(emitter, "\n");
                    vitte_c17_ast_emit_statement(
                        emitter,
                        child,
                        indent);
                    return;
                }
                if (child == NULL ||
                    child->kind != VITTE_AST_NODE_BLOCK) {
                    emitter->failed = true;
                    return;
                }
                vitte_c17_ast_write(emitter, "{\n");
                vitte_c17_ast_emit_block_contents(
                    emitter,
                    child,
                    indent + 1u);
                vitte_c17_ast_indent(emitter, indent);
                vitte_c17_ast_write(emitter, "}");
            }
            vitte_c17_ast_write(emitter, "\n");
            return;

        case VITTE_AST_NODE_WHILE_STMT:
            vitte_c17_ast_write(emitter, "while ");
            vitte_c17_ast_emit_condition(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, " {\n");
            child = vitte_c17_ast_child(emitter, node, 1u);
            vitte_c17_ast_emit_block_contents(
                emitter,
                child,
                indent + 1u);
            vitte_c17_ast_indent(emitter, indent);
            vitte_c17_ast_write(emitter, "}\n");
            return;

        case VITTE_AST_NODE_MATCH_STMT:
            if (node->child_count < 2u) {
                emitter->failed = true;
                return;
            }
            vitte_c17_ast_write(emitter, "switch (");
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, ") {\n");
            for (index = 1u; index < node->child_count; ++index) {
                const vitte_ast_node_t *arm;
                const vitte_ast_node_t *pattern;
                const vitte_ast_node_t *body;

                arm = vitte_c17_ast_child(emitter, node, index);
                if (arm == NULL ||
                    arm->kind != VITTE_AST_NODE_MATCH_ARM ||
                    arm->child_count != 2u) {
                    emitter->failed = true;
                    return;
                }
                pattern = vitte_c17_ast_child(emitter, arm, 0u);
                body = vitte_c17_ast_child(emitter, arm, 1u);
                vitte_c17_ast_indent(emitter, indent + 1u);
                if (pattern != NULL &&
                    pattern->kind == VITTE_AST_NODE_PATH &&
                    pattern->child_count == 1u &&
                    vitte_c17_ast_token_is(
                        emitter,
                        vitte_c17_ast_child(emitter, pattern, 0u),
                        "_")) {
                    vitte_c17_ast_write(emitter, "default:\n");
                } else {
                    vitte_c17_ast_write(emitter, "case ");
                    vitte_c17_ast_emit_expression(emitter, pattern);
                    vitte_c17_ast_write(emitter, ":\n");
                }
                if (body != NULL &&
                    body->kind == VITTE_AST_NODE_BLOCK) {
                    vitte_c17_ast_indent(emitter, indent + 2u);
                    vitte_c17_ast_write(emitter, "{\n");
                    vitte_c17_ast_emit_block_contents(
                        emitter,
                        body,
                        indent + 3u);
                    vitte_c17_ast_indent(emitter, indent + 3u);
                    vitte_c17_ast_write(emitter, "break;\n");
                    vitte_c17_ast_indent(emitter, indent + 2u);
                    vitte_c17_ast_write(emitter, "}\n");
                } else {
                    vitte_c17_ast_indent(emitter, indent + 2u);
                    vitte_c17_ast_write(emitter, "return ");
                    vitte_c17_ast_emit_expression(emitter, body);
                    vitte_c17_ast_write(emitter, ";\n");
                }
            }
            vitte_c17_ast_indent(emitter, indent);
            vitte_c17_ast_write(emitter, "}\n");
            return;

        case VITTE_AST_NODE_BREAK_STMT:
            vitte_c17_ast_write(emitter, "break;\n");
            return;

        case VITTE_AST_NODE_CONTINUE_STMT:
            vitte_c17_ast_write(emitter, "continue;\n");
            return;

        case VITTE_AST_NODE_ASSERT_STMT:
            vitte_c17_ast_write(emitter, "assert(");
            vitte_c17_ast_emit_expression(
                emitter,
                vitte_c17_ast_child(emitter, node, 0u));
            vitte_c17_ast_write(emitter, ");\n");
            return;

        default:
            emitter->failed = true;
            return;
    }
}

static const vitte_ast_node_t *
vitte_c17_ast_find_child_kind(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node,
    vitte_ast_node_kind_t kind)
{
    size_t index;

    if (node == NULL) {
        return NULL;
    }

    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *child;

        child = vitte_c17_ast_child(emitter, node, index);
        if (child != NULL && child->kind == kind) {
            return child;
        }
    }

    return NULL;
}

static void
vitte_c17_ast_emit_parameter_list(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *function)
{
    size_t index;
    bool first;

    first = true;
    for (index = 1u; index < function->child_count; ++index) {
        const vitte_ast_node_t *parameter;

        parameter = vitte_c17_ast_child(emitter, function, index);
        if (parameter == NULL ||
            parameter->kind != VITTE_AST_NODE_PARAMETER) {
            continue;
        }

        if (!first) {
            vitte_c17_ast_write(emitter, ", ");
        }
        vitte_c17_ast_emit_type(
            emitter,
            vitte_c17_ast_child(emitter, parameter, 1u));
        vitte_c17_ast_write(emitter, " ");
        vitte_c17_ast_write_token(
            emitter,
            vitte_c17_ast_child(emitter, parameter, 0u));
        first = false;
    }

    if (first) {
        vitte_c17_ast_write(emitter, "void");
    }
}

static void
vitte_c17_ast_emit_function_signature(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *function,
    bool prototype)
{
    const vitte_ast_node_t *return_type;
    const vitte_ast_node_t *name;

    name = vitte_c17_ast_child(emitter, function, 0u);
    return_type =
        vitte_c17_ast_find_child_kind(
            emitter,
            function,
            VITTE_AST_NODE_TYPE_EXPR);

    if (return_type == NULL) {
        vitte_c17_ast_write(emitter, "void ");
    } else {
        vitte_c17_ast_emit_type(emitter, return_type);
        vitte_c17_ast_write(emitter, " ");
    }

    if (emitter->space_depth == 0u &&
        vitte_c17_ast_token_is(emitter, name, "main")) {
        vitte_c17_ast_write(emitter, "vitte_entry_main");
    } else {
        vitte_c17_ast_emit_function_name(emitter, name);
    }
    vitte_c17_ast_write(emitter, "(");
    vitte_c17_ast_emit_parameter_list(emitter, function);
    vitte_c17_ast_write(
        emitter,
        prototype ? ");\n" : ") {\n");
}

static bool
vitte_c17_ast_emit_form(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name;
    size_t index;

    name = vitte_c17_ast_child(emitter, node, 0u);
    if (name == NULL) {
        return false;
    }

    vitte_c17_ast_write(emitter, "struct ");
    vitte_c17_ast_write_token(emitter, name);
    vitte_c17_ast_write(emitter, " {\n");
    for (index = 1u; index < node->child_count; ++index) {
        const vitte_ast_node_t *field;

        field = vitte_c17_ast_child(emitter, node, index);
        if (field == NULL ||
            field->kind != VITTE_AST_NODE_FIELD_DECL) {
            continue;
        }
        vitte_c17_ast_write(emitter, "    ");
        vitte_c17_ast_emit_declarator(
            emitter,
            vitte_c17_ast_child(emitter, field, 1u),
            vitte_c17_ast_child(emitter, field, 0u));
        vitte_c17_ast_write(emitter, ";\n");
    }
    vitte_c17_ast_write(emitter, "};\n\n");
    return !emitter->failed;
}

static void
vitte_c17_ast_emit_forms_in_scope(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *scope,
    bool forward_declarations)
{
    size_t index;

    if (scope == NULL || emitter->failed) {
        return;
    }
    for (index = 0u; index < scope->child_count; ++index) {
        const vitte_ast_node_t *declaration;

        declaration = vitte_c17_ast_child(emitter, scope, index);
        if (declaration == NULL) {
            emitter->failed = true;
            return;
        }

        if (declaration->kind == VITTE_AST_NODE_FORM_DECL) {
            const vitte_ast_node_t *name;

            name = vitte_c17_ast_child(emitter, declaration, 0u);
            if (name == NULL) {
                emitter->failed = true;
                return;
            }
            if (forward_declarations) {
                vitte_c17_ast_write(emitter, "typedef struct ");
                vitte_c17_ast_write_token(emitter, name);
                vitte_c17_ast_write(emitter, " ");
                vitte_c17_ast_write_token(emitter, name);
                vitte_c17_ast_write(emitter, ";\n");
            } else {
                (void)vitte_c17_ast_emit_form(emitter, declaration);
            }
        } else if (declaration->kind == VITTE_AST_NODE_SPACE_DECL &&
                   declaration->child_count > 1u) {
            const vitte_ast_node_t *body;

            body = vitte_c17_ast_child(emitter, declaration, 1u);
            if (body == NULL ||
                body->kind != VITTE_AST_NODE_BLOCK) {
                emitter->failed = true;
                return;
            }
            vitte_c17_ast_emit_forms_in_scope(
                emitter,
                body,
                forward_declarations);
        }
    }
}

static bool
vitte_c17_ast_emit_pick(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name;
    size_t index;

    name = vitte_c17_ast_child(emitter, node, 0u);
    if (name == NULL) {
        return false;
    }

    vitte_c17_ast_write(emitter, "typedef enum ");
    vitte_c17_ast_write_token(emitter, name);
    vitte_c17_ast_write(emitter, " {\n");
    for (index = 1u; index < node->child_count; ++index) {
        const vitte_ast_node_t *variant;

        variant = vitte_c17_ast_child(emitter, node, index);
        if (variant == NULL ||
            variant->kind != VITTE_AST_NODE_VARIANT_DECL) {
            continue;
        }
        vitte_c17_ast_write(emitter, "    ");
        vitte_c17_ast_write_token(emitter, name);
        vitte_c17_ast_write(emitter, "_");
        vitte_c17_ast_write_token(
            emitter,
            vitte_c17_ast_child(emitter, variant, 0u));
        vitte_c17_ast_write(
            emitter,
            index + 1u < node->child_count ? ",\n" : "\n");
    }
    vitte_c17_ast_write(emitter, "} ");
    vitte_c17_ast_write_token(emitter, name);
    vitte_c17_ast_write(emitter, ";\n\n");
    return !emitter->failed;
}

static bool
vitte_c17_ast_emit_constant(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name;
    const vitte_ast_node_t *type;
    const vitte_ast_node_t *initializer;

    name = vitte_c17_ast_child(emitter, node, 0u);
    type = vitte_c17_ast_child(emitter, node, 1u);
    initializer =
        node->child_count > 2u
            ? vitte_c17_ast_child(emitter, node, 2u)
            : vitte_c17_ast_child(emitter, node, 1u);
    if (name == NULL || type == NULL || initializer == NULL) {
        return false;
    }

    vitte_c17_ast_write(emitter, "static const ");
    vitte_c17_ast_emit_type(emitter, type);
    vitte_c17_ast_write(emitter, " ");
    vitte_c17_ast_write_token(emitter, name);
    vitte_c17_ast_write(emitter, " = ");
    vitte_c17_ast_emit_expression(emitter, initializer);
    vitte_c17_ast_write(emitter, ";\n");
    return !emitter->failed;
}

static bool
vitte_c17_ast_is_terminal_intrinsic(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name;

    if (node == NULL ||
        node->kind != VITTE_AST_NODE_EXTERN_PROC_DECL) {
        return false;
    }
    name = vitte_c17_ast_child(emitter, node, 0u);
    return vitte_c17_ast_token_is(emitter, name, "terminal_clear") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_hide_cursor") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_show_cursor") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_print") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_print_i64") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_key_pressed") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_read_key") ||
           vitte_c17_ast_token_is(emitter, name, "terminal_sleep_ms");
}

static bool
vitte_c17_ast_is_intrinsic(
    const vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name;

    if (node == NULL ||
        node->kind != VITTE_AST_NODE_EXTERN_PROC_DECL) {
        return false;
    }
    name = vitte_c17_ast_child(emitter, node, 0u);
    return vitte_c17_ast_token_is(emitter, name, "io_print") ||
           vitte_c17_ast_token_is(emitter, name, "io_print_i64") ||
           vitte_c17_ast_token_is(emitter, name, "io_read_i64") ||
           vitte_c17_ast_token_is(emitter, name, "random_i64") ||
           vitte_c17_ast_is_terminal_intrinsic(emitter, node);
}

static bool
vitte_c17_ast_emit_function(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *function)
{
    const vitte_ast_node_t *body;
    const vitte_ast_node_t *previous;

    body =
        vitte_c17_ast_find_child_kind(
            emitter,
            function,
            VITTE_AST_NODE_BLOCK);
    if (body == NULL) {
        return false;
    }

    previous = emitter->function;
    emitter->function = function;
    vitte_c17_ast_emit_source_line(emitter, function);
    vitte_c17_ast_emit_function_signature(
        emitter,
        function,
        false);
    vitte_c17_ast_emit_block_contents(
        emitter,
        body,
        1u);
    vitte_c17_ast_write(emitter, "}\n\n");
    emitter->function = previous;
    return !emitter->failed;
}

static void
vitte_c17_ast_emit_space_functions(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *scope,
    bool prototypes)
{
    size_t index;

    if (scope == NULL || emitter->failed) {
        return;
    }

    for (index = 0u;
         index < scope->child_count && !emitter->failed;
         ++index) {
        const vitte_ast_node_t *declaration;

        declaration = vitte_c17_ast_child(emitter, scope, index);
        if (declaration == NULL) {
            emitter->failed = true;
            return;
        }
        if (declaration->kind != VITTE_AST_NODE_SPACE_DECL) {
            continue;
        }
        if (declaration->child_count < 2u) {
            continue;
        }
        if (emitter->space_depth >=
            sizeof(emitter->space_stack) /
                sizeof(emitter->space_stack[0])) {
            emitter->failed = true;
            return;
        }

        emitter->space_stack[emitter->space_depth++] = declaration;
        {
            const vitte_ast_node_t *body;
            size_t child_index;

            body = vitte_c17_ast_child(emitter, declaration, 1u);
            if (body == NULL ||
                body->kind != VITTE_AST_NODE_BLOCK) {
                emitter->failed = true;
            } else {
                for (child_index = 0u;
                     child_index < body->child_count &&
                     !emitter->failed;
                     ++child_index) {
                    const vitte_ast_node_t *member;

                    member = vitte_c17_ast_child(
                        emitter,
                        body,
                        child_index);
                    if (member == NULL) {
                        emitter->failed = true;
                        break;
                    }
                    if (member->kind == VITTE_AST_NODE_PROC_DECL) {
                        if (prototypes) {
                            vitte_c17_ast_emit_function_signature(
                                emitter,
                                member,
                                true);
                        } else if (!vitte_c17_ast_emit_function(
                                       emitter,
                                       member)) {
                            emitter->failed = true;
                        }
                    } else if (member->kind ==
                                   VITTE_AST_NODE_EXTERN_PROC_DECL &&
                               prototypes &&
                               !vitte_c17_ast_is_intrinsic(
                                   emitter,
                                   member)) {
                        vitte_c17_ast_emit_function_signature(
                            emitter,
                            member,
                            true);
                    }
                }
                vitte_c17_ast_emit_space_functions(
                    emitter,
                    body,
                    prototypes);
            }
        }
        --emitter->space_depth;
    }
}

static bool
vitte_c17_ast_emit_test(
    vitte_c17_ast_emitter_t *emitter,
    const vitte_ast_node_t *test,
    size_t test_index)
{
    const vitte_ast_node_t *body;
    const vitte_ast_node_t *previous;

    body =
        vitte_c17_ast_find_child_kind(
            emitter,
            test,
            VITTE_AST_NODE_BLOCK);
    if (body == NULL) {
        return false;
    }

    vitte_c17_ast_emit_source_line(emitter, test);
    if (fprintf(
            emitter->output,
            "static void vitte_test_%zu(void) {\n",
            test_index) < 0) {
        emitter->failed = true;
        return false;
    }

    previous = emitter->function;
    emitter->function = NULL;
    vitte_c17_ast_emit_block_contents(
        emitter,
        body,
        1u);
    vitte_c17_ast_write(emitter, "}\n\n");
    emitter->function = previous;
    return !emitter->failed;
}

bool
vitte_c17_emit_ast_with_options(
    FILE *output,
    const vitte_parser_t *parser,
    const vitte_c17_compile_options_t *options)
{
    vitte_c17_ast_emitter_t emitter;
    vitte_c17_compile_options_t default_options;
    const vitte_ast_node_t *root;
    size_t index;
    size_t test_count;
    bool has_main;
    bool has_terminal_api;

    if (output == NULL ||
        parser == NULL ||
        vitte_parser_root(parser) == VITTE_AST_INVALID_ID) {
        return false;
    }

    memset(&default_options, 0, sizeof(default_options));
    if (options == NULL) {
        options = &default_options;
    }

    memset(&emitter, 0, sizeof(emitter));
    emitter.output = output;
    emitter.parser = parser;
    emitter.source_path = options->source_path;
    emitter.debug_info = options->debug_info;
    emitter.debug_runtime = options->debug_runtime;
    emitter.emit_line_directives = options->emit_line_directives;

    root =
        vitte_parser_get_node(
            parser,
            vitte_parser_root(parser));
    if (root == NULL ||
        root->kind != VITTE_AST_NODE_TRANSLATION_UNIT) {
        return false;
    }

    has_terminal_api = false;
    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *node;

        node = vitte_c17_ast_child(&emitter, root, index);
        if (vitte_c17_ast_is_terminal_intrinsic(&emitter, node)) {
            has_terminal_api = true;
            break;
        }
    }

    vitte_c17_ast_write(
        &emitter,
        "#if defined(__APPLE__) || defined(__unix__)\n"
        "#define _POSIX_C_SOURCE 200809L\n"
        "#endif\n"
        "#include <assert.h>\n"
        "#include <inttypes.h>\n"
        "#include <stdbool.h>\n"
        "#include <stddef.h>\n"
        "#include <stdint.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <time.h>\n"
        "#if defined(__APPLE__) || defined(__unix__)\n"
        "#include <arpa/inet.h>\n"
        "#include <errno.h>\n"
        "#include <netinet/in.h>\n"
        "#include <sys/socket.h>\n"
        "#include <sys/select.h>\n"
        "#include <termios.h>\n"
        "#include <unistd.h>\n"
        "#endif\n"
        "\n");

    if (emitter.debug_runtime) {
        vitte_c17_ast_emit_debug_runtime(&emitter);
    }

    vitte_c17_ast_emit_forms_in_scope(
        &emitter,
        root,
        true);
    vitte_c17_ast_write(&emitter, "\n");

    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *node;

        node = vitte_c17_ast_child(&emitter, root, index);
        if (node != NULL && node->kind == VITTE_AST_NODE_PICK_DECL) {
            (void)vitte_c17_ast_emit_pick(&emitter, node);
        }
    }

    vitte_c17_ast_emit_forms_in_scope(
        &emitter,
        root,
        false);

    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *node;

        node = vitte_c17_ast_child(&emitter, root, index);
        if (node != NULL &&
            (node->kind == VITTE_AST_NODE_CONST_DECL ||
             node->kind == VITTE_AST_NODE_STATIC_DECL)) {
            (void)vitte_c17_ast_emit_constant(&emitter, node);
        }
    }
    vitte_c17_ast_write(&emitter, "\n");

    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *node;

        node = vitte_c17_ast_child(&emitter, root, index);
        if (node != NULL && node->kind == VITTE_AST_NODE_PROC_DECL) {
            vitte_c17_ast_emit_function_signature(
                &emitter,
                node,
                true);
        } else if (node != NULL &&
                   node->kind == VITTE_AST_NODE_EXTERN_PROC_DECL &&
                   !vitte_c17_ast_is_intrinsic(&emitter, node)) {
            vitte_c17_ast_emit_function_signature(
                &emitter,
                node,
                true);
        }
    }
    vitte_c17_ast_emit_space_functions(
        &emitter,
        root,
        true);

    vitte_c17_ast_write(
        &emitter,
        "\n"
        "static bool vitte_string_is_space(unsigned char value) {\n"
        "    return value == (unsigned char)' ' ||\n"
        "           value == (unsigned char)'\\t' ||\n"
        "           value == (unsigned char)'\\n' ||\n"
        "           value == (unsigned char)'\\r' ||\n"
        "           value == (unsigned char)'\\f' ||\n"
        "           value == (unsigned char)'\\v';\n"
        "}\n"
        "int64_t vitte_string_length(const char *value) {\n"
        "    return value == NULL ? 0 : (int64_t)strlen(value);\n"
        "}\n"
        "const char *vitte_string_trim(const char *value) {\n"
        "    const char *begin;\n"
        "    const char *end;\n"
        "    char *copy;\n"
        "    size_t length;\n"
        "    if (value == NULL) { return \"\"; }\n"
        "    begin = value;\n"
        "    while (*begin != '\\0' &&\n"
        "           vitte_string_is_space((unsigned char)*begin)) {\n"
        "        ++begin;\n"
        "    }\n"
        "    end = value + strlen(value);\n"
        "    while (end > begin &&\n"
        "           vitte_string_is_space((unsigned char)end[-1])) {\n"
        "        --end;\n"
        "    }\n"
        "    length = (size_t)(end - begin);\n"
        "    copy = (char *)malloc(length + 1u);\n"
        "    if (copy == NULL) { return \"\"; }\n"
        "    memcpy(copy, begin, length);\n"
        "    copy[length] = '\\0';\n"
        "    return copy;\n"
        "}\n"
        "bool vitte_string_starts_with(\n"
        "    const char *value,\n"
        "    const char *prefix) {\n"
        "    size_t prefix_length;\n"
        "    if (value == NULL || prefix == NULL) { return false; }\n"
        "    prefix_length = strlen(prefix);\n"
        "    return strlen(value) >= prefix_length &&\n"
        "           memcmp(value, prefix, prefix_length) == 0;\n"
        "}\n"
        "bool vitte_string_ends_with(\n"
        "    const char *value,\n"
        "    const char *suffix) {\n"
        "    size_t value_length;\n"
        "    size_t suffix_length;\n"
        "    if (value == NULL || suffix == NULL) { return false; }\n"
        "    value_length = strlen(value);\n"
        "    suffix_length = strlen(suffix);\n"
        "    return value_length >= suffix_length &&\n"
        "           memcmp(value + value_length - suffix_length,\n"
        "                  suffix, suffix_length) == 0;\n"
        "}\n"
        "void io_print(const char *text) {\n"
        "    if (text != NULL) { (void)fputs(text, stdout); }\n"
        "}\n"
        "void io_print_i64(int64_t value) {\n"
        "    (void)printf(\"%\" PRId64, value);\n"
        "}\n"
        "int64_t io_read_i64(void) {\n"
        "    int64_t value = 0;\n"
        "    if (scanf(\"%\" SCNd64, &value) != 1) { return 0; }\n"
        "    return value;\n"
        "}\n"
        "int64_t random_i64(int64_t minimum, int64_t maximum) {\n"
        "    uint64_t width;\n"
        "    uint64_t offset;\n"
        "    if (maximum < minimum) { return minimum; }\n"
        "    width = (uint64_t)maximum - (uint64_t)minimum + UINT64_C(1);\n"
        "    if (width == 0u) { return (int64_t)rand(); }\n"
        "    offset = (uint64_t)rand() % width;\n"
        "    return minimum + (int64_t)offset;\n"
        "}\n\n");

    if (has_terminal_api) {
        vitte_c17_ast_write(
            &emitter,
        "void terminal_clear(void) {\n"
        "    (void)fputs(\"\\x1b[2J\\x1b[H\", stdout);\n"
        "    (void)fflush(stdout);\n"
        "}\n"
        "void terminal_print(const char *text) {\n"
        "    if (text != NULL) { (void)fputs(text, stdout); }\n"
        "    (void)fflush(stdout);\n"
        "}\n"
        "void terminal_print_i64(int64_t value) {\n"
        "    (void)printf(\"%\" PRId64, value);\n"
        "    (void)fflush(stdout);\n"
        "}\n"
        "#if defined(__APPLE__) || defined(__unix__)\n"
        "static struct termios vitte_terminal_original;\n"
        "static bool vitte_terminal_original_valid;\n"
        "void terminal_hide_cursor(void) {\n"
        "    struct termios raw;\n"
        "    if (!vitte_terminal_original_valid &&\n"
        "        isatty(STDIN_FILENO) != 0 &&\n"
        "        tcgetattr(STDIN_FILENO, &vitte_terminal_original) == 0) {\n"
        "        raw = vitte_terminal_original;\n"
        "        raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);\n"
        "        raw.c_cc[VMIN] = 0;\n"
        "        raw.c_cc[VTIME] = 0;\n"
        "        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {\n"
        "            vitte_terminal_original_valid = true;\n"
        "        }\n"
        "    }\n"
        "    (void)fputs(\"\\x1b[?25l\", stdout);\n"
        "    (void)fflush(stdout);\n"
        "}\n"
        "void terminal_show_cursor(void) {\n"
        "    if (vitte_terminal_original_valid) {\n"
        "        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH,\n"
        "                        &vitte_terminal_original);\n"
        "        vitte_terminal_original_valid = false;\n"
        "    }\n"
        "    (void)fputs(\"\\x1b[?25h\", stdout);\n"
        "    (void)fflush(stdout);\n"
        "}\n"
        "bool terminal_key_pressed(void) {\n"
        "    fd_set read_set;\n"
        "    struct timeval timeout = { 0, 0 };\n"
        "    int result;\n"
        "    FD_ZERO(&read_set);\n"
        "    FD_SET(STDIN_FILENO, &read_set);\n"
        "    result = select(STDIN_FILENO + 1, &read_set, NULL, NULL,\n"
        "                    &timeout);\n"
        "    return result > 0 && FD_ISSET(STDIN_FILENO, &read_set);\n"
        "}\n"
        "char terminal_read_key(void) {\n"
        "    unsigned char key;\n"
        "    return read(STDIN_FILENO, &key, 1u) == 1 ? (char)key : '\\0';\n"
        "}\n"
        "void terminal_sleep_ms(int64_t milliseconds) {\n"
        "    struct timespec delay;\n"
        "    struct timespec remaining;\n"
        "    if (milliseconds <= 0) { return; }\n"
        "    delay.tv_sec = (time_t)(milliseconds / INT64_C(1000));\n"
        "    delay.tv_nsec = (long)((milliseconds % INT64_C(1000)) *\n"
        "                           INT64_C(1000000));\n"
        "    while (nanosleep(&delay, &remaining) != 0 && errno == EINTR) {\n"
        "        delay = remaining;\n"
        "    }\n"
        "}\n"
        "#else\n"
        "void terminal_hide_cursor(void) { }\n"
        "void terminal_show_cursor(void) { }\n"
        "bool terminal_key_pressed(void) { return false; }\n"
        "char terminal_read_key(void) { return '\\0'; }\n"
        "void terminal_sleep_ms(int64_t milliseconds) {\n"
        "    clock_t start = clock();\n"
        "    clock_t ticks = (clock_t)((double)milliseconds *\n"
        "                              (double)CLOCKS_PER_SEC / 1000.0);\n"
        "    while (milliseconds > 0 && clock() - start < ticks) { }\n"
        "}\n"
        "#endif\n\n");
    }

    has_main = false;
    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *node;
        const vitte_ast_node_t *name;

        node = vitte_c17_ast_child(&emitter, root, index);
        if (node == NULL ||
            node->kind != VITTE_AST_NODE_PROC_DECL) {
            continue;
        }
        name = vitte_c17_ast_child(&emitter, node, 0u);
        if (vitte_c17_ast_token_is(&emitter, name, "main")) {
            has_main = true;
            break;
        }
    }
    test_count = 0u;
    for (index = 0u; index < root->child_count; ++index) {
        const vitte_ast_node_t *node;

        node = vitte_c17_ast_child(&emitter, root, index);
        if (node != NULL && node->kind == VITTE_AST_NODE_PROC_DECL) {
            if (!vitte_c17_ast_emit_function(&emitter, node)) {
                break;
            }
        } else if (node != NULL &&
                   node->kind == VITTE_AST_NODE_TEST_DECL) {
            if (!has_main) {
                if (!vitte_c17_ast_emit_test(
                        &emitter,
                        node,
                        test_count)) {
                    break;
                }
                ++test_count;
            }
        } else if (node != NULL &&
                   node->kind == VITTE_AST_NODE_EXTERN_PROC_DECL &&
                   !vitte_c17_ast_is_intrinsic(&emitter, node)) {
            continue;
        } else if (node != NULL &&
                   node->kind != VITTE_AST_NODE_SPACE_DECL &&
                   node->kind != VITTE_AST_NODE_USE_DECL &&
                   node->kind != VITTE_AST_NODE_TEST_DECL &&
                   node->kind != VITTE_AST_NODE_FORM_DECL &&
                   node->kind != VITTE_AST_NODE_PICK_DECL &&
                   node->kind != VITTE_AST_NODE_CONST_DECL &&
                   node->kind != VITTE_AST_NODE_STATIC_DECL &&
                   node->kind != VITTE_AST_NODE_EXTERN_PROC_DECL) {
            emitter.failed = true;
            break;
        }
    }
    vitte_c17_ast_emit_space_functions(
        &emitter,
        root,
        false);

    if (has_main && !emitter.failed) {
        vitte_c17_ast_write(
            &emitter,
            "int main(void) {\n"
            "    srand((unsigned int)1u);\n"
            "    return (int)vitte_entry_main();\n"
            "}\n");
    } else if (test_count != 0u && !emitter.failed) {
        vitte_c17_ast_write(
            &emitter,
            "int main(void) {\n");
        for (index = 0u; index < test_count; ++index) {
            vitte_c17_ast_indent(&emitter, 1u);
            if (fprintf(
                    emitter.output,
                    "vitte_test_%zu();\n",
                    index) < 0) {
                emitter.failed = true;
                break;
            }
        }
        vitte_c17_ast_write(
            &emitter,
            "    return 0;\n"
            "}\n");
    }

    return !emitter.failed && fflush(output) == 0;
}

bool
vitte_c17_emit_ast(
    FILE *output,
    const vitte_parser_t *parser)
{
    return vitte_c17_emit_ast_with_options(output, parser, NULL);
}

#if defined(__APPLE__) || defined(__unix__)
static bool
vitte_c17_wait_for_process(
    pid_t process,
    int *exit_code)
{
    int status;
    pid_t waited;

    do {
        waited = waitpid(process, &status, 0);
    } while (waited < 0 && errno == EINTR);

    if (waited != process ||
        !WIFEXITED(status)) {
        return false;
    }

    *exit_code = WEXITSTATUS(status);
    return true;
}

static bool
vitte_c17_spawn(
    const char *const arguments[],
    bool inherit_environment)
{
    pid_t process;
    int exit_code;
    int result;

    (void)inherit_environment;
    result = posix_spawnp(
        &process,
        arguments[0],
        NULL,
        NULL,
        (char *const *)arguments,
        environ);
    if (result != 0) {
        return false;
    }

    return vitte_c17_wait_for_process(process, &exit_code) &&
           exit_code == 0;
}
#endif

bool
vitte_c17_compile_ast_with_options(
    const vitte_parser_t *parser,
    const char *output_path,
    bool emit_c,
    bool run,
    int run_argc,
    char **run_argv,
    const vitte_c17_compile_options_t *options)
{
    vitte_c17_compile_options_t default_options;

    memset(&default_options, 0, sizeof(default_options));
    if (options == NULL) {
        options = &default_options;
    }

    if (emit_c) {
        FILE *output;
        bool close_output;
        bool success;

        close_output = output_path != NULL;
        output = close_output
            ? fopen(output_path, "wb")
            : stdout;
        if (output == NULL) {
            return false;
        }

        success = vitte_c17_emit_ast_with_options(output, parser, options);
        if (close_output && fclose(output) != 0) {
            success = false;
        }
        return success;
    }

#if defined(__APPLE__) || defined(__unix__)
    {
        char source_path[] = "/tmp/vitte-source-XXXXXX";
        char executable_path[] = "/tmp/vitte-program-XXXXXX";
        int source_descriptor;
        int executable_descriptor;
        FILE *source_file;
        bool success;
        const char *compile_arguments[16];
        size_t compile_argument_count;

        source_descriptor = mkstemp(source_path);
        if (source_descriptor < 0) {
            return false;
        }

        source_file = fdopen(source_descriptor, "w");
        if (source_file == NULL) {
            (void)close(source_descriptor);
            (void)unlink(source_path);
            return false;
        }

        success = vitte_c17_emit_ast_with_options(
            source_file,
            parser,
            options);
        if (fclose(source_file) != 0) {
            success = false;
        }
        if (!success) {
            (void)unlink(source_path);
            return false;
        }

        if (run) {
            executable_descriptor = mkstemp(executable_path);
            if (executable_descriptor < 0) {
                (void)unlink(source_path);
                return false;
            }
            (void)close(executable_descriptor);
            if (unlink(executable_path) != 0) {
                (void)unlink(source_path);
                return false;
            }
        } else {
            if (output_path == NULL) {
                output_path = "a.out";
            }
            executable_path[0] = '\0';
        }

        compile_argument_count = 0u;
        compile_arguments[compile_argument_count++] = "cc";
        if (options->debug_info) {
            /* Keep the generated C easy to inspect and preserve frame roots. */
            compile_arguments[compile_argument_count++] = "-g";
            compile_arguments[compile_argument_count++] = "-O0";
            compile_arguments[compile_argument_count++] =
                "-fno-omit-frame-pointer";
            compile_arguments[compile_argument_count++] = "-fno-inline";
            if (options->debug_runtime) {
                compile_arguments[compile_argument_count++] =
                    "-DVITTE_DEBUG_RUNTIME";
            }
        }
        compile_arguments[compile_argument_count++] = "-std=c17";
        compile_arguments[compile_argument_count++] = "-x";
        compile_arguments[compile_argument_count++] = "c";
        compile_arguments[compile_argument_count++] = "-o";
        compile_arguments[compile_argument_count++] =
            run ? executable_path : output_path;
        compile_arguments[compile_argument_count++] = source_path;
        compile_arguments[compile_argument_count] = NULL;
        success = vitte_c17_spawn(compile_arguments, true);
        (void)unlink(source_path);

        if (success && run) {
            char **run_arguments;
            size_t argument_count;
            size_t index;
            pid_t process;
            int exit_code;
            int spawn_result;

            if (run_argc < 0 ||
                (run_argc != 0 && run_argv == NULL)) {
                (void)unlink(executable_path);
                return false;
            }
            argument_count = (size_t)run_argc;
            if (argument_count > SIZE_MAX / sizeof(*run_arguments) - 2u) {
                (void)unlink(executable_path);
                return false;
            }
            run_arguments = (char **)calloc(
                argument_count + 2u,
                sizeof(*run_arguments));
            if (run_arguments == NULL) {
                (void)unlink(executable_path);
                return false;
            }
            run_arguments[0] = executable_path;
            for (index = 0u; index < argument_count; ++index) {
                run_arguments[index + 1u] = run_argv[index];
            }
            spawn_result = posix_spawn(
                &process,
                executable_path,
                NULL,
                NULL,
                run_arguments,
                environ);
            free(run_arguments);
            success = spawn_result == 0 &&
                      vitte_c17_wait_for_process(process, &exit_code) &&
                      exit_code == 0;
            (void)unlink(executable_path);
        }

        return success;
    }
#else
    (void)parser;
    (void)output_path;
    (void)run;
    (void)run_argc;
    (void)run_argv;
    return false;
#endif
}

bool
vitte_c17_compile_ast(
    const vitte_parser_t *parser,
    const char *output_path,
    bool emit_c,
    bool run,
    int run_argc,
    char **run_argv)
{
    return vitte_c17_compile_ast_with_options(
        parser,
        output_path,
        emit_c,
        run,
        run_argc,
        run_argv,
        NULL);
}
