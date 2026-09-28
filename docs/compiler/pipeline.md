# Pipeline de compilation réel

La source de vérité exécutable de cette cartographie est
`src/vitte/compiler/infrastructure/pipeline_contract.vit`. Elle décrit les
artefacts échangés, la frontière contractuelle, le fichier et le symbole qui
implémentent chaque étape. `canonical_pipeline_contract_ready` refuse une carte
incomplète, discontinue ou contradictoire.

## Comment une route est sélectionnée

La route auto-hébergée part de `src/vitte/compiler/main.vit`, puis est
configurée par `src/vitte/compiler/driver/cli.vit` et
`src/vitte/compiler/driver/compile.vit` :

| Configuration | Route réellement exécutée |
| --- | --- |
| `@backend=llvm` | frontend/middle-end commun, puis LLVM IR, objet et lien natifs |
| `@backend=llvm-ir` | même route LLVM ; le profil de compilation conserve la cible LLVM |
| `@backend=x86_64` (valeur par défaut) | frontend/middle-end commun, puis sélection d'instructions x86_64, assemblage et lien |

Ces valeurs sont des lignes de manifeste (`@backend=...`), pas des noms de
répertoires. La sélection est faite dans
`src/vitte/compiler/driver/compile.vit`; elle ne doit donc pas être déduite du
backend C historique situé sous `src/vitte/compiler/backend/c`.

### Compatibilité backend / cible

La validation de configuration intervient avant le codegen :

| Backend | Cibles acceptées | Options spécifiques |
| --- | --- | --- |
| `x86_64` | `x86_64-linux` uniquement | sélection native, allocation de registres et assemblage |
| `llvm` | `x86_64-linux`, `aarch64-linux`, `riscv64-linux`, `i386-linux` | optimisation LLVM, LTO/PGO et toolchain LLVM |
| `llvm-ir` | mêmes cibles que `llvm` | même sélection LLVM, avec conservation de la représentation LLVM |

Une combinaison non supportée est rejetée par `config_support_message` avec
`BACKEND_E_UNSUPPORTED_TARGET`; elle n'atteint donc ni l'émission d'objet ni le
linker. L'option toolchain native, LTO ou PGO est signalée comme inactive
lorsque le backend choisi n'est pas LLVM.

Trois routes sont distinguées :

- LLVM et x86_64 partagent le frontend et le middle-end auto-hébergés ;
- elles divergent uniquement après la validation ABI de l'IR ;
- le bootstrap C17 est une route indépendante et volontairement réduite. Il ne
  prétend pas posséder de MIR ni de passes MIR.

## Tronc commun auto-hébergé

| # | Entrée → sortie | Phase / frontière | Implémentation réelle |
| ---: | --- | --- | --- |
| 0 | source Vitte → source normalisée | source / `RawSourceToNormalizedSource` | `frontend/input.vit::normalize_input` |
| 1 | source normalisée → source expansée | source / `NormalizedSourceToExpandedSource` | `frontend/macros/expand.vit::expand_macros` |
| 2 | source expansée → tokens | lexer / `SourceToLexer` | `frontend/lexer/scanner.vit::scan_tokens_from_file` |
| 3 | tokens → AST parsé | parser / `LexerToParser` | `frontend/parse/parser.vit::parse_source` |
| 4 | AST parsé → AST validé | AST / `ParserToAst` | `frontend/ast/validate.vit::validate_ast_root` |
| 5 | AST validé → HIR | HIR / `AstToHir` | `middle/hir/lower_ast.vit::lower_ast_to_hir` |
| 6 | HIR → HIR validé | HIR / `HirToValidatedHir` | `middle/hir/validate.vit::validate_hir_contract` |
| 7 | HIR validé → HIR résolu | sema / `HirToSema` | `analysis/sema/resolver.vit::run_sema_hir` |
| 8 | HIR résolu → HIR typé | typeck / `SemaToTypeck` | `analysis/typeck/api.vit::run_production_typeck_hir` |
| 9 | HIR typé → MIR | MIR / `TypedHirToMir` | `middle/lower/hir_to_mir.vit::lower_hir_to_mir` |
| 10 | MIR → MIR validé | MIR / `MirToValidatedMir` | `middle/mir/validate.vit::validate_mir_contract` |
| 11 | MIR validé → MIR optimisé | passes MIR / `MirToPasses` | `middle/passes/pass_manager.vit::run_passes` |
| 12 | MIR optimisé → IR | IR / `PassesToIr` | `middle/lower/mir_to_ir.vit::lower_mir_to_ir_for_target` |
| 13 | IR → IR vérifié | IR / `IrToVerifiedIr` | `backend/ir/ir.vit::verify_unit_contract` |
| 14 | IR vérifié → IR prêt pour l'ABI | ABI / `IrToAbi` | `backend/boundary_contracts.vit::abi_contract_validate` |

Le frontend exécute concrètement normalisation, expansion des macros, lexing,
parsing puis validation AST dans `frontend/pipeline.vit`. Les résultats de
contrat du frontend, de l'analyse, du HIR, du MIR, des passes, de l'IR et de
l'ABI sont propagés par `middle/pipeline.vit` et `backend/pipeline.vit`. Un
contrat invalide interdit l'entrée dans l'étape suivante.

### Artefacts intermédiaires auto-hébergés

Les répertoires de sortie conceptuels du compilateur sont :

| Phase | Artefact | Consommateur |
| --- | --- | --- |
| frontend | tokens, AST validé | lowering HIR |
| analyse | HIR résolu et typé | lowering MIR |
| middle-end | MIR validé puis optimisé | lowering IR |
| backend commun | IR vérifié et ABI-ready | LLVM ou x86_64 |
| production native | `.ll`, `.s`, objet natif | bridge natif puis linker |
| résultat | exécutable, objet ou bibliothèque selon `packaging` | driver / utilisateur |

Les contrôles de contrat (`frontend_contract`, `analysis_contract`,
`hir_contract`, `mir_contract`, `passes_contract`, `ir_contract`,
`abi_contract`, `backend_contract`, `link_contract`) sont des portes
bloquantes. Une phase peut donc produire un rapport invalide, mais la phase
suivante n'est pas appelée.

## Route LLVM

La route LLVM ajoute quatre étapes au tronc commun :

| # | Entrée → sortie | Implémentation / outil |
| ---: | --- | --- |
| 15 | IR prêt ABI → LLVM IR | `backend/codegen/mod.vit::run_codegen_llvm_with_profile` |
| 16 | LLVM IR → objet natif | `backend/native_bridge.vit::emit_native_object_from_llvm_ir`, commande `clang -Wno-override-module -c ...ll -o ...` |
| 17 | objet natif → `target/release/vitte` | `backend/native_bridge.vit::link_native_executable`, outil de lien natif configuré (`cc` sur l'hôte, `clang` en cross-compilation) |
| 18 | release → release vérifiée | `tools/bootstrap_real/bootstrap_real.py::validate_candidate` |

Flux complet :

`source → lexer → parser → AST → HIR → sema → typeck → MIR → passes → IR → ABI → LLVM IR → objet → link → release`

Le codegen LLVM passe par `llvm_bindings` pour construire l'IR LLVM et par
`native_toolchain.vit` pour planifier les arguments de compilation. En mode
hôte, le contexte de cible est vide et `clang` retargete la représentation vers
la machine courante ; en cross-compilation, le bridge ajoute `-target` et,
si fourni, `--sysroot`.

## Route x86_64 native

La route x86_64 consomme exactement le même IR prêt ABI, puis utilise le backend
natif :

| # | Entrée → sortie | Implémentation / outil |
| ---: | --- | --- |
| 15 | IR prêt ABI → assembleur x86_64 | `backend/codegen/mod.vit::run_codegen_x86_64_with_profile` |
| 16 | assembleur x86_64 → objet natif | `backend/native_bridge.vit::emit_native_object_from_assembly`, `clang -x assembler -c ...s -o ...` |
| 17 | objet natif → `target/release/vitte` | `backend/native_bridge.vit::link_native_executable`, outil de lien natif configuré (`cc` sur l'hôte, `clang` en cross-compilation) |
| 18 | release → release vérifiée | `tools/bootstrap_real/bootstrap_real.py::validate_candidate` |

Flux complet :

`source → lexer → parser → AST → HIR → sema → typeck → MIR → passes → IR → ABI → assembleur x86_64 → objet → link → release`

Le backend x86_64 produit un texte assembleur marqué `elf64-relocatable`.
Le bridge exige un symbole d'entrée, assemble avec `clang`, puis vérifie
l'objet natif avant d'autoriser le link. Le chemin x86_64 n'utilise pas les
bindings LLVM et n'active pas LTO/PGO.

`self_hosted_backend_routes_are_separated` vérifie que LLVM et x86_64 ont les
15 mêmes étapes amont et qu'ils divergent précisément à `AbiToBackend`.

## Route bootstrap C17 stricte

Cette route est implémentée dans `bootstrap/src`. C'est le compilateur seed, pas
le backend C du compilateur auto-hébergé.

Elle se construit séparément avec `make bootstrap-c17`, qui délègue à
`bootstrap/Makefile` et produit `target/bootstrap-c17/vitte-bootstrap`. Ce
binaire construit ensuite le compilateur source en `target/stage1/vitte` lors
du gate stage1 ; il ne sélectionne jamais la route LLVM ou x86_64
auto-hébergée.

| # | Entrée → sortie | Implémentation réelle |
| ---: | --- | --- |
| 0 | source Vitte → tokens bootstrap | `bootstrap/src/lexer/lexer.c::vitte_lexer_lex_all` |
| 1 | tokens → AST bootstrap | `bootstrap/src/parser/parser.c::vitte_parser_parse_module` |
| 2 | AST → AST validé | `bootstrap/src/ast/ast.c::vitte_ast_validate` |
| 3 | AST validé → AST résolu | `bootstrap/src/sema/sema.c::vitte_sema_analyze` |
| 4 | AST résolu → AST typé | `bootstrap/src/sema/sema.c::vitte_sema_analyze` |
| 5 | AST typé → HIR bootstrap | `bootstrap/src/hir/hir.c::vitte_hir_lower_ast` |
| 6 | HIR → HIR validé | `bootstrap/src/hir/hir.c::vitte_hir_validate` |
| 7 | HIR validé → IR bootstrap | `bootstrap/src/ir/ir.c::vitte_ir_lower_hir` |
| 8 | IR → IR/ABI validé | `bootstrap/src/ir/ir.c::vitte_ir_validate` |
| 9 | IR validé → unité de traduction C17 | `bootstrap/src/backend/c17/backend.c::vitte_c17_backend_emit_ir_to_file` |
| 10 | C17 → objet natif non conservé | `bootstrap/src/driver/driver.c::vitte_driver_compile_c` |
| 11 | objet → `target/stage1/vitte` | même invocation hôte de `vitte_driver_compile_c` |
| 12 | stage1 → stage1 vérifié | `tools/bootstrap_real/bootstrap_real.py::validate_candidate` |

Flux complet :

`source → lexer C17 → parser C17 → AST → sema/typeck → HIR bootstrap → IR bootstrap → ABI → C17 → objet+link fusionnés → stage1 vérifié`

Les étapes objet et link représentent deux frontières contractuelles, mais une
seule invocation du compilateur C hôte (`cc` configuré par le bootstrap) ;
aucun objet intermédiaire durable n'est inventé. Cette invocation reste en C17
strict avec `-std=c17 -Wall -Wextra -Werror -pedantic`.

`c17_bootstrap_route_is_mir_free` protège l'invariant suivant : le bootstrap
C17 abaisse directement son HIR vers son IR. Ajouter artificiellement MIR ou
des passes MIR à cette carte rend le contrat canonique invalide.

### Ce qui n'est pas la route C17

`src/vitte/compiler/backend/c` est un backend C auto-hébergé distinct, avec
son propre lowering MIR→C et ses profils de cible. Il ne fait pas partie du
bootstrap C17 sous `bootstrap/src/backend/c17` et ne doit pas être utilisé pour
conclure que le seed possède un pipeline MIR ou un backend Vitte natif.

### Commandes de construction et de vérification

```text
make bootstrap-c17
make bootstrap-c17-smoke
make c17-separation-stability
make compiler-real-pipeline-audit
```

`make bootstrap-c17` compile tous les `.c` du seed avec les options C17
strictes et écrit `target/bootstrap-c17/vitte-bootstrap`. Le smoke test exerce
la commande `check`, les imports et plusieurs builds de programmes Vitte.
`c17-separation-stability` vérifie notamment que les marqueurs C17 ne fuient
pas dans les modules Vitte auto-hébergés. L'audit du pipeline réel contrôle la
reachability des phases, indépendamment de la carte déclarative.

## Frontières d'échec

Chaque transition est une frontière bloquante. Une sortie invalide ne peut pas
être consommée par la phase suivante : AST invalide vers HIR, HIR invalide vers
sema/typeck, MIR invalide vers les passes, IR invalide vers l'ABI, ABI invalide
vers le backend, backend invalide vers l'objet, ou link invalide vers la
release. Les violations sont transportées par `ContractResult` et adaptées aux
diagnostics canoniques sans coupler les représentations IR à leur rendu.

Le backend C auto-hébergé situé sous `src/vitte/compiler/backend/c` reste une
fonctionnalité séparée. Il n'est ni supprimé ni confondu avec la chaîne de
confiance du bootstrap C17 décrite ici.
