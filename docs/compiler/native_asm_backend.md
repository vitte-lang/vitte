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

## Toolchain native

`NativeToolchain` constitue l'unique contrat pour le compilateur LLVM IR, le
pilote assembleur, le pilote de linkage, la source runtime et son répertoire
d'en-têtes. Chaque opération produit un `NativeToolCommand` avec un rôle, un
outil, une liste d'arguments structurée et un rendu réservé aux diagnostics.

Le runtime C reçoit explicitement les chemins issus de ce plan et exécute le
tableau `argv` sans shell. Les noms par défaut `clang` et `cc` ne vivent que dans
la construction de la toolchain par défaut ; une toolchain explicite peut les
remplacer sans modifier le bridge ni le runtime.

## Matrice d'architectures

`backend-architecture-matrix-gate` matérialise deux fois un objet ELF64 pour
`x86_64-unknown-linux-gnu` et `aarch64-unknown-linux-gnu`. Chaque paire doit être
identique octet pour octet. Le vérificateur runtime contrôle la machine cible,
le symbole exporté et l'unwind ; le gate relit aussi `ET_REL`, `.text`, `.symtab`,
`.strtab`, `.eh_frame` et `.note.GNU-stack`.

## Debug et unwind

Le mode debug émet les directives `.file`/`.loc`, une unité de compilation
DWARF v4 minimale et les sections `.debug_info`, `.debug_abbrev`, `.debug_str`
et `.debug_line`. Les chemins enregistrés sont réduits au nom du fichier pour
préserver la reproductibilité entre checkouts.

Les directives CFI décrivent le CFA, la sauvegarde de `%rbp` et sa restauration.
Elles produisent `.eh_frame` dans tous les profils afin de conserver le
déroulage de pile en release. Quand `emit_debug_sections` est faux, les sections
DWARF disparaissent mais `.eh_frame` reste obligatoire.
