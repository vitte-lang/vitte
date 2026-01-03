#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

debug_context_t* debug_create(void) {
    debug_context_t *dbg = (debug_context_t *)malloc(sizeof(debug_context_t));
    if (!dbg) return NULL;
    
    dbg->breakpoints = (breakpoint_t *)malloc(sizeof(breakpoint_t) * 64);
    dbg->breakpoint_count = 0;
    
    dbg->flags.trace_instructions = false;
    dbg->flags.trace_stack = false;
    dbg->flags.trace_memory = false;
    dbg->flags.trace_gc = false;
    dbg->flags.break_on_error = false;
    dbg->flags.verbose = false;
    
    dbg->instruction_count = 0;
    dbg->stack_operations = 0;
    dbg->memory_operations = 0;
    
    return dbg;
}

void debug_free(debug_context_t *dbg) {
    if (!dbg) return;
    free(dbg->breakpoints);
    free(dbg);
}

void debug_enable_tracing(debug_context_t *dbg, bool enable) {
    if (!dbg) return;
    dbg->flags.trace_instructions = enable;
}

void debug_enable_stack_trace(debug_context_t *dbg, bool enable) {
    if (!dbg) return;
    dbg->flags.trace_stack = enable;
}

void debug_enable_memory_trace(debug_context_t *dbg, bool enable) {
    if (!dbg) return;
    dbg->flags.trace_memory = enable;
}

void debug_enable_gc_trace(debug_context_t *dbg, bool enable) {
    if (!dbg) return;
    dbg->flags.trace_gc = enable;
}

void debug_trace_instruction(debug_context_t *dbg, uint8_t opcode, int offset) {
    if (!dbg || !dbg->flags.trace_instructions) return;
    
    dbg->instruction_count++;
    printf("[TRACE] Instruction %llu @ offset %d (opcode=%d)\n", 
           dbg->instruction_count, offset, opcode);
}

void debug_trace_stack(debug_context_t *dbg, vitte_vm_t *vm) {
    if (!dbg || !dbg->flags.trace_stack || !vm) return;
    
    dbg->stack_operations++;
    printf("[STACK] Operation %llu - Stack top: %d\n", 
           dbg->stack_operations, vm->stack_top);
}

void debug_trace_memory(debug_context_t *dbg, void *ptr, size_t size, const char *operation) {
    if (!dbg || !dbg->flags.trace_memory) return;
    
    dbg->memory_operations++;
    printf("[MEMORY] Operation %llu - %s %zu bytes @ %p\n", 
           dbg->memory_operations, operation, size, ptr);
}

void debug_add_breakpoint(debug_context_t *dbg, int line, const char *function) {
    if (!dbg || dbg->breakpoint_count >= 64) return;
    
    dbg->breakpoints[dbg->breakpoint_count].line = line;
    dbg->breakpoints[dbg->breakpoint_count].function = function;
    dbg->breakpoints[dbg->breakpoint_count].enabled = true;
    dbg->breakpoint_count++;
}

void debug_remove_breakpoint(debug_context_t *dbg, int index) {
    if (!dbg || index < 0 || index >= dbg->breakpoint_count) return;
    
    dbg->breakpoints[index] = dbg->breakpoints[dbg->breakpoint_count - 1];
    dbg->breakpoint_count--;
}

bool debug_check_breakpoint(debug_context_t *dbg, int line) {
    if (!dbg) return false;
    
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].line == line && dbg->breakpoints[i].enabled) {
            return true;
        }
    }
    
    return false;
}

void debug_print_statistics(debug_context_t *dbg) {
    if (!dbg) return;
    
    printf("=== Debug Statistics ===\n");
    printf("Instructions Traced: %llu\n", dbg->instruction_count);
    printf("Stack Operations:    %llu\n", dbg->stack_operations);
    printf("Memory Operations:   %llu\n", dbg->memory_operations);
    printf("Breakpoints Set:     %d\n", dbg->breakpoint_count);
    printf("\n");
}
