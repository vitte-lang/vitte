#include "lexer/lexer.h"
#include "parser/parser.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static uint32_t
next_random(uint32_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

int
main(void)
{
    uint32_t state = UINT32_C(0x56495454);
    size_t iteration;

    for (iteration = 0u; iteration < 10000u; ++iteration) {
        char source[1024];
        size_t length = (size_t)(next_random(&state) % UINT32_C(1024));
        vitte_lexer_t lexer;
        vitte_parser_t parser;
        size_t index;

        for (index = 0u; index < length; ++index) {
            source[index] = (char)(next_random(&state) & UINT32_C(0xff));
        }

        assert(vitte_lexer_init(&lexer, source, length, UINT32_C(99)));
        if (!vitte_lexer_run(&lexer)) {
            assert(vitte_lexer_is_valid(&lexer));
            vitte_lexer_destroy(&lexer);
            continue;
        }

        assert(vitte_lexer_validate(&lexer));
        assert(vitte_lexer_token_count(&lexer) > 0u);
        (void)vitte_lexer_fingerprint(&lexer);

        if (vitte_parser_init(
                &parser,
                lexer.tokens,
                lexer.token_count)) {
            (void)vitte_parser_run(&parser);
            if (!vitte_parser_has_failed(&parser)) {
                assert(vitte_parser_validate(&parser));
                (void)vitte_parser_fingerprint(&parser);
            }
            vitte_parser_destroy(&parser);
        }

        vitte_lexer_destroy(&lexer);
    }

    return 0;
}
