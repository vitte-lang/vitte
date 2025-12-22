#include "cli.h"

int main(void) {
    cli_context_t *ctx = cli_init();
    if (!ctx) {
        return 1;
    }
    cli_free(ctx);
    return 0;
}
