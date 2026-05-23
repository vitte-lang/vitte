# Native ASM Backend (177)

Objectif: backend assembleur natif minimal mais traçable.

## Pipeline

- instruction selection (IR -> x86_64 textual ops)
- register allocation (virtual -> emplacement concret)
- stack layout estimation
- calling convention baseline (`sysv-amd64`)
- prologue/epilogue validation signal
- object text packaging

## Modes d’intégration

- backend selectable: `asm-native`
- sortie object format: `asm-native-relocatable-model`
- intégré au pipeline backend/driver sans fallback implicite

## Contrats

- `instruction_selected` doit être vrai pour un module non vide,
- `registers_allocated` doit couvrir toutes instructions sélectionnées,
- `prologue/epilogue` attendus pour marquer le résultat `valid`.
