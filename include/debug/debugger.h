#ifndef STEEL_DEBUGGER_H
#define STEEL_DEBUGGER_H

#include "../runtime/vm.h"

typedef struct {
    VM *vm;
    int *breakpoints;
    size_t bp_count;
    int paused;
} Debugger;

Debugger *debugger_create(VM *vm);
void debugger_set_breakpoint(Debugger *dbg, int line);
void debugger_step(Debugger *dbg);
void debugger_destroy(Debugger *dbg);

#endif
