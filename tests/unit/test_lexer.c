#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../compiler/include/compiler/lexer.h"

static token_t* tokenize_src(const char* src, uint32_t* out_count) {
    lexer_t* lex = lexer_create(src, (uint32_t)strlen(src));
    assert(lex != NULL);
    assert(out_count != NULL);

    token_t* tokens = lexer_tokenize(lex, out_count);
    lexer_destroy(lex);
    assert(tokens != NULL);
    return tokens;
}

void test_lexer_basic(void) {
    const char* src = "foo 123 \"hi\" ( ) ; , + -";
    uint32_t count = 0;
    token_t* t = tokenize_src(src, &count);

    assert(count >= 9);
    assert(t[0].kind == TOK_IDENT);
    assert(t[1].kind == TOK_INT);
    assert(t[2].kind == TOK_STRING);
    assert(t[3].kind == TOK_LPAREN);
    assert(t[4].kind == TOK_RPAREN);
    assert(t[5].kind == TOK_SEMICOLON);
    assert(t[6].kind == TOK_COMMA);
    assert(t[7].kind == TOK_PLUS);
    assert(t[8].kind == TOK_MINUS);

    uint32_t i = 0;
    while (i < count && t[i].kind != TOK_EOF) {
        i++;
    }
    assert(i < count);

    free(t);
    printf("test_lexer_basic passed\n");
}

int main(void) {
    test_lexer_basic();
    return 0;
}
