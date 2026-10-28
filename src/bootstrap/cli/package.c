/* Package installation for the bootstrap CLI. */

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api/version.h"
#include "filesystem/filesystem.h"
#include "internal.h"
#include "platform/platform.h"

#define VITTE_PACKAGE_PATH_CAP 4096u
#define VITTE_PACKAGE_REGISTRY_URL "https://vitte-lang.org/registry/packages.tsv"
#define VITTE_PACKAGE_REGISTRY_LIMIT ((size_t)(4u * 1024u * 1024u))
#define VITTE_PACKAGE_ARCHIVE_LIMIT ((size_t)(512u * 1024u * 1024u))

typedef struct vitte_package_release {
    char name[128];
    char version[64];
    char download_url[2048];
    char sha256[65];
} vitte_package_release_t;

typedef struct vitte_sha256 {
    uint32_t state[8];
    uint64_t bit_count;
    unsigned char block[64];
    size_t used;
} vitte_sha256_t;

static const uint32_t vitte_sha256_constants[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static uint32_t
vitte_sha256_rotate_right(uint32_t value, unsigned int amount)
{
    return (value >> amount) | (value << (32u - amount));
}

static void
vitte_sha256_transform(vitte_sha256_t *context, const unsigned char block[64])
{
    uint32_t words[64];
    uint32_t a, b, c, d, e, f, g, h;
    size_t index;

    for (index = 0u; index < 16u; ++index) {
        size_t offset = index * 4u;
        words[index] = ((uint32_t)block[offset] << 24u) |
            ((uint32_t)block[offset + 1u] << 16u) |
            ((uint32_t)block[offset + 2u] << 8u) |
            (uint32_t)block[offset + 3u];
    }
    for (index = 16u; index < 64u; ++index) {
        uint32_t x = words[index - 15u];
        uint32_t y = words[index - 2u];
        uint32_t s0 = vitte_sha256_rotate_right(x, 7u) ^
            vitte_sha256_rotate_right(x, 18u) ^ (x >> 3u);
        uint32_t s1 = vitte_sha256_rotate_right(y, 17u) ^
            vitte_sha256_rotate_right(y, 19u) ^ (y >> 10u);
        words[index] = words[index - 16u] + s0 + words[index - 7u] + s1;
    }

    a = context->state[0]; b = context->state[1];
    c = context->state[2]; d = context->state[3];
    e = context->state[4]; f = context->state[5];
    g = context->state[6]; h = context->state[7];
    for (index = 0u; index < 64u; ++index) {
        uint32_t s1 = vitte_sha256_rotate_right(e, 6u) ^
            vitte_sha256_rotate_right(e, 11u) ^
            vitte_sha256_rotate_right(e, 25u);
        uint32_t choice = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + s1 + choice + vitte_sha256_constants[index] +
            words[index];
        uint32_t s0 = vitte_sha256_rotate_right(a, 2u) ^
            vitte_sha256_rotate_right(a, 13u) ^
            vitte_sha256_rotate_right(a, 22u);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = s0 + majority;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    context->state[0] += a; context->state[1] += b;
    context->state[2] += c; context->state[3] += d;
    context->state[4] += e; context->state[5] += f;
    context->state[6] += g; context->state[7] += h;
}

static void
vitte_sha256_update(vitte_sha256_t *context, const unsigned char *data,
                    size_t length)
{
    size_t offset = 0u;
    while (offset < length) {
        size_t available = sizeof(context->block) - context->used;
        size_t amount = length - offset < available ? length - offset : available;
        memcpy(context->block + context->used, data + offset, amount);
        context->used += amount;
        offset += amount;
        context->bit_count += (uint64_t)amount * 8u;
        if (context->used == sizeof(context->block)) {
            vitte_sha256_transform(context, context->block);
            context->used = 0u;
        }
    }
}

static void
vitte_sha256_digest(const unsigned char *data, size_t length, char output[65])
{
    static const char hex[] = "0123456789abcdef";
    vitte_sha256_t context = {{
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    }, 0u, {0u}, 0u};
    uint64_t original_bits;
    unsigned char marker = 0x80u;
    unsigned char zero = 0u;
    unsigned char length_bytes[8];
    size_t index;

    vitte_sha256_update(&context, data, length);
    original_bits = context.bit_count;
    vitte_sha256_update(&context, &marker, 1u);
    while (context.used != 56u) {
        vitte_sha256_update(&context, &zero, 1u);
    }
    for (index = 0u; index < 8u; ++index) {
        length_bytes[7u - index] = (unsigned char)(original_bits >> (index * 8u));
    }
    vitte_sha256_update(&context, length_bytes, sizeof(length_bytes));
    for (index = 0u; index < 8u; ++index) {
        size_t byte_index;
        for (byte_index = 0u; byte_index < 4u; ++byte_index) {
            unsigned char value = (unsigned char)(context.state[index] >>
                (24u - (unsigned int)byte_index * 8u));
            output[index * 8u + byte_index * 2u] = hex[value >> 4u];
            output[index * 8u + byte_index * 2u + 1u] = hex[value & 15u];
        }
    }
    output[64] = '\0';
}

static bool
vitte_package_safe_name(const char *name)
{
    const unsigned char *cursor = (const unsigned char *)name;
    if (name == NULL || *name == '\0') return false;
    for (; *cursor != '\0'; ++cursor) {
        if (!isalnum(*cursor) && *cursor != '-' && *cursor != '_') return false;
    }
    return true;
}

static bool
vitte_package_parse_stable_version(const char *text, uint32_t parts[3])
{
    size_t field;
    const char *cursor = text;
    if (text == NULL || *text == '\0') return false;
    for (field = 0u; field < 3u; ++field) {
        const char *field_start = cursor;
        uint64_t value = 0u;
        size_t digits = 0u;
        if (*cursor < '0' || *cursor > '9') return false;
        while (*cursor >= '0' && *cursor <= '9') {
            value = value * 10u + (uint64_t)(*cursor - '0');
            if (value > UINT32_MAX) return false;
            ++cursor;
            ++digits;
        }
        if (digits > 1u && field_start[0] == '0') return false;
        parts[field] = (uint32_t)value;
        if (field != 2u) {
            if (*cursor != '.') return false;
            ++cursor;
        }
    }
    return *cursor == '\0';
}

static int
vitte_package_compare_versions(const uint32_t left[3], const uint32_t right[3])
{
    size_t index;
    for (index = 0u; index < 3u; ++index) {
        if (left[index] < right[index]) return -1;
        if (left[index] > right[index]) return 1;
    }
    return 0;
}

static bool
vitte_package_compatible(const char *list, const char *compiler_version)
{
    const char *cursor = list;
    while (cursor != NULL && *cursor != '\0') {
        const char *end = strchr(cursor, ',');
        size_t length = end != NULL ? (size_t)(end - cursor) : strlen(cursor);
        if (strlen(compiler_version) == length &&
            memcmp(cursor, compiler_version, length) == 0) return true;
        if (end == NULL) break;
        cursor = end + 1;
    }
    return false;
}

static bool
vitte_package_copy_field(char *destination, size_t capacity, const char *value)
{
    size_t length;
    if (destination == NULL || capacity == 0u || value == NULL) return false;
    length = strlen(value);
    if (length == 0u || length >= capacity) return false;
    memcpy(destination, value, length + 1u);
    return true;
}

static bool
vitte_package_registry_resolve(const char *package_name, const char *requested,
                               const char *compiler_version,
                               const char *registry_path,
                               vitte_package_release_t *release)
{
    vitte_filesystem_buffer_t file = {0};
    vitte_filesystem_result_t fs_result;
    char *line;
    char *cursor;
    uint32_t best_version[3] = {0u, 0u, 0u};
    bool found = false;

    fs_result = vitte_filesystem_read_file(registry_path,
        VITTE_PACKAGE_REGISTRY_LIMIT, &file);
    if (fs_result.error != VITTE_FILESYSTEM_ERROR_NONE) return false;
    line = (char *)file.data;
    cursor = line;
    while (cursor != NULL && *cursor != '\0') {
        char *next = strchr(cursor, '\n');
        char *fields[5];
        size_t field;
        uint32_t parsed_version[3];
        if (next != NULL) *next = '\0';
        if (*cursor == '\0' || *cursor == '#') {
            cursor = next != NULL ? next + 1 : NULL;
            continue;
        }
        fields[0] = cursor;
        for (field = 1u; field < 5u; ++field) {
            char *separator = strchr(fields[field - 1u], '|');
            if (separator == NULL) break;
            *separator = '\0';
            fields[field] = separator + 1;
        }
        if (field == 5u && strchr(fields[4], '|') == NULL &&
            strcmp(fields[0], package_name) == 0 &&
            vitte_package_parse_stable_version(fields[1], parsed_version) &&
            vitte_package_compatible(fields[2], compiler_version) &&
            (requested == NULL || strcmp(fields[1], requested) == 0) &&
            strlen(fields[4]) == 64u) {
            size_t hex_index;
            bool valid_hash = true;
            for (hex_index = 0u; hex_index < 64u; ++hex_index) {
                if (!isxdigit((unsigned char)fields[4][hex_index])) valid_hash = false;
            }
            if (valid_hash && (!found || vitte_package_compare_versions(
                    parsed_version, best_version) > 0)) {
                if (!vitte_package_copy_field(release->name, sizeof(release->name), fields[0]) ||
                    !vitte_package_copy_field(release->version, sizeof(release->version), fields[1]) ||
                    !vitte_package_copy_field(release->download_url,
                                              sizeof(release->download_url), fields[3]) ||
                    !vitte_package_copy_field(release->sha256,
                                              sizeof(release->sha256), fields[4]) ||
                    strncmp(release->download_url, "https://", 8u) != 0) {
                    found = false;
                } else {
                    memcpy(best_version, parsed_version, sizeof(best_version));
                    found = true;
                }
            }
        }
        cursor = next != NULL ? next + 1 : NULL;
    }
    vitte_filesystem_buffer_destroy(&file);
    return found;
}

static bool
vitte_package_remove_tree(const char *path)
{
    vitte_filesystem_metadata_t metadata;
    vitte_filesystem_result_t result = vitte_filesystem_stat(path, false, &metadata);
    if (result.error != VITTE_FILESYSTEM_ERROR_NONE) return true;
    if (metadata.kind != VITTE_FILESYSTEM_KIND_DIRECTORY) {
        return vitte_filesystem_remove_file(path).error == VITTE_FILESYSTEM_ERROR_NONE;
    }
    {
        vitte_filesystem_directory_list_t listing = {0};
        size_t index;
        result = vitte_filesystem_list_directory(path, 0u, &listing);
        if (result.error != VITTE_FILESYSTEM_ERROR_NONE) return false;
        for (index = 0u; index < listing.count; ++index) {
            char child[VITTE_PACKAGE_PATH_CAP];
            int written = snprintf(child, sizeof(child), "%s/%s", path,
                                   listing.entries[index].name);
            if (written < 0 || (size_t)written >= sizeof(child) ||
                !vitte_package_remove_tree(child)) {
                vitte_filesystem_directory_list_destroy(&listing);
                return false;
            }
        }
        vitte_filesystem_directory_list_destroy(&listing);
    }
    return vitte_filesystem_remove_directory(path).error == VITTE_FILESYSTEM_ERROR_NONE;
}

static bool
vitte_package_join(char *buffer, size_t capacity, const char *left, const char *right)
{
    int written = snprintf(buffer, capacity, "%s/%s", left, right);
    return written >= 0 && (size_t)written < capacity;
}

static bool
vitte_package_user_root(char *buffer, size_t capacity)
{
    const char *base;
    int written;
#if defined(_WIN32)
    base = getenv("LOCALAPPDATA");
    if (base == NULL || *base == '\0') return false;
    written = snprintf(buffer, capacity, "%s/Vitte/modules", base);
#else
    base = getenv("XDG_DATA_HOME");
    if (base != NULL && *base != '\0') {
        written = snprintf(buffer, capacity, "%s/vitte/modules", base);
    } else {
        base = getenv("HOME");
        if (base == NULL || *base == '\0') return false;
        written = snprintf(buffer, capacity, "%s/.local/share/vitte/modules", base);
    }
#endif
    return written >= 0 && (size_t)written < capacity;
}

static bool
vitte_package_update_file(const char *path, const char *prefix,
                          const char *replacement, bool *existed,
                          vitte_filesystem_buffer_t *original)
{
    vitte_filesystem_buffer_t current = {0};
    vitte_filesystem_result_t result;
    char *output;
    size_t output_length = 0u;
    size_t capacity;
    char *cursor;
    size_t prefix_length = strlen(prefix);
    bool had_prefix = false;

    *existed = vitte_filesystem_exists(path);
    if (*existed) {
        result = vitte_filesystem_read_file(path, 1024u * 1024u, &current);
        if (result.error != VITTE_FILESYSTEM_ERROR_NONE) return false;
        original->data = current.data;
        original->length = current.length;
        original->capacity = current.capacity;
        memset(&current, 0, sizeof(current));
    }
    capacity = (original->length + strlen(replacement) + 2u);
    output = (char *)malloc(capacity);
    if (output == NULL) return false;
    cursor = (char *)original->data;
    while (cursor != NULL && *cursor != '\0') {
        char *end = strchr(cursor, '\n');
        size_t line_length = end != NULL ? (size_t)(end - cursor) : strlen(cursor);
        if (line_length >= prefix_length && memcmp(cursor, prefix, prefix_length) == 0) {
            had_prefix = true;
        } else {
            if (output_length + line_length + 2u > capacity) {
                free(output);
                return false;
            }
            memcpy(output + output_length, cursor, line_length);
            output_length += line_length;
            output[output_length++] = '\n';
        }
        cursor = end != NULL ? end + 1 : NULL;
    }
    if (output_length != 0u && output[output_length - 1u] != '\n') {
        output[output_length++] = '\n';
    }
    (void)had_prefix;
    {
        size_t replacement_length = strlen(replacement);
        if (output_length + replacement_length + 2u > capacity) {
            free(output);
            return false;
        }
        memcpy(output + output_length, replacement, replacement_length);
        output_length += replacement_length;
        output[output_length++] = '\n';
        output[output_length] = '\0';
    }
    result = vitte_filesystem_write_file_atomic(path, output, output_length);
    free(output);
    return result.error == VITTE_FILESYSTEM_ERROR_NONE;
}

static bool
vitte_package_rollback_file(const char *path, bool existed,
                            const vitte_filesystem_buffer_t *original)
{
    if (existed) {
        return vitte_filesystem_write_file_atomic(path, original->data,
                                                  original->length).error ==
            VITTE_FILESYSTEM_ERROR_NONE;
    }
    if (vitte_filesystem_exists(path)) {
        return vitte_filesystem_remove_file(path).error ==
            VITTE_FILESYSTEM_ERROR_NONE;
    }
    return true;
}

static bool
vitte_package_write_project_records(const char *project_root,
                                    const vitte_package_release_t *release)
{
    char manifest_path[VITTE_PACKAGE_PATH_CAP];
    char lock_path[VITTE_PACKAGE_PATH_CAP];
    char dependency_prefix[256];
    char dependency_record[512];
    char lock_prefix[256];
    char lock_record[3072];
    vitte_filesystem_buffer_t manifest_original = {0};
    vitte_filesystem_buffer_t lock_original = {0};
    bool manifest_existed = false;
    bool lock_existed = false;
    bool manifest_written;
    int written;

    if (!vitte_package_join(manifest_path, sizeof(manifest_path),
                            project_root, "vitte.packages.toml") ||
        !vitte_package_join(lock_path, sizeof(lock_path), project_root, "vitte.lock")) {
        return false;
    }
    written = snprintf(dependency_prefix, sizeof(dependency_prefix),
                       "dependency.%s =", release->name);
    if (written < 0 || (size_t)written >= sizeof(dependency_prefix)) return false;
    written = snprintf(dependency_record, sizeof(dependency_record),
                       "%s \"%s\"", dependency_prefix, release->version);
    if (written < 0 || (size_t)written >= sizeof(dependency_record)) return false;
    written = snprintf(lock_prefix, sizeof(lock_prefix), "package.%s.", release->name);
    if (written < 0 || (size_t)written >= sizeof(lock_prefix)) return false;
    written = snprintf(lock_record, sizeof(lock_record),
        "%sversion = \"%s\"\n%ssource = \"%s\"\n%schecksum = \"sha256:%s\"",
        lock_prefix, release->version, lock_prefix, release->download_url,
        lock_prefix, release->sha256);
    if (written < 0 || (size_t)written >= sizeof(lock_record)) return false;

    if (!vitte_package_update_file(manifest_path, dependency_prefix,
            dependency_record, &manifest_existed, &manifest_original)) goto cleanup;
    manifest_written = true;
    if (!vitte_package_update_file(lock_path, lock_prefix, lock_record,
            &lock_existed, &lock_original)) {
        if (manifest_written) {
            (void)vitte_package_rollback_file(manifest_path, manifest_existed,
                                               &manifest_original);
        }
        goto cleanup;
    }
    vitte_filesystem_buffer_destroy(&manifest_original);
    vitte_filesystem_buffer_destroy(&lock_original);
    return true;

cleanup:
    vitte_filesystem_buffer_destroy(&manifest_original);
    vitte_filesystem_buffer_destroy(&lock_original);
    return false;
}

static bool
vitte_package_archive_has_metadata(const char *stage,
                                   const vitte_package_release_t *release)
{
    char metadata_path[VITTE_PACKAGE_PATH_CAP];
    vitte_filesystem_buffer_t file = {0};
    char expected_name[160];
    char expected_version[96];
    bool success = false;
    int written;
    if (!vitte_package_join(metadata_path, sizeof(metadata_path), stage,
                            "package.toml") ||
        vitte_filesystem_read_file(metadata_path, 65536u, &file).error !=
            VITTE_FILESYSTEM_ERROR_NONE) return false;
    written = snprintf(expected_name, sizeof(expected_name), "name = \"%s\"",
                       release->name);
    if (written < 0 || (size_t)written >= sizeof(expected_name)) goto done;
    written = snprintf(expected_version, sizeof(expected_version),
                       "version = \"%s\"", release->version);
    if (written < 0 || (size_t)written >= sizeof(expected_version)) goto done;
    success = strstr((char *)file.data, expected_name) != NULL &&
        strstr((char *)file.data, expected_version) != NULL;
done:
    vitte_filesystem_buffer_destroy(&file);
    return success;
}

vitte_main_exit_code_t
vitte_main_install_package(const char *name, const char *requested_version,
                           bool global_install)
{
    char compiler_version[64] = {0};
    char cwd[VITTE_PACKAGE_PATH_CAP] = {0};
    char modules_root[VITTE_PACKAGE_PATH_CAP] = {0};
    char archive_path[VITTE_PACKAGE_PATH_CAP] = {0};
    char stage_path[VITTE_PACKAGE_PATH_CAP] = {0};
    char target_path[VITTE_PACKAGE_PATH_CAP] = {0};
    char backup_path[VITTE_PACKAGE_PATH_CAP] = {0};
    char registry_path[VITTE_PACKAGE_PATH_CAP] = {0};
    vitte_package_release_t release;
    vitte_filesystem_buffer_t archive = {0};
    char digest[65] = {0};
    char expected_digest[65] = {0};
    char version_directory[192] = {0};
    const char *curl_arguments[9];
    const char *extract_command[8];
    char archive_name[256] = {0};
    char stage_name[256] = {0};
    char backup_name[256] = {0};
    bool old_target_moved = false;
    bool new_target_moved = false;
    int written;

    if (!vitte_package_safe_name(name)) {
        (void)fprintf(stderr, "vitte install: invalid package name '%s'\n",
                      name != NULL ? name : "");
        return VITTE_MAIN_EXIT_USAGE;
    }
    if (requested_version != NULL) {
        uint32_t parsed[3];
        if (!vitte_package_parse_stable_version(requested_version, parsed)) {
            (void)fprintf(stderr, "vitte install: version must be a stable x.y.z version\n");
            return VITTE_MAIN_EXIT_USAGE;
        }
    }
    written = snprintf(compiler_version, sizeof(compiler_version), "%u.%u.%u",
        (unsigned int)VITTE_VERSION_MAJOR, (unsigned int)VITTE_VERSION_MINOR,
        (unsigned int)VITTE_VERSION_PATCH);
    if (written < 0 || (size_t)written >= sizeof(compiler_version) ||
        !vitte_platform_getcwd(cwd, sizeof(cwd))) {
        (void)fprintf(stderr, "vitte install: cannot determine the project directory\n");
        return VITTE_MAIN_EXIT_FAILURE;
    }
    if (global_install) {
        if (!vitte_package_user_root(modules_root, sizeof(modules_root))) {
            (void)fprintf(stderr, "vitte install: cannot determine the user module directory\n");
            return VITTE_MAIN_EXIT_FAILURE;
        }
    } else if (!vitte_package_join(modules_root, sizeof(modules_root), cwd,
                                   ".vitte/modules")) {
        return VITTE_MAIN_EXIT_FAILURE;
    }
    if (vitte_filesystem_create_directories(modules_root).error !=
        VITTE_FILESYSTEM_ERROR_NONE) {
        (void)fprintf(stderr, "vitte install: cannot create module directory '%s'\n",
                      modules_root);
        return VITTE_MAIN_EXIT_FAILURE;
    }
    if (!vitte_package_join(registry_path, sizeof(registry_path), modules_root,
                            ".packages.tsv")) {
        return VITTE_MAIN_EXIT_FAILURE;
    }

    curl_arguments[0] = "curl"; curl_arguments[1] = "--fail";
    curl_arguments[2] = "--location"; curl_arguments[3] = "--silent";
    curl_arguments[4] = "--show-error"; curl_arguments[5] = "--output";
    curl_arguments[6] = registry_path;
    curl_arguments[7] = VITTE_PACKAGE_REGISTRY_URL;
    curl_arguments[8] = NULL;
    if (!vitte_platform_run(curl_arguments, true)) {
        (void)fprintf(stderr, "vitte install: could not download the package registry\n");
        goto failure;
    }
    memset(&release, 0, sizeof(release));
    if (!vitte_package_registry_resolve(name, requested_version,
            compiler_version, registry_path, &release)) {
        (void)fprintf(stderr,
            "vitte install: no stable compatible release found for '%s'%s%s\n",
            name, requested_version != NULL ? " version " : "",
            requested_version != NULL ? requested_version : "");
        goto failure;
    }
    written = snprintf(version_directory, sizeof(version_directory), "%s-%s",
                       release.name, release.version);
    if (written < 0 || (size_t)written >= sizeof(version_directory)) goto failure;
    written = snprintf(archive_name, sizeof(archive_name), ".%s.tar.gz", version_directory);
    if (written < 0 || (size_t)written >= sizeof(archive_name)) goto failure;
    written = snprintf(stage_name, sizeof(stage_name), ".%s-staging", release.name);
    if (written < 0 || (size_t)written >= sizeof(stage_name)) goto failure;
    written = snprintf(backup_name, sizeof(backup_name), ".%s-backup", release.name);
    if (written < 0 || (size_t)written >= sizeof(backup_name)) goto failure;
    if (!vitte_package_join(archive_path, sizeof(archive_path), modules_root,
                            archive_name) ||
        !vitte_package_join(stage_path, sizeof(stage_path), modules_root,
                            stage_name) ||
        !vitte_package_join(target_path, sizeof(target_path), modules_root,
                            release.name) ||
        !vitte_package_join(backup_path, sizeof(backup_path), modules_root,
                            backup_name)) goto failure;
    curl_arguments[6] = archive_path;
    curl_arguments[7] = release.download_url;
    if (!vitte_platform_run(curl_arguments, true)) {
        (void)fprintf(stderr, "vitte install: download failed for %s %s\n",
                      release.name, release.version);
        goto failure;
    }
    if (vitte_filesystem_read_file(archive_path, VITTE_PACKAGE_ARCHIVE_LIMIT,
                                   &archive).error != VITTE_FILESYSTEM_ERROR_NONE) {
        (void)fprintf(stderr, "vitte install: downloaded archive cannot be read\n");
        goto failure;
    }
    vitte_sha256_digest(archive.data, archive.length, digest);
    vitte_filesystem_buffer_destroy(&archive);
    (void)snprintf(expected_digest, sizeof(expected_digest), "%s", release.sha256);
    for (written = 0; written < 64; ++written) {
        if (expected_digest[written] >= 'A' && expected_digest[written] <= 'F') {
            expected_digest[written] = (char)(expected_digest[written] - 'A' + 'a');
        }
    }
    if (strcmp(digest, expected_digest) != 0) {
        (void)fprintf(stderr, "vitte install: SHA-256 mismatch for %s %s\n",
                      release.name, release.version);
        goto failure;
    }

    (void)vitte_package_remove_tree(stage_path);
    if (vitte_filesystem_create_directories(stage_path).error !=
        VITTE_FILESYSTEM_ERROR_NONE) goto failure;
    extract_command[0] = "tar";
    extract_command[1] = "-xzf";
    extract_command[2] = archive_path;
    extract_command[3] = "-C";
    extract_command[4] = stage_path;
    extract_command[5] = "--strip-components";
    extract_command[6] = "1";
    extract_command[7] = NULL;
    if (!vitte_platform_run(extract_command, true) ||
        !vitte_package_archive_has_metadata(stage_path, &release)) {
        (void)fprintf(stderr, "vitte install: archive layout or package metadata is invalid\n");
        goto failure;
    }

    (void)vitte_package_remove_tree(backup_path);
    if (vitte_filesystem_exists(target_path)) {
        if (vitte_filesystem_rename(target_path, backup_path).error !=
            VITTE_FILESYSTEM_ERROR_NONE) goto failure;
        old_target_moved = true;
    }
    if (vitte_filesystem_rename(stage_path, target_path).error !=
        VITTE_FILESYSTEM_ERROR_NONE) {
        if (old_target_moved) (void)vitte_filesystem_rename(backup_path, target_path);
        goto failure;
    }
    new_target_moved = true;
    if (!global_install && !vitte_package_write_project_records(cwd, &release)) {
        (void)fprintf(stderr, "vitte install: could not update project manifest and lock file\n");
        (void)vitte_package_remove_tree(target_path);
        if (old_target_moved) (void)vitte_filesystem_rename(backup_path, target_path);
        new_target_moved = false;
        goto failure;
    }
    if (old_target_moved) (void)vitte_package_remove_tree(backup_path);
    (void)vitte_filesystem_remove_file(archive_path);
    (void)vitte_filesystem_remove_file(registry_path);
    (void)printf("Installed %s %s %s\n", release.name, release.version,
                 global_install ? "for the current user" : "in this project");
    return VITTE_MAIN_EXIT_SUCCESS;

failure:
    if (archive.data != NULL) vitte_filesystem_buffer_destroy(&archive);
    if (!new_target_moved && stage_path[0] != '\0') {
        (void)vitte_package_remove_tree(stage_path);
    }
    if (archive_path[0] != '\0') (void)vitte_filesystem_remove_file(archive_path);
    if (registry_path[0] != '\0') (void)vitte_filesystem_remove_file(registry_path);
    return VITTE_MAIN_EXIT_FAILURE;
}
