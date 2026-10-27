#include "arena/arena.h"
#include "filesystem/filesystem.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void
test_arena_limits(void)
{
    vitte_arena_config_t settings = vitte_arena_config_default();
    vitte_arena_context_t arena;

    settings.block_size = (size_t)1024u;
    settings.memory_limit = (size_t)64u;
    settings.string_limit = (size_t)8u;
    assert(vitte_arena_context_init(&arena, &settings));
    assert(vitte_arena_context_alloc(&arena, (size_t)32u) != NULL);
    assert(vitte_arena_context_alloc(&arena, (size_t)64u) == NULL);
    assert(vitte_arena_context_last_error(&arena) ==
           VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED);
    vitte_arena_context_clear_error(&arena);
    assert(vitte_arena_context_alloc_array(
               &arena, SIZE_MAX, (size_t)2u, 0u) == NULL);
    assert(vitte_arena_context_last_error(&arena) ==
           VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);
    assert(vitte_arena_context_is_valid(&arena));
    vitte_arena_context_destroy(&arena);
}

static void
test_large_file_and_path_limits(void)
{
    char *directory = NULL;
    char *path = NULL;
    char *oversized_path;
    char *normalized = NULL;
    unsigned char *payload;
    vitte_filesystem_buffer_t buffer = {NULL, 0u, 0u};
    const size_t payload_size = (size_t)4u * (size_t)1024u * (size_t)1024u;
    size_t index;

    assert(vitte_filesystem_current_directory(&directory).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
               directory, ".vitte-robustness-large.bin", &path).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    payload = (unsigned char *)malloc(payload_size);
    assert(payload != NULL);
    for (index = 0u; index < payload_size; ++index) {
        payload[index] = (unsigned char)(index * (size_t)31u);
    }
    assert(vitte_filesystem_write_file(path, payload, payload_size).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_read_file(path, (size_t)1024u, &buffer).error ==
           VITTE_FILESYSTEM_ERROR_TOO_LARGE);
    assert(buffer.data == NULL && buffer.length == 0u);
    assert(vitte_filesystem_read_file(
               path, payload_size + (size_t)1u, &buffer).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(buffer.length == payload_size);
    assert(memcmp(buffer.data, payload, payload_size) == 0);
    vitte_filesystem_buffer_destroy(&buffer);

    oversized_path = (char *)malloc(
        VITTE_FILESYSTEM_DEFAULT_MAX_PATH_BYTES + (size_t)2u);
    assert(oversized_path != NULL);
    memset(
        oversized_path,
        'x',
        VITTE_FILESYSTEM_DEFAULT_MAX_PATH_BYTES + (size_t)1u);
    oversized_path[VITTE_FILESYSTEM_DEFAULT_MAX_PATH_BYTES + (size_t)1u] =
        '\0';
    assert(vitte_filesystem_normalize(oversized_path, &normalized).error ==
           VITTE_FILESYSTEM_ERROR_PATH_TOO_LONG);
    free(normalized);
    free(oversized_path);

    assert(vitte_filesystem_remove_file(path).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    free(payload);
    free(path);
    free(directory);
}

int
main(void)
{
    test_arena_limits();
    test_large_file_and_path_limits();
    return 0;
}
