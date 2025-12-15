#include "../../include/debug/debugger.h"
#include <stdlib.h>

Debugger *debugger_create(VM *vm) {
    Debugger *dbg = malloc(sizeof(Debugger));
    dbg->vm = vm;
    dbg->breakpoints = NULL;
    dbg->bp_count = 0;
    dbg->paused = 0;
    return dbg;
}

void debugger_set_breakpoint(Debugger *dbg, int line) {
    // Set breakpoint
    dbg->bp_count++;
}

void debugger_step(Debugger *dbg) {
    // Single step execution
}

void debugger_destroy(Debugger *dbg) {
    free(dbg->breakpoints);
    free(dbg);
}
