#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "compiler/lexer.h"

#define TEST(name) printf("TEST: %s\n", name)
#define ASSERT(cond) assert(cond)
#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_STR_EQ(a, b) assert(strcmp((a), (b)) == 0)

void test_lexer_empty(void) {
    TEST("lexer_empty");
    
    lexer_t *lex = lexer_create("", 0);
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(count, 1);  /* EOF token */
    ASSERT_EQ(tokens[0].kind, TOK_EOF);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_identifier(void) {
    TEST("lexer_identifier");
    
    const char *src = "hello world";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_IDENT);
    ASSERT_EQ(tokens[0].line, 1);
    ASSERT_EQ(tokens[0].col, 1);
    ASSERT_EQ(tokens[1].kind, TOK_IDENT);
    ASSERT_EQ(tokens[2].kind, TOK_EOF);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_keywords(void) {
    TEST("lexer_keywords");
    
    const char *src = "fn let if else while for";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_KEYWORD);  /* fn */
    ASSERT_EQ(tokens[1].kind, TOK_KEYWORD);  /* let */
    ASSERT_EQ(tokens[2].kind, TOK_KEYWORD);  /* if */
    ASSERT_EQ(tokens[3].kind, TOK_KEYWORD);  /* else */
    ASSERT_EQ(tokens[4].kind, TOK_KEYWORD);  /* while */
    ASSERT_EQ(tokens[5].kind, TOK_KEYWORD);  /* for */
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_integers(void) {
    TEST("lexer_integers");
    
    const char *src = "42 0x2a 0b101010 0o52";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_INT);   /* 42 */
    ASSERT_EQ(tokens[1].kind, TOK_INT);   /* 0x2a */
    ASSERT_EQ(tokens[2].kind, TOK_INT);   /* 0b101010 */
    ASSERT_EQ(tokens[3].kind, TOK_INT);   /* 0o52 */
    ASSERT_EQ(tokens[4].kind, TOK_EOF);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_floats(void) {
    TEST("lexer_floats");
    
    const char *src = "3.14 1.0 2e10 3.5e-2";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_FLOAT);  /* 3.14 */
    ASSERT_EQ(tokens[1].kind, TOK_FLOAT);  /* 1.0 */
    ASSERT_EQ(tokens[2].kind, TOK_FLOAT);  /* 2e10 */
    ASSERT_EQ(tokens[3].kind, TOK_FLOAT);  /* 3.5e-2 */
    ASSERT_EQ(tokens[4].kind, TOK_EOF);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_strings(void) {
    TEST("lexer_strings");
    
    const char *src = "\"hello\" 'world'";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_STRING);
    ASSERT_EQ(tokens[1].kind, TOK_STRING);
    ASSERT_EQ(tokens[2].kind, TOK_EOF);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_punctuation(void) {
    TEST("lexer_punctuation");
    
    const char *src = "() [] {} , ; . :";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_LPAREN);
    ASSERT_EQ(tokens[1].kind, TOK_RPAREN);
    ASSERT_EQ(tokens[2].kind, TOK_LBRACK);
    ASSERT_EQ(tokens[3].kind, TOK_RBRACK);
    ASSERT_EQ(tokens[4].kind, TOK_LBRACE);
    ASSERT_EQ(tokens[5].kind, TOK_RBRACE);
    ASSERT_EQ(tokens[6].kind, TOK_COMMA);
    ASSERT_EQ(tokens[7].kind, TOK_SEMICOLON);
    ASSERT_EQ(tokens[8].kind, TOK_DOT);
    ASSERT_EQ(tokens[9].kind, TOK_COLON);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_operators(void) {
    TEST("lexer_operators");
    
    const char *src = "+ - * / % = < > !";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].kind, TOK_PLUS);
    ASSERT_EQ(tokens[1].kind, TOK_MINUS);
    ASSERT_EQ(tokens[2].kind, TOK_STAR);
    ASSERT_EQ(tokens[3].kind, TOK_SLASH);
    ASSERT_EQ(tokens[4].kind, TOK_PERCENT);
    ASSERT_EQ(tokens[5].kind, TOK_ASSIGN);
    ASSERT_EQ(tokens[6].kind, TOK_LT);
    ASSERT_EQ(tokens[7].kind, TOK_GT);
    ASSERT_EQ(tokens[8].kind, TOK_BANG);
    
    free(tokens);
    lexer_destroy(lex);
}

void test_lexer_line_tracking(void) {
    TEST("lexer_line_tracking");
    
    const char *src = "a\nb\nc";
    lexer_t *lex = lexer_create(src, strlen(src));
    ASSERT(lex != NULL);
    
    uint32_t count = 0;
    token_t *tokens = lexer_tokenize(lex, &count);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(tokens[0].line, 1);  /* a */
    ASSERT_EQ(tokens[1].line, 2);  /* b */
    ASSERT_EQ(tokens[2].line, 3);  /* c */
    
    free(tokens);
    lexer_destroy(lex);
}

int main(void) {
    printf("=== Lexer Unit Tests ===\n\n");
    
    test_lexer_empty();
    test_lexer_identifier();
    test_lexer_keywords();
    test_lexer_integers();
    test_lexer_floats();
    test_lexer_strings();
    test_lexer_punctuation();
    test_lexer_operators();
    test_lexer_line_tracking();
    
    printf("\n=== All tests passed! ===\n");
    
    return 0;
}
