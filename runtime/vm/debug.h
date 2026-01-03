#ifndef VITTE_DEBUG_H
#define VITTE_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct vitte_vm vitte_vm_t;

/* Debug flags */
typedef struct {
    bool trace_instructions;
    bool trace_stack;
    bool trace_memory;
    bool trace_gc;
    bool break_on_error;
    bool verbose;
} debug_flags_t;

/* Breakpoint */
typedef struct {
    int line;
    const char *function;
    bool enabled;
} breakpoint_t;

/* Debug context */
typedef struct {
    debug_flags_t flags;
    breakpoint_t *breakpoints;
    int breakpoint_count;
    
    uint64_t instruction_count;
    uint64_t stack_operations;
    uint64_t memory_operations;
} debug_context_t;

/* Debug operations */
debug_context_t* debug_create(void);
void debug_free(debug_context_t *dbg);

void debug_enable_tracing(debug_context_t *dbg, bool enable);
void debug_enable_stack_trace(debug_context_t *dbg, bool enable);
void debug_enable_memory_trace(debug_context_t *dbg, bool enable);
void debug_enable_gc_trace(debug_context_t *dbg, bool enable);

/* Tracing */
void debug_trace_instruction(debug_context_t *dbg, uint8_t opcode, int offset);
void debug_trace_stack(debug_context_t *dbg, vitte_vm_t *vm);
void debug_trace_memory(debug_context_t *dbg, void *ptr, size_t size, const char *operation);

/* Breakpoints */
void debug_add_breakpoint(debug_context_t *dbg, int line, const char *function);
void debug_remove_breakpoint(debug_context_t *dbg, int index);
bool debug_check_breakpoint(debug_context_t *dbg, int line);

/* Statistics */
void debug_print_statistics(debug_context_t *dbg);

#endif
