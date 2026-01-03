#ifndef VITTE_CALLING_CONVENTION_H
#define VITTE_CALLING_CONVENTION_H

#include <stdint.h>
#include <stdbool.h>

/* Calling convention definitions for Vitte ABI */

/* Register allocation */
#define VITTE_ARG_REGS 6        /* Arguments passed in registers */
#define VITTE_CALLER_SAVED 12   /* Caller must save */
#define VITTE_CALLEE_SAVED 5    /* Callee must preserve */

/* Stack frame structure */
typedef struct {
    void *return_address;
    void *saved_frame_pointer;
    uint64_t saved_registers[VITTE_CALLEE_SAVED];
    uint64_t local_variables[256];
} vitte_stack_frame_t;

/* Function signature */
typedef struct {
    const char *name;
    int arg_count;
    int return_value_reg;
    int *arg_registers;
    bool is_variadic;
} vitte_function_signature_t;

/* Call frame */
typedef struct {
    void *function_ptr;
    int arg_count;
    int return_register;
    uint64_t saved_registers[VITTE_CALLER_SAVED];
    void *saved_sp;
    void *saved_fp;
} vitte_call_frame_t;

/* ABI operations */
void abi_init_call_frame(vitte_call_frame_t *frame, void *function_ptr, int arg_count);
void abi_setup_arguments(vitte_call_frame_t *frame, uint64_t *args);
void abi_save_registers(vitte_call_frame_t *frame);
void abi_restore_registers(vitte_call_frame_t *frame);
void abi_push_frame(vitte_call_frame_t *frame);
void abi_pop_frame(vitte_call_frame_t *frame);

/* Register mapping */
#define VITTE_REG_ARG0  0       /* First argument */
#define VITTE_REG_ARG1  1       /* Second argument */
#define VITTE_REG_ARG2  2       /* Third argument */
#define VITTE_REG_ARG3  3       /* Fourth argument */
#define VITTE_REG_ARG4  4       /* Fifth argument */
#define VITTE_REG_ARG5  5       /* Sixth argument */
#define VITTE_REG_RETURN 0      /* Return value */
#define VITTE_REG_FP    15      /* Frame pointer */
#define VITTE_REG_SP    14      /* Stack pointer */

#endif
