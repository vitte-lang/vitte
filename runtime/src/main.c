#include "../include/vitte_types.h"
#include "../include/vitte_vm.h"
#include "../gc/garbage_collector.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

/* REPL (Read-Eval-Print Loop) */
typedef struct {
    vitte_vm_t *vm;
    gc_heap_t *gc;
    bool is_running;
    int line_count;
} vitte_runtime_t;

/* Initialize runtime */
vitte_runtime_t* runtime_create(void) {
    vitte_runtime_t *rt = (vitte_runtime_t *)malloc(sizeof(vitte_runtime_t));
    if (!rt) return NULL;
    
    rt->vm = vitte_vm_create();
    rt->gc = gc_create(1024 * 1024);  /* 1MB initial threshold */
    rt->is_running = true;
    rt->line_count = 0;
    
    if (!rt->vm || !rt->gc) {
        free(rt);
        return NULL;
    }
    
    return rt;
}

void runtime_free(vitte_runtime_t *rt) {
    if (!rt) return;
    
    if (rt->vm) vitte_vm_free(rt->vm);
    if (rt->gc) gc_free(rt->gc);
    
    free(rt);
}

/* REPL prompt */
void repl_print_welcome(void) {
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║         Vitte Language Runtime v1.0.0              ║\n");
    printf("║      Type 'help' for commands, 'exit' to quit      ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void repl_print_prompt(int line) {
    printf("vitte:%d> ", line);
    fflush(stdout);
}

void repl_print_help(void) {
    printf("\nVitte REPL Commands:\n");
    printf("  help              - Show this help message\n");
    printf("  builtins          - List available built-in functions\n");
    printf("  gc_stats          - Show garbage collector statistics\n");
    printf("  mem_info          - Show memory information\n");
    printf("  disasm            - Disassemble last bytecode\n");
    printf("  exit              - Exit the REPL\n");
    printf("\n");
}

void repl_show_builtins(void) {
    printf("\nAvailable Built-in Functions (%d total):\n", builtin_count());
    printf("  String:   strlen, substr, strcat, strtoupper\n");
    printf("  Math:     sqrt, abs, floor, ceil, pow\n");
    printf("  Array:    array_len, array_push, array_pop\n");
    printf("  Type:     to_int, to_float, to_string, type_of\n\n");
}

void repl_show_gc_stats(vitte_runtime_t *rt) {
    gc_print_stats(rt->gc);
    printf("\n");
}

void repl_show_mem_info(vitte_runtime_t *rt) {
    printf("\n=== Memory Information ===\n");
    printf("Stack Top:       %d / %d\n", rt->vm->stack_top, VITTE_STACK_MAX);
    printf("Heap Used:       %zu / %zu bytes\n", rt->vm->heap_used, rt->vm->heap_size);
    printf("GC Allocated:    %zu bytes\n", gc_bytes_allocated(rt->gc));
    printf("Instructions:    %" PRIu64 "\n", rt->vm->instruction_count);
    printf("\n");
}

/* Execute bytecode */
vitte_result_t runtime_execute(vitte_runtime_t *rt, chunk_t *chunk) {
    if (!rt || !chunk) return VITTE_RUNTIME_ERROR;
    
    vitte_result_t result = vitte_vm_execute(rt->vm, chunk);
    
    /* Check if GC should run */
    if (gc_bytes_allocated(rt->gc) > rt->gc->next_gc_threshold) {
        printf("[GC] Running garbage collection...\n");
        gc_collect(rt->vm, rt->gc);
    }
    
    return result;
}

/* Test programs */
void test_arithmetic(vitte_runtime_t *rt) {
    printf("\n=== Testing Arithmetic ===\n");
    
    chunk_t *chunk = chunk_create();
    if (!chunk) return;
    
    /* Compile: 42 + 8 */
    int const_42 = chunk_add_constant(chunk, vitte_int(42));
    int const_8 = chunk_add_constant(chunk, vitte_int(8));
    
    chunk_write(chunk, OP_CONST, 1);
    chunk_write(chunk, const_42, 1);
    chunk_write(chunk, OP_CONST, 1);
    chunk_write(chunk, const_8, 1);
    chunk_write(chunk, OP_ADD, 1);
    chunk_write(chunk, OP_PRINT, 1);
    chunk_write(chunk, OP_HALT, 1);
    
    printf("Result of 42 + 8:\n");
    runtime_execute(rt, chunk);
    
    chunk_free(chunk);
}

void test_strings(vitte_runtime_t *rt) {
    printf("\n=== Testing Strings ===\n");
    
    chunk_t *chunk = chunk_create();
    if (!chunk) return;
    
    /* Create and print a string */
    int const_hello = chunk_add_constant(chunk, vitte_string("Hello, Vitte!"));
    
    chunk_write(chunk, OP_CONST, 1);
    chunk_write(chunk, const_hello, 1);
    chunk_write(chunk, OP_PRINT, 1);
    chunk_write(chunk, OP_HALT, 1);
    
    printf("String output:\n");
    runtime_execute(rt, chunk);
    
    chunk_free(chunk);
}

void test_arrays(vitte_runtime_t *rt) {
    printf("\n=== Testing Arrays ===\n");
    
    vitte_value_t arr = vitte_array();
    vitte_array_t *arr_obj = (vitte_array_t *)arr.as.pointer;
    
    /* Push values */
    vitte_value_t vals[] = {
        vitte_int(1),
        vitte_int(2),
        vitte_int(3)
    };
    
    for (int i = 0; i < 3; i++) {
        if (arr_obj->count >= arr_obj->capacity) {
            arr_obj->capacity *= 2;
            arr_obj->values = (vitte_value_t *)realloc(arr_obj->values, 
                                                       sizeof(vitte_value_t) * arr_obj->capacity);
        }
        arr_obj->values[arr_obj->count++] = vals[i];
    }
    
    printf("Array created with %d elements\n", arr_obj->count);
    printf("Elements: ");
    for (int i = 0; i < arr_obj->count; i++) {
        vitte_print(arr_obj->values[i]);
        if (i < arr_obj->count - 1) printf(", ");
    }
    printf("\n");
}

/* Main REPL loop */
void repl_run(vitte_runtime_t *rt) {
    repl_print_welcome();
    
    while (rt->is_running) {
        repl_print_prompt(rt->line_count++);
        
        char *input = readline("");
        if (!input) {
            printf("exit\n");
            break;
        }
        
        /* Process commands */
        if (strcmp(input, "exit") == 0) {
            rt->is_running = false;
        } else if (strcmp(input, "help") == 0) {
            repl_print_help();
        } else if (strcmp(input, "builtins") == 0) {
            repl_show_builtins();
        } else if (strcmp(input, "gc_stats") == 0) {
            repl_show_gc_stats(rt);
        } else if (strcmp(input, "mem_info") == 0) {
            repl_show_mem_info(rt);
        } else if (strlen(input) > 0) {
            printf("Unknown command: %s (type 'help' for available commands)\n", input);
        }
        
        free(input);
    }
    
    printf("Goodbye!\n");
}

/* Main entry point */
int main(int argc, char *argv[]) {
    vitte_runtime_t *rt = runtime_create();
    if (!rt) {
        fprintf(stderr, "Failed to create runtime\n");
        return 1;
    }
    
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* Run tests */
        printf("Running Vitte Runtime Tests\n");
        printf("============================\n");
        
        test_arithmetic(rt);
        test_strings(rt);
        test_arrays(rt);
        
        printf("\n✓ All tests completed\n");
    } else {
        /* Run REPL */
        repl_run(rt);
    }
    
    runtime_free(rt);
    return 0;
}
