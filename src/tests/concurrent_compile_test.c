#include "cli/internal.h"
#include "api/api.h"

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>

typedef struct compile_worker {
    const char *path;
    vitte_main_exit_code_t result;
    size_t completed;
} compile_worker_t;

static void
run_api_compilation(
    vitte_api_t *api)
{
    static const char source[] =
        "proc main() -> int { return 0; }\n";
    vitte_api_result_t *result = NULL;
    vitte_status_t status;
    const char *diagnostics;

    status = vitte_api_compile_string(
        api,
        source,
        &result);
    assert(status == VITTE_STATUS_OK);
    assert(result != NULL);
    assert(vitte_api_result_status(result) == status);
    diagnostics = vitte_api_result_diagnostics(result, NULL);
    assert(diagnostics != NULL);
    vitte_api_result_destroy(result);
}

static void *
run_compile(void *argument)
{
    compile_worker_t *worker = (compile_worker_t *)argument;
    vitte_main_options_t options;
    vitte_api_t *api = NULL;

    memset(&options, 0, sizeof(options));
    options.command = VITTE_MAIN_COMMAND_CHECK;
    options.input_path = worker->path;
    options.diagnostic_format = VITTE_MAIN_DIAGNOSTIC_TERMINAL;
    options.quiet = true;
    options.color = false;
    assert(vitte_api_create(&api) == VITTE_STATUS_OK);
    worker->result = vitte_main_driver_compile(&options);
    run_api_compilation(api);
    worker->completed++;

    for (size_t round = 1u; round < 16u; ++round) {
        worker->result = vitte_main_driver_compile(&options);
        run_api_compilation(api);
        worker->completed++;
    }

    vitte_api_destroy(api);

    return NULL;
}

int
main(int argc, char **argv)
{
    enum { WORKER_COUNT = 8 };
    pthread_t threads[WORKER_COUNT];
    compile_worker_t workers[WORKER_COUNT];
    size_t index;

    assert(argc == 3);
    for (index = 0u; index < WORKER_COUNT; ++index) {
        workers[index].path = argv[index % 2u + 1u];
        workers[index].result = VITTE_MAIN_EXIT_INTERNAL;
        workers[index].completed = 0u;
        assert(pthread_create(
            &threads[index],
            NULL,
            run_compile,
            &workers[index]) == 0);
    }

    for (index = 0u; index < WORKER_COUNT; ++index) {
        assert(pthread_join(threads[index], NULL) == 0);
        assert(workers[index].result == VITTE_MAIN_EXIT_SUCCESS);
        assert(workers[index].completed == 16u);
    }

    return 0;
}
