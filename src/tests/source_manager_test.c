#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source/source.h"

int main(void) {
    static const char text[] = "first\r\nsecond\n";
    vitte_source_context_t context;
    vitte_source_location_t location;
    vitte_source_slice_t slice;
    const vitte_source_entry_t *entry;
    vitte_source_id_t id;

    assert(vitte_source_init(&context));
    assert(vitte_source_add(
        &context,
        "test.vit",
        "test.vit",
        text,
        strlen(text),
        &id));
    assert(id != VITTE_SOURCE_ID_INVALID);
    entry = vitte_source_find_path(&context, "test.vit");
    assert(entry != NULL && entry->id == id);
    entry = vitte_source_get(&context, id);
    assert(entry != NULL);
    assert(entry->line_count == 3u);
    assert(entry->length == strlen(text));
    assert(vitte_source_location_from_offset(&context, id, 7u, &location));
    assert(location.line == 2u && location.column == 1u);
    assert(vitte_source_get_line(&context, id, 1u, &slice));
    assert(slice.begin == 0u && slice.length == 5u);
    assert(memcmp(slice.data, "first", 5u) == 0);
    assert(vitte_source_get_line(&context, id, 2u, &slice));
    assert(slice.begin == 7u && slice.length == 6u);
    assert(memcmp(slice.data, "second", 6u) == 0);
    assert(vitte_source_get_line(&context, id, 3u, &slice));
    assert(slice.length == 0u && slice.begin == strlen(text));
    assert(!vitte_source_location_from_offset(
        &context,
        id,
        strlen(text) + 1u,
        &location));
    assert(vitte_source_get(&context, VITTE_SOURCE_ID_INVALID) == NULL);
    vitte_source_destroy(&context);

    {
        static const char unicode_text[] = "café αβ 🙂\nseconde ligne\n";
        assert(vitte_source_init(&context));
        assert(vitte_source_add(
            &context,
            "unicodé.vit",
            "unicodé.vit",
            unicode_text,
            strlen(unicode_text),
            &id));
        assert(vitte_source_get_line(&context, id, 1u, &slice));
        assert(slice.length == strlen("café αβ 🙂"));
        assert(memcmp(slice.data, "café αβ 🙂", slice.length) == 0);
        assert(vitte_source_location_from_offset(
            &context,
            id,
            strlen("café αβ 🙂\n"),
            &location));
        assert(location.line == 2u && location.column == 1u);
        vitte_source_destroy(&context);
    }

    /* A large source exercises line-index allocation, offset arithmetic and
       cleanup without relying on the compiler driver. */
    {
        enum { LARGE_SOURCE_BYTES = 4u * 1024u * 1024u };
        char *large = (char *)malloc(LARGE_SOURCE_BYTES);
        vitte_source_stats_t stats;
        size_t index;

        assert(large != NULL);
        for (index = 0u; index < LARGE_SOURCE_BYTES; ++index) {
            large[index] =
                (index % 4096u) == 4095u ? '\n' : 'x';
        }

        assert(vitte_source_init(&context));
        assert(vitte_source_add(
            &context,
            "large.vit",
            "large.vit",
            large,
            LARGE_SOURCE_BYTES,
            &id));
        assert(vitte_source_validate(&context));
        assert(vitte_source_fingerprint(&context) != UINT64_C(0));
        stats = vitte_source_stats(&context);
        assert(stats.sources_added == 1u);
        assert(stats.bytes_added == LARGE_SOURCE_BYTES);
        assert(stats.lines_indexed > 1000u);
        vitte_source_destroy(&context);
        free(large);
    }

    /* Failed registration must release partially built entries and leave a
       context that can be reset and reused for another compilation. */
    {
        static const unsigned char invalid_utf8[] = {0xc0u, 0xafu};
        static const char small[] = "ok\n";

        assert(vitte_source_init(&context));
        assert(vitte_source_set_limits(&context, 1u, 1u));
        assert(!vitte_source_add(
            &context,
            "too-large.vit",
            "too-large.vit",
            "ab",
            2u,
            &id));
        assert(vitte_source_last_error(&context) ==
               VITTE_SOURCE_ERROR_SOURCE_TOO_LARGE);
        assert(vitte_source_reset(&context));
        assert(vitte_source_set_limits(&context, 1u, 16u));
        assert(!vitte_source_add(
            &context,
            "invalid.vit",
            "invalid.vit",
            (const char *)invalid_utf8,
            sizeof(invalid_utf8),
            &id));
        assert(vitte_source_last_error(&context) ==
               VITTE_SOURCE_ERROR_INVALID_UTF8);
        assert(vitte_source_reset(&context));
        assert(context.source_count == 0u);
        assert(vitte_source_add(
            &context,
            "reused.vit",
            "reused.vit",
            small,
            strlen(small),
            &id));
        assert(vitte_source_validate(&context));
        vitte_source_destroy(&context);
    }

    /* Repeated independent generations catch stale ids, retained buffers and
       reset leaks that a single lifecycle would not expose. */
    assert(vitte_source_init(&context));
    for (size_t round = 0u; round < 256u; ++round) {
        char name[32];
        int written = snprintf(
            name,
            sizeof(name),
            "round-%zu.vit",
            round);

        assert(written > 0 && (size_t)written < sizeof(name));
        assert(vitte_source_add(
            &context,
            name,
            name,
            "return 0;\n",
            strlen("return 0;\n"),
            &id));
        assert(vitte_source_validate(&context));
        assert(vitte_source_reset(&context));
        assert(context.source_count == 0u);
    }
    vitte_source_destroy(&context);

    return 0;
}
