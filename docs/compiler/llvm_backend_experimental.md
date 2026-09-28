# LLVM Backend Experimental (176)

Objectif: proposer un backend LLVM avancé optionnel.

## Surface backend

- backend selectable: `llvm-exp`
- conserve les profils (`enterprise-*`) et les options d’optimisation
- respecte la chaîne toolchain LLVM (IR -> object -> link)

## Capacités fondation

- LLVM IR emission (via bindings backend)
- debug metadata path (via profile options)
- optimization bridge (`O0..Oz`, LTO/PGO compat checks)
- target lowering multi-target (`x86_64`, `aarch64`, `riscv64`, `i386`)
- linker integration via toolchain status/diagnostics

## Contrat de sécurité pipeline

- en cas d’échec object/toolchain: erreur codegen explicite
- aucun fallback silencieux sur backend critique
- diagnostics ramenés au niveau driver
