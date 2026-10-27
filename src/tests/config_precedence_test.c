#include "config/config.h"
#include "filesystem/filesystem.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct environment_entry {
    const char *name;
    const char *value;
} environment_entry_t;

static const char *
test_environment_get(
    void *user_data,
    const char *name)
{
    const environment_entry_t *entries =
        (const environment_entry_t *)user_data;
    size_t index;

    for (index = 0u; entries[index].name != NULL; ++index) {
        if (strcmp(entries[index].name, name) == 0) {
            return entries[index].value;
        }
    }
    return NULL;
}

static void
write_config_file(const char *path)
{
    const char text[] =
        "jobs = 2 # project default\n"
        "diagnostic-format = \"json\"\n";
    assert(vitte_filesystem_write_file(
        path,
        text,
        sizeof(text) - 1u).error == VITTE_FILESYSTEM_ERROR_NONE);
}

int
main(void)
{
    char *directory = NULL;
    char *path = NULL;
    char *conflict = NULL;
    char *empty = NULL;
    char *section = NULL;
    const environment_entry_t environment[] = {
        {"VITTE_JOBS", "3"},
        {"VITTE_DIAGNOSTIC_FORMAT", "sarif"},
        {NULL, NULL}
    };
    const char *cli[] = {"--jobs=4", "--diagnostic-format", "lsp"};
    vitte_config_t config;
    const vitte_config_entry_t *jobs;
    const vitte_config_entry_t *diagnostic;
    const vitte_config_message_t *message;
    vitte_config_stats_t stats;

    assert(vitte_filesystem_current_directory(&directory).error ==
           VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        directory,
        ".vitte-config-precedence.toml",
        &path).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        directory,
        ".vitte-config-conflict.toml",
        &conflict).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        directory,
        ".vitte-config-empty.toml",
        &empty).error == VITTE_FILESYSTEM_ERROR_NONE);
    assert(vitte_filesystem_join(
        directory,
        ".vitte-config-section.toml",
        &section).error == VITTE_FILESYSTEM_ERROR_NONE);
    write_config_file(path);
    assert(vitte_config_init(&config));
    assert(vitte_config_apply_defaults(&config));
    assert(vitte_config_load_file(
        &config,
        path,
        VITTE_CONFIG_SOURCE_PROJECT));
    assert(vitte_config_apply_environment_with(
        &config,
        "VITTE_",
        test_environment_get,
        (void *)environment));
    diagnostic = vitte_config_find_entry(
        &config,
        VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT);
    assert(diagnostic != NULL);
    assert(diagnostic->origin.source == VITTE_CONFIG_SOURCE_ENVIRONMENT);
    assert(strcmp(diagnostic->origin.name,
                  "VITTE_DIAGNOSTIC_FORMAT") == 0);
    assert(vitte_config_apply_cli(
        &config,
        (int)(sizeof(cli) / sizeof(cli[0])),
        cli));
    assert(vitte_config_validate(&config));

    assert(vitte_config_resolved(&config)->jobs == 4u);
    assert(vitte_config_resolved(&config)->diagnostic_format ==
           VITTE_CONFIG_DIAGNOSTIC_LSP);
    jobs = vitte_config_find_entry(&config, VITTE_CONFIG_KEY_JOBS);
    assert(jobs != NULL);
    assert(jobs->origin.source == VITTE_CONFIG_SOURCE_COMMAND_LINE);
    assert(strcmp(jobs->origin.name, "--jobs=4") == 0);
    stats = vitte_config_stats(&config);
    assert(stats.validation_count == 1u);

    assert(vitte_config_reset(&config));
    assert(vitte_config_apply_defaults(&config));
    assert(vitte_config_load_file(
        &config,
        path,
        VITTE_CONFIG_SOURCE_PROJECT));
    jobs = vitte_config_find_entry(&config, VITTE_CONFIG_KEY_JOBS);
    assert(jobs != NULL);
    assert(jobs->origin.line == 1u);
    assert(jobs->origin.column == 8u);
    assert(strcmp(jobs->origin.name, path) == 0);

    {
        const char text[] = "jobs = 5\n";
        assert(vitte_filesystem_write_file(
            conflict,
            text,
            sizeof(text) - 1u).error == VITTE_FILESYSTEM_ERROR_NONE);
        assert(!vitte_config_load_file(
            &config,
            conflict,
            VITTE_CONFIG_SOURCE_PROJECT));
        message = vitte_config_message(&config);
        assert(message != NULL);
        assert(message->error == VITTE_CONFIG_ERROR_CONFLICT);
        assert(message->key == VITTE_CONFIG_KEY_JOBS);
        assert(message->origin.line == 1u);
        assert(message->origin.column == 8u);
        assert(strcmp(message->origin.name, conflict) == 0);
    }

    assert(vitte_config_reset(&config));
    assert(vitte_config_apply_defaults(&config));
    {
        const char text[] = "jobs =\n";
        assert(vitte_filesystem_write_file(
            empty,
            text,
            sizeof(text) - 1u).error == VITTE_FILESYSTEM_ERROR_NONE);
        assert(!vitte_config_load_file(
            &config,
            empty,
            VITTE_CONFIG_SOURCE_PROJECT));
        message = vitte_config_message(&config);
        assert(message != NULL);
        assert(message->error == VITTE_CONFIG_ERROR_INVALID_VALUE);
        assert(message->key == VITTE_CONFIG_KEY_JOBS);
        assert(message->origin.line == 1u);
        assert(message->origin.column == 7u);
    }

    assert(vitte_config_reset(&config));
    assert(vitte_config_apply_defaults(&config));
    {
        const char *invalid_cli[] = {"--does-not-exist=1"};
        assert(!vitte_config_apply_cli(
            &config,
            1,
            invalid_cli));
        message = vitte_config_message(&config);
        assert(message != NULL);
        assert(message->error == VITTE_CONFIG_ERROR_UNKNOWN_KEY);
        assert(message->origin.line == 0u);
        assert(message->origin.column == 1u);
        assert(strcmp(message->origin.name, "--does-not-exist=1") == 0);
    }

    assert(vitte_config_reset(&config));
    assert(vitte_config_apply_defaults(&config));
    {
        char argument[] = "--jobs";
        const char *missing_value[] = {argument};
        assert(!vitte_config_apply_cli(
            &config,
            1,
            missing_value));
        argument[2] = 'X';
        message = vitte_config_message(&config);
        assert(message != NULL);
        assert(message->error == VITTE_CONFIG_ERROR_INVALID_VALUE);
        assert(message->origin.column == 1u);
        assert(strcmp(message->origin.name, "--jobs") == 0);
    }

    assert(vitte_config_reset(&config));
    assert(vitte_config_apply_defaults(&config));
    {
        const char text[] = "[build]\njobs = 2\n";
        assert(vitte_filesystem_write_file(
            section,
            text,
            sizeof(text) - 1u).error == VITTE_FILESYSTEM_ERROR_NONE);
        assert(!vitte_config_load_file(
            &config,
            section,
            VITTE_CONFIG_SOURCE_PROJECT));
        message = vitte_config_message(&config);
        assert(message != NULL);
        assert(message->error == VITTE_CONFIG_ERROR_INVALID_CONFIG);
        assert(message->origin.line == 1u);
        assert(message->origin.column == 1u);
    }

    {
        vitte_config_t source;
        vitte_config_t merged;
        vitte_config_t clone;
        const vitte_config_entry_t *merged_jobs;
        const vitte_config_entry_t *clone_jobs;

        assert(vitte_config_init(&source));
        assert(vitte_config_apply_defaults(&source));
        assert(vitte_config_load_file(
            &source,
            path,
            VITTE_CONFIG_SOURCE_PROJECT));
        assert(vitte_config_init(&merged));
        assert(vitte_config_apply_defaults(&merged));
        assert(vitte_config_merge(&merged, &source));
        vitte_config_destroy(&source);
        merged_jobs = vitte_config_find_entry(
            &merged,
            VITTE_CONFIG_KEY_JOBS);
        assert(merged_jobs != NULL);
        assert(merged_jobs->value.as.size == 2u);
        assert(strcmp(merged_jobs->origin.name, path) == 0);

        assert(vitte_config_clone(&clone, &merged));
        vitte_config_destroy(&merged);
        clone_jobs = vitte_config_find_entry(
            &clone,
            VITTE_CONFIG_KEY_JOBS);
        assert(clone_jobs != NULL);
        assert(clone_jobs->value.as.size == 2u);
        assert(strcmp(clone_jobs->origin.name, path) == 0);
        vitte_config_destroy(&clone);
    }

    vitte_config_destroy(&config);
    (void)vitte_filesystem_remove_file(empty);
    (void)vitte_filesystem_remove_file(section);
    (void)vitte_filesystem_remove_file(conflict);
    (void)vitte_filesystem_remove_file(path);
    free(empty);
    free(section);
    free(conflict);
    free(path);
    free(directory);
    return 0;
}
