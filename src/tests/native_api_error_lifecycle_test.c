#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
assert_error_ok(compiler_api_error_Error *error)
{
    assert(error != NULL);
    assert(error->status == compiler_api_error_Status_Ok);
    assert(error->code != NULL);
    assert(strcmp(error->code, "VITTE_OK") == 0);
    assert(error->message != NULL);
    assert(strcmp(error->message, "ok") == 0);
    assert(error->details == NULL);
    assert(compiler_api_error_error_validate(error));
}

static void
assert_error_destroyed(compiler_api_error_Error *error)
{
    assert(error != NULL);
    assert(error->status == compiler_api_error_Status_Ok);
    assert(error->code != NULL && error->code[0] == '\0');
    assert(error->message != NULL && error->message[0] == '\0');
    assert(error->details == NULL);
}

int
main(void)
{
    compiler_api_error_Error initialized = {
        .status = compiler_api_error_Status_Ok,
        .code = "",
        .message = "",
        .details = NULL
    };
    size_t iteration;

    compiler_api_error_error_init(&initialized);
    assert_error_ok(&initialized);
    compiler_api_error_error_destroy(&initialized);
    assert_error_destroyed(&initialized);

    compiler_api_error_error_clear_last();
    assert(!compiler_api_error_error_last_is_set());

    for (iteration = 0u; iteration < 4096u; ++iteration) {
        compiler_api_error_Error current =
            compiler_api_error_error_ok();
        compiler_api_error_Error copied =
            compiler_api_error_error_ok();
        compiler_api_error_Error scratch =
            compiler_api_error_error_ok();
        compiler_api_error_Error *last;

        assert_error_ok(&current);
        assert_error_ok(&copied);
        assert_error_ok(&scratch);

        compiler_api_error_error_set_details(
            &current,
            compiler_api_error_Status_ErrorIo,
            "VITTE_TEST_IO",
            "transient I/O failure",
            "path=/tmp/input.vit");
        assert(current.status == compiler_api_error_Status_ErrorIo);
        assert(strcmp(current.code, "VITTE_TEST_IO") == 0);
        assert(strcmp(current.message, "transient I/O failure") == 0);
        assert(strcmp(current.details, "path=/tmp/input.vit") == 0);
        assert(compiler_api_error_error_validate(&current));

        last = compiler_api_error_error_last();
        assert(last != &current);
        assert(last->status == current.status);
        assert(strcmp(last->code, current.code) == 0);
        assert(strcmp(last->details, current.details) == 0);

        compiler_api_error_error_copy(&copied, &current);
        assert(copied.status == current.status);
        assert(strcmp(copied.code, current.code) == 0);
        assert(strcmp(copied.message, current.message) == 0);
        assert(strcmp(copied.details, current.details) == 0);

        /* Exercise self-copy through the C ABI, bypassing Vitte borrow checks. */
        compiler_api_error_error_copy(&copied, &copied);
        assert(strcmp(copied.code, "VITTE_TEST_IO") == 0);
        assert(strcmp(copied.details, "path=/tmp/input.vit") == 0);

        /* Replacement frees every old owned field before storing defaults. */
        compiler_api_error_error_set(
            &current,
            compiler_api_error_Status_ErrorInternal,
            NULL,
            NULL);
        assert(current.status == compiler_api_error_Status_ErrorInternal);
        assert(strcmp(current.code, "VITTE_ERROR_INTERNAL") == 0);
        assert(strcmp(current.message, "internal compiler error") == 0);
        assert(current.details == NULL);
        assert(compiler_api_error_error_validate(&current));

        compiler_api_error_error_copy(&scratch, &current);
        compiler_api_error_error_copy(&scratch, NULL);
        assert_error_ok(&scratch);

        compiler_api_error_error_reset(&copied);
        assert_error_ok(&copied);

        compiler_api_error_error_clear_last();
        assert_error_ok(compiler_api_error_error_last());
        assert(!compiler_api_error_error_last_is_set());

        compiler_api_error_error_destroy(&current);
        compiler_api_error_error_destroy(&copied);
        compiler_api_error_error_destroy(&scratch);
        assert_error_destroyed(&current);
        assert_error_destroyed(&copied);
        assert_error_destroyed(&scratch);
    }

    compiler_api_error_error_clear_last();
    (void)puts("Native API error lifecycle unit tests passed.");
    return 0;
}
