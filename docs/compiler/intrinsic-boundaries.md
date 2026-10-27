# Frontières des intrinsics natives

Ce document fixe la séparation entre les opérations définies par le langage,
les opérations qui doivent devenir des instructions du backend et les services
fournis par le runtime. Une déclaration `export intrinsic` est un contrat
source; elle ne prouve pas qu'un backend sait déjà l'abaisser.

La règle est qu'aucune de ces opérations ne passe par `export extern proc` ou
par un import C écrit en Vitte. Le compilateur choisit une instruction native,
une séquence machine ou un symbole de runtime Vitte connu du backend. Le backend
C17 peut employer des constructions C dans le fichier généré, mais elles ne
deviennent pas des imports C de l'API Vitte.

## Statut observé dans le dépôt

| Surface | Contrat source | Déclaration au registre | IR/backend actuel |
| --- | --- | --- | --- |
| Atomiques | `core::intrinsics::atomic` déclare charge, stockage, échange, compare-échange, add, sub et fence | Les sept noms ont des descripteurs `LOWER_DIRECTLY` dans `compiler::builtin` | Aucun opcode atomique dans `IROpcode` ni chemin atomique dans l'émetteur C17 Vitte; le contrôle sémantique des wrappers `core::sync::atomic` échoue sur les paramètres génériques |
| Mémoire volatile | `volatile_load<T>` et `volatile_store<T>` | Aucun descripteur | L'IR n'a que `Load` et `Store` ordinaires; l'émetteur C17 les rend par `*pointer`, sans marqueur volatile |
| Allocation brute | `native_allocate` et `native_deallocate` | Aucun descripteur | Pas d'opcode ni d'abaissement natif dédié; le backend bootstrap C17 reconnaît les noms `malloc` et `free`, pas ces intrinsics |
| Copie, déplacement, remplissage | `intrinsic_copy`, `intrinsic_move` et `intrinsic_fill` sont des procédures Vitte avec boucles; le registre connaît séparément `memcpy`, `memmove`, `memset` et `memcmp` | Descripteurs mémoire présents, sans chemin d'abaissement observé dans l'IR C17 Vitte | Les procédures `core` ont une implémentation Vitte; aucune opération mémoire dédiée dans `IROpcode` |
| Trap et chemin inaccessible | `trap()` est déclaré intrinsic; `unreachable` est aussi une déclaration dans le registre | `trap` et `unreachable` ont des descripteurs directs | `IROpcode.Unreachable` est rendu par `abort();`; le backend ne montre pas d'abaissement dédié de `trap` |
| Conversion de représentation | `mem_transmute<T,U>` est déclarée intrinsic | Aucun descripteur dédié | Aucun opcode dédié; à abaisser en conversion de représentation validée, sans appel de fonction |

Le registre `compiler::builtin` décrit des propriétés et des arités; il ne
constitue pas à lui seul un abaisseur. L'IR et le backend doivent porter les
opérations et leur sémantique jusqu'au code final.

## Opérations atomiques

Les primitives backend requises sont :

| Intrinsic | Sémantique à conserver |
| --- | --- |
| `atomic_load(ptr, order)` | Une lecture indivisible; accepte `Relaxed`, `Acquire` ou `SeqCst`. |
| `atomic_store(ptr, value, order)` | Une écriture indivisible; accepte `Relaxed`, `Release` ou `SeqCst`. |
| `atomic_exchange(ptr, value, order)` | Remplace indivisiblement et retourne l'ancienne valeur. |
| `atomic_compare_exchange(ptr, expected, replacement, success, failure)` | Compare puis remplace en une opération indivisible; retourne si le remplacement a eu lieu. L'ordre d'échec est seulement `Relaxed`, `Acquire` ou `SeqCst`, et ne peut pas dépasser l'ordre de succès. |
| `atomic_fetch_add/sub(ptr, amount, order)` | Mise à jour indivisible; retourne l'ancienne valeur. Réservé aux types entiers atomiques. |
| `atomic_fence(order)` | Barrière de synchronisation correspondant à l'ordre demandé. |

Les fonctions `fetch_and`, `fetch_or` et `fetch_xor` de `core::sync::atomic`
sont des boucles compare-échange en Vitte; le backend peut les optimiser mais
n'a pas besoin d'intrinsics publiques supplémentaires pour les rendre
correctes.

Le contrat générique actuel est trop large. La première ABI atomique doit
accepter seulement les scalaires primitifs supportés par la cible, avec
alignement naturel; `fetch_add/sub` doit en plus exiger un entier. Les formes
agrégées et les types non atomiques doivent produire un diagnostic. Les ordres
doivent être validés pendant l'analyse sémantique, plutôt que par un appelant
qui oublierait les fonctions auxiliaires de `core::sync::ordering`.

Les atomiques doivent devenir des instructions IR typées. Les réduire à des
charges/stores ordinaires est incorrect, même sur une cible où ces accès semblent
indivisibles. Le backend choisit les instructions machine ou, pour C17, une
construction `_Atomic`/`stdatomic` équivalente; aucun symbole atomique C ne doit
être déclaré depuis l'API Vitte. Les cibles qui ne supportent pas une largeur
donnée doivent signaler cette limite ou utiliser une primitive runtime
explicitement définie par le backend.

## Opérations mémoire

- Les `Load` et `Store` ordinaires restent des opérations IR normales.
- `volatile_load/store` doivent garder une opération volatile dédiée jusqu'à
  l'émission. Chaque appel effectue exactement un accès de la largeur de `T`;
  ces opérations ne sont pas atomiques et n'établissent pas de synchronisation
  entre threads. L'adresse doit être valide et correctement alignée.
- `memcpy`, `memmove`, `memset` et `memcmp` peuvent être des opcodes ou être
  abaissés en boucles backend. `memcpy` interdit le chevauchement; `memmove` le
  permet; `memset` écrit un octet répété; `memcmp` compare des octets non signés
  et retourne un ordre négatif, nul ou positif. Le backend ne doit pas les
  implémenter en déclarant des fonctions C externes.
- `intrinsic_copy/move/fill` sont actuellement des fonctions Vitte. Elles
  restent correctes sans reconnaissance spéciale du compilateur; leur nom
  `intrinsic_` ne les transforme pas en intrinsics.
- `native_allocate/deallocate` sont des opérations de runtime, car l'allocation
  dépend du système cible. Le backend doit les relier à l'ABI runtime Vitte de
  la cible, avec la règle d'alignement documentée par `core`; il ne doit pas
  résoudre ces noms en symboles C importés. `alloc::allocator::system` ajoute
  l'alignement supérieur au pointeur et conserve la base pour la libération.
- `mem_transmute` devient une conversion de représentation sans appel. Le
  compilateur doit vérifier les tailles connues à la compilation; la validité
  des bits de la destination reste une précondition de l'opération unsafe.

## Trap et contrôle de flot

`trap()` doit terminer l'exécution immédiatement avec une primitive trap de la
cible et un effet `noreturn`. Il ne doit pas appeler `abort`, `exit`, `raise` ou
une fonction importée. `unreachable` est un terminateur de flot et une promesse
au compilateur; il ne doit pas être confondu avec un trap observable. Un backend
peut l'émettre comme indication d'inaccessibilité, avec un trap en configuration
de diagnostic. `panic()` peut préparer son contexte en Vitte puis appeler
`trap()`.

`debug_break` et `assume` sont des intrinsics de compilateur connexes : le
premier est dépendant de la cible; le second porte une hypothèse d'optimisation
et ne peut pas servir de contrôle de validation à l'exécution.

## Travail requis par couche

1. **Sémantique** — lier les noms réservés au registre, vérifier arité, types,
   restrictions atomiques et ordres mémoire; rejeter les intrinsics inconnues.
2. **IR** — ajouter des opcodes atomiques, volatils et trap distincts; conserver
   types, ordres et effets mémoire. Relier les builtins mémoire à des opcodes
   ou à des fonctions Vitte normales.
3. **Backend** — abaisser les opcodes sans prototypes externes; remplacer
   `Unreachable -> abort()` par une vraie terminaison IR; fournir l'ABI runtime
   cible pour l'allocation.
4. **Vérification de sortie** — inspecter le code généré et les symboles liés
   pour garantir qu'aucun `extern` C atomique, mémoire ou trap n'a été introduit.

Sources de référence : `modules/core/src/intrinsics/{atomic,memory,unreachable}.vit`,
`modules/core/src/mem/transmute.vit`, `modules/core/src/sync/ordering.vit`,
`src/compiler/builtin/builtin.vitte`, `src/compiler/ir/ir.vitte`,
`src/compiler/backend/c17/ir_emitter.vitte` et
`src/bootstrap/backend/c17/ast_emitter.c`.
