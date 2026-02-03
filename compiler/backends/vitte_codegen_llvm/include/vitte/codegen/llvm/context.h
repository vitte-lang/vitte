// ============================================================
// vitte_codegen_llvm::context
// Contexte bas niveau du codegen LLVM
// ============================================================

#ifndef VITTE_CODEGEN_LLVM_CONTEXT_H
#define VITTE_CODEGEN_LLVM_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------------
// Dépendances LLVM (headers officiels)
// ------------------------------------------------------------

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Target/TargetMachine.h>


// ------------------------------------------------------------
// Niveaux d’optimisation codegen
// ------------------------------------------------------------

typedef enum vitte_codegen_llvm_opt_level {
    VITTE_LLVM_O0 = 0,
    VITTE_LLVM_O1 = 1,
    VITTE_LLVM_O2 = 2,
    VITTE_LLVM_O3 = 3,
    VITTE_LLVM_Os = 4,
    VITTE_LLVM_Oz = 5
} vitte_codegen_llvm_opt_level_t;


// ------------------------------------------------------------
// Format de sortie LLVM
// ------------------------------------------------------------

typedef enum vitte_codegen_llvm_emit_kind {
    VITTE_LLVM_EMIT_OBJECT,
    VITTE_LLVM_EMIT_ASSEMBLY,
    VITTE_LLVM_EMIT_BITCODE
} vitte_codegen_llvm_emit_kind_t;


// ------------------------------------------------------------
// Configuration du backend LLVM (codegen)
// ------------------------------------------------------------

typedef struct vitte_codegen_llvm_config {
    const char* target_triple;
    const char* cpu;
    const char* features;

    vitte_codegen_llvm_opt_level_t opt_level;
    vitte_codegen_llvm_emit_kind_t emit_kind;

    int verify_module;   // bool
    int debug_ir;        // bool
} vitte_codegen_llvm_config_t;


// ------------------------------------------------------------
// Contexte LLVM codegen
// ------------------------------------------------------------

typedef struct vitte_codegen_llvm_context {
    llvm::LLVMContext* llvm_context;
    llvm::TargetMachine* target_machine;
    llvm::DataLayout* data_layout;

    vitte_codegen_llvm_config_t config;
} vitte_codegen_llvm_context_t;


// ------------------------------------------------------------
// API — gestion du contexte
// ------------------------------------------------------------

/**
 * Initialise un contexte LLVM codegen.
 * Les targets LLVM doivent être déjà initialisées.
 */
vitte_codegen_llvm_context_t*
vitte_codegen_llvm_context_create(
    const vitte_codegen_llvm_config_t* config
);


/**
 * Libère le contexte LLVM codegen.
 */
void
vitte_codegen_llvm_context_destroy(
    vitte_codegen_llvm_context_t* ctx
);


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

/**
 * Retourne le niveau d’optimisation LLVM natif.
 */
int
vitte_codegen_llvm_opt_level_native(
    vitte_codegen_llvm_opt_level_t level
);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_CODEGEN_LLVM_CONTEXT_H
