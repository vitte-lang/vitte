// Vitte Compiler - Parser Implementation
// Syntax analysis and AST construction

#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Helper Functions
// ============================================================================

static token_t current_token(parser_t *parser) {
    if (parser->current >= parser->token_count) {
        return (token_t){.type = TOKEN_EOF};
    }
    return parser->tokens[parser->current];
}

static token_t peek_token(parser_t *parser, int offset) {
    if (parser->current + offset >= parser->token_count) {
        return (token_t){.type = TOKEN_EOF};
    }
    return parser->tokens[parser->current + offset];
}

static void advance(parser_t *parser) {
    if (parser->current < parser->token_count) {
        parser->current++;
    }
}

static bool match(parser_t *parser, token_type_t type) {
    if (current_token(parser).type == type) {
        advance(parser);
        return true;
    }
    return false;
}

static bool check(parser_t *parser, token_type_t type) {
    return current_token(parser).type == type;
}

static void add_error(parser_t *parser, const char *message) {
    if (parser->error_count >= parser->error_capacity) {
        parser->error_capacity *= 2;
        char **new_errors = realloc(parser->errors, 
            sizeof(char*) * parser->error_capacity);
        if (!new_errors) return;
        parser->errors = new_errors;
    }
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Line %d: %s", 
        current_token(parser).line, message);
    
    parser->errors[parser->error_count++] = malloc(strlen(buffer) + 1);
    if (parser->errors[parser->error_count - 1]) {
        strcpy(parser->errors[parser->error_count - 1], buffer);
    }
}

// ============================================================================
// AST Node Management
// ============================================================================

ast_node_t* ast_node_create(ast_node_type_t type, int line, int column) {
    ast_node_t *node = malloc(sizeof(ast_node_t));
    if (!node) return NULL;
    
    node->type = type;
    node->line = line;
    node->column = column;
    node->data = NULL;
    node->children = NULL;
    node->children_count = 0;
    node->annotation = NULL;
    
    return node;
}

void ast_node_add_child(ast_node_t *parent, ast_node_t *child) {
    if (!parent || !child) return;
    
    ast_node_t **new_children = realloc(parent->children,
        sizeof(ast_node_t*) * (parent->children_count + 1));
    if (!new_children) return;
    
    parent->children = new_children;
    parent->children[parent->children_count++] = child;
}

void ast_node_free(ast_node_t *node) {
    if (!node) return;
    
    for (size_t i = 0; i < node->children_count; i++) {
        ast_node_free(node->children[i]);
    }
    
    free(node->children);
    free(node->data);
    free(node->annotation);
    free(node);
}

const char* ast_node_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_PROGRAM: return "Program";
        case AST_FUNCTION: return "Function";
        case AST_VARIABLE: return "Variable";
        case AST_ASSIGNMENT: return "Assignment";
        case AST_BINARY_OP: return "BinaryOp";
        case AST_UNARY_OP: return "UnaryOp";
        case AST_CALL: return "Call";
        case AST_IF: return "If";
        case AST_FOR: return "For";
        case AST_WHILE: return "While";
        case AST_RETURN: return "Return";
        case AST_BREAK: return "Break";
        case AST_CONTINUE: return "Continue";
        case AST_BLOCK: return "Block";
        case AST_LITERAL: return "Literal";
        case AST_IDENTIFIER: return "Identifier";
        case AST_TYPE: return "Type";
        case AST_STRUCT: return "Struct";
        case AST_ENUM: return "Enum";
        case AST_TRAIT: return "Trait";
        case AST_MATCH: return "Match";
        case AST_ARRAY: return "Array";
        case AST_INDEX: return "Index";
        case AST_MEMBER: return "Member";
        default: return "Unknown";
    }
}

void ast_print(ast_node_t *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    printf("%s", ast_node_type_name(node->type));
    
    if (node->annotation) {
        printf(" <%s>", node->annotation);
    }
    
    printf(" (line %d)\n", node->line);
    
    for (size_t i = 0; i < node->children_count; i++) {
        ast_print(node->children[i], indent + 1);
    }
}

// ============================================================================
// Expression Parsing
// ============================================================================

static ast_node_t* parse_primary(parser_t *parser);
static ast_node_t* parse_assignment(parser_t *parser);

static ast_node_t* parse_primary(parser_t *parser) {
    token_t token = current_token(parser);
    
    // Literals
    if (token.type == TOKEN_INTEGER || token.type == TOKEN_FLOAT ||
        token.type == TOKEN_STRING || token.type == TOKEN_CHAR) {
        ast_node_t *node = ast_node_create(AST_LITERAL, token.line, token.column);
        advance(parser);
        return node;
    }
    
    // Identifiers
    if (token.type == TOKEN_IDENTIFIER) {
        ast_node_t *node = ast_node_create(AST_IDENTIFIER, token.line, token.column);
        advance(parser);
        
        // Function call
        if (check(parser, TOKEN_LPAREN)) {
            advance(parser);
            ast_node_t *call = ast_node_create(AST_CALL, token.line, token.column);
            ast_node_add_child(call, node);
            
            while (!check(parser, TOKEN_RPAREN) && 
                   !check(parser, TOKEN_EOF)) {
                ast_node_t *arg = parse_assignment(parser);
                ast_node_add_child(call, arg);
                
                if (!match(parser, TOKEN_COMMA)) break;
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                add_error(parser, "Expected ')' after arguments");
            }
            
            return call;
        }
        
        return node;
    }
    
    // Grouped expression
    if (match(parser, TOKEN_LPAREN)) {
        ast_node_t *expr = parse_assignment(parser);
        if (!match(parser, TOKEN_RPAREN)) {
            add_error(parser, "Expected ')' after expression");
        }
        return expr;
    }
    
    // Boolean literals
    if (match(parser, TOKEN_KW_TRUE) || match(parser, TOKEN_KW_FALSE)) {
        return ast_node_create(AST_LITERAL, token.line, token.column);
    }
    
    // Nil literal
    if (match(parser, TOKEN_NIL)) {
        return ast_node_create(AST_LITERAL, token.line, token.column);
    }
    
    add_error(parser, "Unexpected token in expression");
    advance(parser);
    return ast_node_create(AST_LITERAL, token.line, token.column);
}

static ast_node_t* parse_unary(parser_t *parser) {
    token_t token = current_token(parser);
    
    if (match(parser, TOKEN_NOT) || match(parser, TOKEN_MINUS) ||
        match(parser, TOKEN_PLUS) || match(parser, TOKEN_BIT_NOT) ||
        match(parser, TOKEN_STAR) || match(parser, TOKEN_BIT_AND)) {
        ast_node_t *node = ast_node_create(AST_UNARY_OP, token.line, token.column);
        ast_node_t *expr = parse_unary(parser);
        ast_node_add_child(node, expr);
        return node;
    }
    
    return parse_primary(parser);
}

static ast_node_t* parse_term(parser_t *parser) {
    ast_node_t *expr = parse_unary(parser);
    
    while (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) ||
           check(parser, TOKEN_PERCENT)) {
        token_t op = current_token(parser);
        advance(parser);
        
        ast_node_t *binary = ast_node_create(AST_BINARY_OP, op.line, op.column);
        ast_node_add_child(binary, expr);
        ast_node_add_child(binary, parse_unary(parser));
        expr = binary;
    }
    
    return expr;
}

static ast_node_t* parse_additive(parser_t *parser) {
    ast_node_t *expr = parse_term(parser);
    
    while (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
        token_t op = current_token(parser);
        advance(parser);
        
        ast_node_t *binary = ast_node_create(AST_BINARY_OP, op.line, op.column);
        ast_node_add_child(binary, expr);
        ast_node_add_child(binary, parse_term(parser));
        expr = binary;
    }
    
    return expr;
}

static ast_node_t* parse_comparison(parser_t *parser) {
    ast_node_t *expr = parse_additive(parser);
    
    while (check(parser, TOKEN_LT) || check(parser, TOKEN_LE) ||
           check(parser, TOKEN_GT) || check(parser, TOKEN_GE)) {
        token_t op = current_token(parser);
        advance(parser);
        
        ast_node_t *binary = ast_node_create(AST_BINARY_OP, op.line, op.column);
        ast_node_add_child(binary, expr);
        ast_node_add_child(binary, parse_additive(parser));
        expr = binary;
    }
    
    return expr;
}

static ast_node_t* parse_equality(parser_t *parser) {
    ast_node_t *expr = parse_comparison(parser);
    
    while (check(parser, TOKEN_EQ) || check(parser, TOKEN_NE)) {
        token_t op = current_token(parser);
        advance(parser);
        
        ast_node_t *binary = ast_node_create(AST_BINARY_OP, op.line, op.column);
        ast_node_add_child(binary, expr);
        ast_node_add_child(binary, parse_comparison(parser));
        expr = binary;
    }
    
    return expr;
}

static ast_node_t* parse_logical_and(parser_t *parser) {
    ast_node_t *expr = parse_equality(parser);
    
    while (match(parser, TOKEN_AND)) {
        token_t op = peek_token(parser, -1);
        ast_node_t *binary = ast_node_create(AST_BINARY_OP, op.line, op.column);
        ast_node_add_child(binary, expr);
        ast_node_add_child(binary, parse_equality(parser));
        expr = binary;
    }
    
    return expr;
}

static ast_node_t* parse_logical_or(parser_t *parser) {
    ast_node_t *expr = parse_logical_and(parser);
    
    while (match(parser, TOKEN_OR)) {
        token_t op = peek_token(parser, -1);
        ast_node_t *binary = ast_node_create(AST_BINARY_OP, op.line, op.column);
        ast_node_add_child(binary, expr);
        ast_node_add_child(binary, parse_logical_and(parser));
        expr = binary;
    }
    
    return expr;
}

static ast_node_t* parse_assignment(parser_t *parser) {
    ast_node_t *expr = parse_logical_or(parser);
    
    if (check(parser, TOKEN_ASSIGN) || check(parser, TOKEN_PLUS_ASSIGN) ||
        check(parser, TOKEN_MINUS_ASSIGN) || check(parser, TOKEN_STAR_ASSIGN) ||
        check(parser, TOKEN_SLASH_ASSIGN)) {
        token_t op = current_token(parser);
        advance(parser);
        
        ast_node_t *assign = ast_node_create(AST_ASSIGNMENT, op.line, op.column);
        ast_node_add_child(assign, expr);
        ast_node_add_child(assign, parse_assignment(parser));
        return assign;
    }
    
    return expr;
}

// ============================================================================
// Statement Parsing
// ============================================================================

static ast_node_t* parse_block(parser_t *parser);
static ast_node_t* parse_statement(parser_t *parser);

static ast_node_t* parse_statement(parser_t *parser) {
    token_t token = current_token(parser);
    
    // Return statement
    if (match(parser, TOKEN_KW_RETURN)) {
        ast_node_t *node = ast_node_create(AST_RETURN, token.line, token.column);
        if (!check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_NEWLINE)) {
            ast_node_add_child(node, parse_assignment(parser));
        }
        match(parser, TOKEN_SEMICOLON);
        return node;
    }
    
    // Break statement
    if (match(parser, TOKEN_KW_BREAK)) {
        ast_node_t *node = ast_node_create(AST_BREAK, token.line, token.column);
        match(parser, TOKEN_SEMICOLON);
        return node;
    }
    
    // Continue statement
    if (match(parser, TOKEN_KW_CONTINUE)) {
        ast_node_t *node = ast_node_create(AST_CONTINUE, token.line, token.column);
        match(parser, TOKEN_SEMICOLON);
        return node;
    }
    
    // If statement
    if (match(parser, TOKEN_KW_IF)) {
        ast_node_t *node = ast_node_create(AST_IF, token.line, token.column);
        if (!match(parser, TOKEN_LPAREN)) {
            add_error(parser, "Expected '(' after 'if'");
        }
        ast_node_add_child(node, parse_assignment(parser));
        if (!match(parser, TOKEN_RPAREN)) {
            add_error(parser, "Expected ')' after condition");
        }
        ast_node_add_child(node, parse_statement(parser));
        
        if (match(parser, TOKEN_KW_ELSE)) {
            ast_node_add_child(node, parse_statement(parser));
        }
        return node;
    }
    
    // For loop
    if (match(parser, TOKEN_KW_FOR)) {
        ast_node_t *node = ast_node_create(AST_FOR, token.line, token.column);
        if (!match(parser, TOKEN_LPAREN)) {
            add_error(parser, "Expected '(' after 'for'");
        }
        // Parse init, condition, update
        ast_node_add_child(node, parse_assignment(parser));
        if (!match(parser, TOKEN_SEMICOLON)) {
            add_error(parser, "Expected ';' after for init");
        }
        ast_node_add_child(node, parse_assignment(parser));
        if (!match(parser, TOKEN_SEMICOLON)) {
            add_error(parser, "Expected ';' after for condition");
        }
        ast_node_add_child(node, parse_assignment(parser));
        if (!match(parser, TOKEN_RPAREN)) {
            add_error(parser, "Expected ')' after for clauses");
        }
        ast_node_add_child(node, parse_statement(parser));
        return node;
    }
    
    // While loop
    if (match(parser, TOKEN_KW_WHILE)) {
        ast_node_t *node = ast_node_create(AST_WHILE, token.line, token.column);
        if (!match(parser, TOKEN_LPAREN)) {
            add_error(parser, "Expected '(' after 'while'");
        }
        ast_node_add_child(node, parse_assignment(parser));
        if (!match(parser, TOKEN_RPAREN)) {
            add_error(parser, "Expected ')' after condition");
        }
        ast_node_add_child(node, parse_statement(parser));
        return node;
    }
    
    // Block statement
    if (check(parser, TOKEN_LBRACE)) {
        return parse_block(parser);
    }
    
    // Expression statement
    ast_node_t *expr = parse_assignment(parser);
    match(parser, TOKEN_SEMICOLON);
    return expr;
}

static ast_node_t* parse_block(parser_t *parser) {
    token_t token = current_token(parser);
    if (!match(parser, TOKEN_LBRACE)) {
        add_error(parser, "Expected '{'");
        return ast_node_create(AST_BLOCK, token.line, token.column);
    }
    
    ast_node_t *block = ast_node_create(AST_BLOCK, token.line, token.column);
    
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        ast_node_add_child(block, parse_statement(parser));
    }
    
    if (!match(parser, TOKEN_RBRACE)) {
        add_error(parser, "Expected '}'");
    }
    
    return block;
}

// ============================================================================
// Function and Type Parsing
// ============================================================================

static ast_node_t* parse_function(parser_t *parser) {
    token_t token = current_token(parser);
    if (!match(parser, TOKEN_KW_FN)) {
        add_error(parser, "Expected 'fn'");
        return ast_node_create(AST_FUNCTION, token.line, token.column);
    }
    
    ast_node_t *func = ast_node_create(AST_FUNCTION, token.line, token.column);
    
    // Function name
    if (match(parser, TOKEN_IDENTIFIER)) {
        // Parameters, return type, body
        if (match(parser, TOKEN_LPAREN)) {
            while (!check(parser, TOKEN_RPAREN) && !check(parser, TOKEN_EOF)) {
                match(parser, TOKEN_IDENTIFIER);  // param name
                match(parser, TOKEN_COLON);
                match(parser, TOKEN_IDENTIFIER);  // param type
                if (!check(parser, TOKEN_RPAREN)) match(parser, TOKEN_COMMA);
            }
            if (!match(parser, TOKEN_RPAREN)) {
                add_error(parser, "Expected ')' after parameters");
            }
        }
        
        // Return type
        if (match(parser, TOKEN_ARROW)) {
            match(parser, TOKEN_IDENTIFIER);
        }
        
        // Body
        ast_node_add_child(func, parse_block(parser));
    }
    
    return func;
}

// ============================================================================
// Program Parsing
// ============================================================================

ast_node_t* parser_parse(parser_t *parser) {
    if (!parser) return NULL;
    
    ast_node_t *program = ast_node_create(AST_PROGRAM, 1, 0);
    
    while (!check(parser, TOKEN_EOF)) {
        if (check(parser, TOKEN_KW_FN)) {
            ast_node_add_child(program, parse_function(parser));
        } else {
            ast_node_add_child(program, parse_statement(parser));
        }
    }
    
    return program;
}

ast_node_t* parser_parse_expression(parser_t *parser) {
    if (!parser) return NULL;
    return parse_assignment(parser);
}

ast_node_t* parser_parse_statement(parser_t *parser) {
    if (!parser) return NULL;
    return parse_statement(parser);
}

// ============================================================================
// Parser Management
// ============================================================================

parser_t* parser_create(token_t *tokens, size_t token_count) {
    if (!tokens) return NULL;
    
    parser_t *parser = malloc(sizeof(parser_t));
    if (!parser) return NULL;
    
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->error_count = 0;
    parser->error_capacity = 32;
    parser->errors = malloc(sizeof(char*) * parser->error_capacity);
    
    if (!parser->errors) {
        free(parser);
        return NULL;
    }
    
    return parser;
}

const char** parser_get_errors(parser_t *parser, size_t *count) {
    if (!parser) {
        *count = 0;
        return NULL;
    }
    *count = parser->error_count;
    return (const char**)parser->errors;
}

void parser_free(parser_t *parser) {
    if (!parser) return;
    
    for (size_t i = 0; i < parser->error_count; i++) {
        free(parser->errors[i]);
    }
    free(parser->errors);
    free(parser);
}
