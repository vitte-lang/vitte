#include <assert.h>
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
    return 0;
}
