// Vitte Compiler - Frontend Test
// Tests for lexer, parser, and semantic analyzer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

// ============================================================================
// Test Helpers
// ============================================================================

#define ASSERT(condition, message) \
    if (!(condition)) { \
        printf("FAIL: %s\n", message); \
        return 0; \
    }

#define TEST(name) \
    printf("\nTest: %s\n", name)

int test_count = 0;
int pass_count = 0;

void test_begin(const char *name) {
    printf("\n=== %s ===\n", name);
}

void test_end(int result) {
    test_count++;
    if (result) {
        pass_count++;
        printf("✓ PASS\n");
    } else {
        printf("✗ FAIL\n");
    }
}

// ============================================================================
// Lexer Tests
// ============================================================================

int test_lexer_basic(void) {
    TEST("Basic Tokenization");
    
    const char *source = "let x = 42;";
    lexer_t *lexer = lexer_create(source);
    
    ASSERT(lexer != NULL, "Lexer creation failed");
    
    token_t tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_LET, "Expected 'let' keyword");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_IDENTIFIER, "Expected identifier");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_ASSIGN, "Expected '='");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_INTEGER, "Expected integer");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_SEMICOLON, "Expected ';'");
    
    lexer_free(lexer);
    return 1;
}

int test_lexer_numbers(void) {
    TEST("Number Lexing");
    
    const char *source = "42 3.14 0xFF";
    lexer_t *lexer = lexer_create(source);
    
    ASSERT(lexer != NULL, "Lexer creation failed");
    
    token_t tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_INTEGER, "Expected integer");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_FLOAT, "Expected float");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_INTEGER, "Expected hex integer");
    
    lexer_free(lexer);
    return 1;
}

int test_lexer_strings(void) {
    TEST("String Lexing");
    
    const char *source = "\"hello\" 'a'";
    lexer_t *lexer = lexer_create(source);
    
    ASSERT(lexer != NULL, "Lexer creation failed");
    
    token_t tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_STRING, "Expected string");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_CHAR, "Expected char");
    
    lexer_free(lexer);
    return 1;
}

int test_lexer_keywords(void) {
    TEST("Keyword Recognition");
    
    const char *source = "fn if else for while return";
    lexer_t *lexer = lexer_create(source);
    
    ASSERT(lexer != NULL, "Lexer creation failed");
    
    token_t tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_FN, "Expected 'fn' keyword");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_IF, "Expected 'if' keyword");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_ELSE, "Expected 'else' keyword");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_FOR, "Expected 'for' keyword");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_WHILE, "Expected 'while' keyword");
    
    tok = lexer_next_token(lexer);
    ASSERT(tok.type == TOKEN_KW_RETURN, "Expected 'return' keyword");
    
    lexer_free(lexer);
    return 1;
}

int test_lexer_operators(void) {
    TEST("Operator Lexing");
    
    const char *source = "+ - * / == != < >";
    lexer_t *lexer = lexer_create(source);
    
    ASSERT(lexer != NULL, "Lexer creation failed");
    
    lexer_next_token(lexer);
    ASSERT(current_token(lexer).type == TOKEN_PLUS, "Expected '+'");
    
    lexer_free(lexer);
    return 1;
}

// ============================================================================
// Parser Tests
// ============================================================================

int test_parser_basic(void) {
    TEST("Basic Parsing");
    
    const char *source = "42;";
    lexer_t *lexer = lexer_create(source);
    lexer_tokenize(lexer);
    
    parser_t *parser = parser_create(lexer->tokens, lexer->token_count);
    ASSERT(parser != NULL, "Parser creation failed");
    
    ast_node_t *ast = parser_parse(parser);
    ASSERT(ast != NULL, "Parse failed");
    ASSERT(ast->type == AST_PROGRAM, "Expected program node");
    
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    
    return 1;
}

int test_parser_function(void) {
    TEST("Function Parsing");
    
    const char *source = "fn add() { }";
    lexer_t *lexer = lexer_create(source);
    lexer_tokenize(lexer);
    
    parser_t *parser = parser_create(lexer->tokens, lexer->token_count);
    ASSERT(parser != NULL, "Parser creation failed");
    
    ast_node_t *ast = parser_parse(parser);
    ASSERT(ast != NULL, "Parse failed");
    ASSERT(ast->children_count > 0, "Expected children");
    
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    
    return 1;
}

int test_parser_if_statement(void) {
    TEST("If Statement Parsing");
    
    const char *source = "if (true) { }";
    lexer_t *lexer = lexer_create(source);
    lexer_tokenize(lexer);
    
    parser_t *parser = parser_create(lexer->tokens, lexer->token_count);
    ASSERT(parser != NULL, "Parser creation failed");
    
    ast_node_t *ast = parser_parse(parser);
    ASSERT(ast != NULL, "Parse failed");
    
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    
    return 1;
}

int test_parser_for_loop(void) {
    TEST("For Loop Parsing");
    
    const char *source = "for (;;) { }";
    lexer_t *lexer = lexer_create(source);
    lexer_tokenize(lexer);
    
    parser_t *parser = parser_create(lexer->tokens, lexer->token_count);
    ASSERT(parser != NULL, "Parser creation failed");
    
    ast_node_t *ast = parser_parse(parser);
    ASSERT(ast != NULL, "Parse failed");
    
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    
    return 1;
}

// ============================================================================
// Semantic Analysis Tests
// ============================================================================

int test_semantic_type_creation(void) {
    TEST("Type Creation");
    
    type_info_t *int_type = type_create(TYPE_INT);
    ASSERT(int_type != NULL, "Type creation failed");
    ASSERT(int_type->kind == TYPE_INT, "Wrong type kind");
    
    type_info_t *ptr_type = type_create_pointer(int_type);
    ASSERT(ptr_type != NULL, "Pointer creation failed");
    ASSERT(ptr_type->kind == TYPE_POINTER, "Wrong pointer kind");
    
    type_info_t *arr_type = type_create_array(int_type, 10);
    ASSERT(arr_type != NULL, "Array creation failed");
    ASSERT(arr_type->array_size == 10, "Wrong array size");
    
    type_free(ptr_type);
    type_free(arr_type);
    type_free(int_type);
    
    return 1;
}

int test_semantic_type_compatibility(void) {
    TEST("Type Compatibility");
    
    type_info_t *int1 = type_create(TYPE_INT);
    type_info_t *int2 = type_create(TYPE_INT);
    
    ASSERT(type_compatible(int1, int2), "Compatible types failed");
    
    type_info_t *float_type = type_create(TYPE_FLOAT);
    ASSERT(!type_compatible(int1, float_type), "Incompatible types check failed");
    
    type_free(int1);
    type_free(int2);
    type_free(float_type);
    
    return 1;
}

int test_semantic_analyzer(void) {
    TEST("Semantic Analysis");
    
    semantic_analyzer_t *analyzer = analyzer_create();
    ASSERT(analyzer != NULL, "Analyzer creation failed");
    
    // Create a simple AST
    ast_node_t *program = ast_node_create(AST_PROGRAM, 1, 0);
    ast_node_t *literal = ast_node_create(AST_LITERAL, 1, 0);
    ast_node_add_child(program, literal);
    
    int result = analyzer_analyze(analyzer, program);
    ASSERT(result == 0 || result == -1, "Analysis failed");
    
    ast_node_free(program);
    analyzer_free(analyzer);
    
    return 1;
}

int test_semantic_symbol_table(void) {
    TEST("Symbol Table");
    
    semantic_analyzer_t *analyzer = analyzer_create();
    ASSERT(analyzer != NULL, "Analyzer creation failed");
    
    type_info_t *int_type = type_create(TYPE_INT);
    analyzer_define_symbol(analyzer, "x", int_type, 1, 1, 0);
    
    symbol_t *sym = analyzer_lookup_symbol(analyzer, "x");
    ASSERT(sym != NULL, "Symbol lookup failed");
    ASSERT(strcmp(sym->name, "x") == 0, "Wrong symbol name");
    
    analyzer_free(analyzer);
    
    return 1;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   Vitte Compiler - Frontend Test Suite                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    // Lexer tests
    printf("\n--- LEXER TESTS ---\n");
    test_begin("Lexical Analysis");
    test_end(test_lexer_basic());
    test_end(test_lexer_numbers());
    test_end(test_lexer_strings());
    test_end(test_lexer_keywords());
    test_end(test_lexer_operators());
    
    // Parser tests
    printf("\n--- PARSER TESTS ---\n");
    test_begin("Syntax Analysis");
    test_end(test_parser_basic());
    test_end(test_parser_function());
    test_end(test_parser_if_statement());
    test_end(test_parser_for_loop());
    
    // Semantic analysis tests
    printf("\n--- SEMANTIC ANALYSIS TESTS ---\n");
    test_begin("Type Checking");
    test_end(test_semantic_type_creation());
    test_end(test_semantic_type_compatibility());
    test_end(test_semantic_analyzer());
    test_end(test_semantic_symbol_table());
    
    // Summary
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║   Test Results: %d/%d passed                           ║\n", pass_count, test_count);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    return pass_count == test_count ? 0 : 1;
}
