// ============================================================
// vitte_codegen_llvm::emit
// Interface publique d’émission LLVM
// ============================================================

#pragma once

#include <string>

namespace llvm {
class Module;
}

namespace vitte::codegen::llvm_backend::emit {


// ------------------------------------------------------------
// Format de sortie
// ------------------------------------------------------------

enum class OutputKind {
    Object,
    Assembly,
    Bitcode,
};


// ------------------------------------------------------------
// API principale
// ------------------------------------------------------------

/**
 * Émet un module LLVM vers un fichier.
 *
 * @param module     Module LLVM déjà généré et valide
 * @param outputPath Chemin du fichier de sortie
 * @param kind       Type de sortie (objet, asm, bitcode)
 * @param triple     Target triple LLVM (ex: x86_64-pc-linux-gnu)
 * @param cpu        CPU cible (ex: generic, skylake, cortex-a72)
 * @param features   Features LLVM (ex: +sse4.2,+avx)
 *
 * @return true si succès, false sinon
 */
bool emitModule(llvm::Module& module,
                const std::string& outputPath,
                OutputKind kind,
                const std::string& triple,
                const std::string& cpu = "generic",
                const std::string& features = "");


/**
 * Émet un fichier objet (.o / .obj)
 */
bool emitObject(llvm::Module& module,
                const std::string& path,
                const std::string& triple);


/**
 * Émet un fichier assembleur (.s)
 */
bool emitAssembly(llvm::Module& module,
                  const std::string& path,
                  const std::string& triple);


/**
 * Émet du LLVM bitcode (.bc)
 */
bool emitBitcode(llvm::Module& module,
                 const std::string& path);

} // namespace vitte::codegen::llvm_backend::emit
