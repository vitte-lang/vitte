#include "import/import.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t
next_random(uint32_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static size_t
iteration_limit(size_t fallback)
{
    const char *text = getenv("VITTE_FUZZ_ITERATIONS");
    size_t value = 0u;

    if (text == NULL || text[0] == '\0') {
        return fallback;
    }
    while (*text >= '0' && *text <= '9') {
        size_t digit = (size_t)(*text - '0');
        if (value > (SIZE_MAX - digit) / 10u) {
            return fallback;
        }
        value = value * 10u + digit;
        ++text;
    }
    return *text == '\0' ? value : fallback;
}

int
main(void)
{
    uint32_t state = UINT32_C(0x1A2B3C4D);
    size_t iteration;

    for (iteration = 0u; iteration < iteration_limit(2000u); ++iteration) {
        vitte_import_context_t context;
        vitte_import_module_id_t modules[8];
        vitte_import_string_id_t names[8];
        size_t module_count;
        size_t index;

        assert(vitte_import_init(&context));
        module_count = 1u +
            (size_t)(next_random(&state) % UINT32_C(8));

        for (index = 0u; index < module_count; ++index) {
            char name[32];
            int length;
            vitte_import_module_desc_t description;

            length = snprintf(name, sizeof(name), "fuzz::m%zu", index);
            assert(length > 0);
            names[index] = vitte_import_intern_string(
                &context,
                name,
                (size_t)length);
            assert(names[index] != VITTE_IMPORT_INVALID_ID);

            description = vitte_import_module_desc_empty();
            description.name = names[index];
            description.is_root = index == 0u;
            modules[index] = vitte_import_add_module(
                &context,
                &description);
            assert(modules[index] != VITTE_IMPORT_INVALID_ID);
        }

        for (index = 0u; index < module_count; ++index) {
            size_t edge_count =
                (size_t)(next_random(&state) % UINT32_C(4));
            size_t edge_index;

            for (edge_index = 0u; edge_index < edge_count; ++edge_index) {
                vitte_import_desc_t description;
                size_t target =
                    (size_t)(next_random(&state) %
                             (uint32_t)module_count);

                description = vitte_import_desc_empty();
                description.source_module = modules[index];
                description.module_name = names[target];
                description.span = vitte_import_span_make(
                    (uint32_t)(index + 1u),
                    edge_index,
                    edge_index + 1u);
                (void)vitte_import_add(&context, &description);
            }
        }

        (void)vitte_import_resolve(&context);
        assert(vitte_import_is_valid(&context));
        (void)vitte_import_fingerprint(&context);
        vitte_import_destroy(&context);
    }

    return 0;
}
