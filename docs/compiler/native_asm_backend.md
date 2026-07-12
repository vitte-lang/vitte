# Native ASM Backend (177)

Objectif: backend assembleur natif traçable avec ABI et layout explicites.

## Pipeline

- instruction selection (IR -> x86_64 textual ops)
- register allocation (virtual -> emplacement concret)
- stack layout typé avec frames alignées
- conventions SysV64 et Win64 consommées depuis le profil cible canonique
- modèle AAPCS64 partagé avec les autres backends
- registres d'arguments/retours, arguments de pile et retours `sret`
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

Le format objet reste un modèle relogeable textuel. La matérialisation d'un
objet natif déterministe est un gate séparé du contrat ABI machine.
