#include <stdio.h>
#include <assert.h>
#include "../../include/compiler/lexer.h"

void test_lexer_basic(void) {
    const char *src = "foo 123 \"hi\" ( ) ; , + -";
    Token *t = lexer_tokenize(src);
    assert(t[0].type == TOKEN_IDENTIFIER);
    assert(t[1].type == TOKEN_NUMBER);
    assert(t[2].type == TOKEN_STRING);
    /* find EOF */
    size_t i = 0;
    while (t[i].type != TOKEN_EOF) i++;
    lexer_free(t);
    printf("✓ test_lexer_basic passed\n");
}

int main(void) {
    test_lexer_basic();
    return 0;
}
