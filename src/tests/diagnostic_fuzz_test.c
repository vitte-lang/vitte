#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "diagnostic/diagnostic.h"

static uint32_t
next_random(
    uint32_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

int
main(void)
{
    static const char source_name[] = "fuzz.vit";
    uint32_t state = UINT32_C(0x56495454);
    size_t iteration;

    for (iteration = 0u; iteration < 1000u; ++iteration) {
        size_t length;
        vitte_ast_span_t span;
        vitte_diagnostic_t storage[1];
        vitte_diagnostic_bag_t bag;
        FILE *stream;

        length =
            (size_t)(next_random(&state) % UINT32_C(256));

        (void)memset(&span, 0, sizeof(span));
        span.source_name = source_name;
        span.start_offset =
            length == 0u
                ? 0u
                : (size_t)(next_random(&state) %
                           (uint32_t)(length + 1u));
        span.end_offset =
            span.start_offset +
            (size_t)(next_random(&state) %
                     (uint32_t)(length - span.start_offset + 1u));
        span.start_line =
            (uint32_t)(next_random(&state) % UINT32_C(100));
        span.start_column =
            (uint32_t)(next_random(&state) % UINT32_C(120));
        span.end_line = span.start_line;
        span.end_column =
            span.start_column +
            (uint32_t)(span.end_offset - span.start_offset);
        span.valid = true;

        assert(vitte_diagnostic_bag_init(
                   &bag,
                   storage,
                   1u) == VITTE_STATUS_OK);
        assert(vitte_diagnostic_emit(
                   &bag,
                   VITTE_DIAGNOSTIC_ERROR,
                   VITTE_DIAGNOSTIC_ORIGIN_INTERNAL,
                   VITTE_INFRA_E_INTERNAL,
                   "fuzz diagnostic",
                   &span) == VITTE_STATUS_OK);

        stream = tmpfile();
        assert(stream != NULL);
        assert(vitte_diagnostic_write_all_format(
                   stream,
                   &bag,
                   VITTE_DIAGNOSTIC_FORMAT_TERMINAL) ==
               VITTE_STATUS_OK);
        assert(vitte_diagnostic_write_all_format(
                   stream,
                   &bag,
                   VITTE_DIAGNOSTIC_FORMAT_JSON) ==
               VITTE_STATUS_OK);
        assert(vitte_diagnostic_write_all_format(
                   stream,
                   &bag,
                   VITTE_DIAGNOSTIC_FORMAT_SARIF) ==
               VITTE_STATUS_OK);
        assert(vitte_diagnostic_write_all_format(
                   stream,
                   &bag,
                   VITTE_DIAGNOSTIC_FORMAT_LSP) ==
               VITTE_STATUS_OK);
        assert(fclose(stream) == 0);
        vitte_diagnostic_bag_reset(&bag);
    }

    return 0;
}
