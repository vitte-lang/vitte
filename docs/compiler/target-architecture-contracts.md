# Vérification des contrats de cibles

Cette vérification compare les profils présents dans le dépôt aux règles des
ABI et ISA officielles. Elle décrit les capacités annoncées et les limites de
l'implémentation actuelle; elle ne vaut pas validation d'une génération ou
d'une exécution sur chaque machine.

## Matrice des architectures

| Architecture | Pointeurs et types dépendants de la cible | Atomiques matériels | État dans Vitte |
| --- | --- | --- | --- |
| `x86_64` | Pointeurs et `usize` de 64 bits avec les ABI usuelles. SysV suit LP64; Win64 suit LLP64 (`long` reste 32 bits). | Les instructions RMW avec préfixe `LOCK` couvrent les largeurs scalaires usuelles jusqu'à 64 bits. Le compare-échange 128 bits `CMPXCHG16B` exige la capacité CPU et un opérande aligné sur 16 octets. | Le profil générique le nomme `X8664`; le backend C17 le nomme `X86_64`. Aucun champ CPU ne permet de vérifier `CMPXCHG16B`. |
| `aarch64` | AAPCS64 définit des pointeurs de données et de code 64 bits. Les ABI Linux/macOS et Windows n'ont pas toutes la même variante de données ni la même convention d'appel. | Les load-exclusive/store-exclusive permettent de construire des opérations atomiques; les instructions LSE de read-modify-write sont une extension Armv8.1-A, donc ne sont pas garanties par le seul nom `aarch64`. | Profil présent. La sélection par défaut choisit `Aapcs64` pour tout AArch64 avant d'examiner Windows; le profil ne distingue donc pas l'ABI Windows ARM64. Aucune extension CPU n'est décrite. |
| `riscv64` | L'ABI LP64* donne des pointeurs de 64 bits. RV64ILP32 reste à l'état draft; l'ABI big-endian n'est pas définie dans le psABI consulté. | L'extension `A` n'est pas impliquée par `riscv64`. Elle fournit LR/SC et AMO au format word, et doubleword sur RV64; les accès atomiques mal alignés ne sont pas une garantie portable. Les opérations plus étroites dépendent des extensions ou d'une séquence backend vérifiée. | Profil présent dans `CodegenArch` et réduit à `RISCV` par le backend C17. Ni ABI LP64*, ni extensions `A`/`Zabha`/`Zacas` ne sont représentées. |
| `wasm32` | Le modèle mémoire 32 bits utilise des pointeurs de 32 bits. `wasm64` n'est pas dans les profils Vitte. | Pour synchroniser des threads, il faut le support Threads et une mémoire partagée. Les accès atomiques doivent respecter l'alignement naturel indiqué par l'instruction. Le modèle courant de la proposition Threads exprime les opérations atomiques en séquentiel cohérent. | Profil présent dans `CodegenArch`, mais absent de `TargetArchitecture` C17. Aucun champ n'indique Threads, mémoire partagée ou le sous-ensemble d'ordres mémoire disponible. |

Les types de taille fixe restent distincts du modèle de pointeurs. La
documentation `core` promet `i128`/`u128`, mais `IRTypeKind` ne contient que
les types entiers jusqu'à 64 bits. Il n'existe donc pas encore de contrat
cohérent pour leur layout, leur arithmétique ou leurs atomiques selon la cible.

## Pointeurs et validation du layout

Les contrats usuels à retenir sont :

- `usize`, `isize`, les pointeurs et les références doivent prendre la largeur
  du modèle de données sélectionné par l'ABI, pas celle de l'hôte qui exécute
  le compilateur;
- les pointeurs de slice et leur longueur doivent suivre le layout cible, y
  compris leur alignement et le padding de la forme agrégée;
- `pointer_bits`, `size_bits`, l'endianness, l'ABI et l'architecture doivent
  former un tuple compatible. Une valeur de largeur seule ne suffit pas à
  définir un layout;
- un pointeur de capacité (par exemple CHERI) n'est pas un pointeur entier
  ordinaire. Aucune ABI de capacité ni règle de provenance n'est modélisée ici.

Le dépôt ne valide pas encore ces invariants uniformément :

1. `vitte_codegen_target_validate` exige seulement des enums valides et des
   largeurs non nulles. Il accepterait par exemple `wasm32` avec un pointeur
   de 64 bits.
2. `target_validate` du backend C17 accepte les pointeurs de 16, 32, 64 ou
   128 bits sans les relier à l'architecture. `core::internal::platform`
   n'accepte que 16, 32 ou 64 bits. Le profil 128 bits est donc accepté par
   une couche et rejeté par une autre, sans ABI de capacité déclarée.
3. `vitte_codegen_target_default` construit le profil depuis
   `version_architecture()` et `sizeof` de l'hôte. Le bridge natif appelle
   cette fonction par défaut; l'option CLI `--target` n'est pas transmise à ce
   chemin. Cette voie ne peut donc pas confirmer un layout de cross-compilation.
4. Le profil générique annonce `wasm32` et `riscv64`; le backend C17 ne connaît
   pas WebAssembly, et le modèle `core::internal::platform` ne connaît pas
   RISC-V. Les listes d'architectures ne décrivent pas encore une matrice
   commune de cibles compilables.

## Limites atomiques à appliquer

Le type atomique ne peut pas être approuvé sur la seule base de
l'architecture. Pour chaque opération, largeur et profil CPU, le compilateur
doit vérifier :

- que l'accès est naturellement aligné et que le type est un scalaire
  autorisé; `fetch_add` et `fetch_sub` doivent être limités aux entiers;
- que l'instruction ou séquence choisie supporte la largeur et l'ordre mémoire;
- si la primitive est toujours sans verrou, dépend d'une extension CPU, ou
  passe par une routine runtime Vitte explicite;
- que l'ordre demandé est préservé. Pour WebAssembly Threads, un abaissement
  plus fort en `seq_cst` peut être correct, mais doit rester explicite et ne
  doit pas inventer des ordres non pris en charge par le backend.

Aujourd'hui, `CodegenTarget` ne porte aucune extension CPU, l'IR n'a pas
d'opcodes atomiques et l'émetteur C17 traite `Load`/`Store` comme des accès
ordinaires. Les wrappers `core::sync` ne peuvent donc pas être considérés
comme portables ni atomiques sur l'une de ces cibles, même si le matériel
possède les instructions requises. Voir aussi
[`intrinsic-boundaries.md`](intrinsic-boundaries.md).

## Décision de support

Pour le compilateur actuel, les noms `x86_64`, `aarch64`, `riscv64` et `wasm32`
sont des étiquettes de profil, pas une promesse de code natif portable. Les
profils utilisables doivent d'abord fixer explicitement le modèle de données
et l'ABI. La prise en charge des atomiques doit ensuite être annoncée par
largeur et extensions CPU, et seulement après ajout des opérations atomiques
typées à l'IR et aux backends.

Ordre de correction :

1. Introduire un layout cible canonique et rejeter les combinaisons
   architecture/OS/ABI/largeurs incompatibles; y raccorder `usize`, `isize`,
   pointeurs, références et slices.
2. Transmettre le triple demandé par `--target` jusqu'au layout et au backend;
   ne pas déduire le layout cible du `sizeof` de l'hôte.
3. Ajouter les extensions/profils CPU et des capacités atomiques par largeur,
   notamment `CMPXCHG16B`, LSE, `A`/extensions atomiques RISC-V et Threads
   WebAssembly.
4. Ajouter des opérations IR typées avec ordre mémoire, largeur et contrainte
   d'alignement, puis faire diagnostiquer les combinaisons non prises en
   charge.

## Références normatives

- [Intel 64 and IA-32 Software Developer's Manual, CMPXCHG16B](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2a-manual.pdf)
- [x86-64 psABI](https://gitlab.com/x86-psABIs/x86-64-ABI), [modèle Windows LLP64](https://learn.microsoft.com/en-us/windows/win32/winprog64/abstract-data-models)
- [Arm AAPCS64, types et pointeurs](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst) et [Armv8.x, extension LSE](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/the-armv8-a-architecture-and-its-ongoing-development)
- [RISC-V ELF psABI, modèles ABI](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/introduction.adoc), [conventions d'appel](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc) et [extension A](https://docs.riscv.org/reference/isa/unpriv/a-st-ext.html)
- [Proposition WebAssembly Threads, atomiques et alignement](https://github.com/WebAssembly/threads/blob/main/proposals/threads/Overview.md)

## Sources du dépôt examinées

`src/compiler/codegen/codegen.vitte`, `src/compiler/backend/c17/backend.vitte`,
`src/compiler/ir/ir.vitte`, `src/compiler/backend/c17/ir_emitter.vitte`,
`src/compiler/driver/native_frontend_bridge.c`,
`modules/core/src/internal/platform.vit`,
`modules/core/src/intrinsics/atomic.vit` et
`docs/compiler/stdlib_core_primitive.md`.
