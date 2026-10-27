#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "diagnostic/diagnostic.h"
#include "diagnostic/render_lsp.h"

/* Model the random byte spans on an ASCII line of the generated length. */
static bool
ascii_position(vitte_source_id_t id, const char *name, size_t offset,
               vitte_diagnostic_lsp_position_encoding_t encoding,
               vitte_diagnostic_lsp_position_t *position, void *user_data)
{
    const size_t *length = (const size_t *)user_data;
    (void)id; (void)name; (void)encoding;
    if (offset > *length) return false;
    position->line = 0u;
    position->character = offset;
    position->valid = true;
    return true;
}

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
        vitte_diagnostic_lsp_options_t lsp;

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
               VITTE_STATUS_ERROR_INVALID_STATE); /* LSP needs a position converter. */
        vitte_diagnostic_lsp_options_init(&lsp);
        lsp.position_converter = ascii_position;
        lsp.position_user_data = &length;
        assert(vitte_diagnostic_render_lsp_array(stream, &bag, NULL, &lsp) == VITTE_STATUS_OK);
        assert(fclose(stream) == 0);
        vitte_diagnostic_bag_reset(&bag);
    }

    return 0;
}
