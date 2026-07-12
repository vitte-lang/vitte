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
- matérialisation `.o` réelle par l'intrinsic hôte
  `vitte_host_emit_assembly_object`

## Modes d’intégration

- backend selectable: `asm-native`
- sortie object format: `asm-native-relocatable-model`
- intégré au pipeline backend/driver sans fallback implicite

## Contrats

- `instruction_selected` doit être vrai pour un module non vide,
- `registers_allocated` doit couvrir toutes instructions sélectionnées,
- `prologue/epilogue` attendus pour marquer le résultat `valid`.

`codegen.object_text` reste un manifeste lisible pour les diagnostics. Le
fichier de sortie est un vrai objet natif produit sans shell par
`clang -target ... -x assembler -c`. L'environnement enfant fixe
`SOURCE_DATE_EPOCH=0` et `ZERO_AR_DATE=1`, et le gate
`native-object-determinism-gate` exige deux sorties ELF octet pour octet
identiques.

Avant d'accepter le fichier, le runtime relit sa structure ELF64 et vérifie :

- le type relogeable et la machine cible ;
- les sections `.text`, `.symtab` et `.strtab` ;
- le symbole d'entrée défini ;
- une table de relocations lorsqu'un appel externe est présent dans l'IR.

Un échec de cette inspection invalide l'objet avec
`BACKEND_E_CODEGEN_FAILED` avant toute étape de linkage.
