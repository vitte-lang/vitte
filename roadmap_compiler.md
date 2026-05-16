# Roadmap Compiler Vitte - Etapes Pro (Activation Reelle de Tous les Fichiers)

Date: 2026-05-16
Portee: `src/vitte/compiler/**`
But: passer d’une arborescence code a un compilateur enterprise, exploitable en production, avec chaque fichier effectivement execute, teste, observable et gouverne.

---

## 1) Definition stricte: fichier effectif
Un fichier `*.vit` est effectif uniquement si:
1. il est exporte par son `mod.vit` parent,
2. il est importe par une couche amont reelle,
3. il est appele par un chemin runtime depuis `driver/pipeline.vit`,
4. il a au moins 1 test nominal,
5. il a au moins 1 test d’erreur,
6. il emet un signal observable (diag/log/metric/trace),
7. il est couvert par CI,
8. son contrat est versionne (breaking change detecte).

Gate obligatoire: PR refusee si un fichier actif perd une condition.

---

## 2) Mode projet pro (gouvernance)
- Branching: feature branches courtes + merge gate.
- RFC obligatoire pour changement de contrat inter-couches.
- Policy deprecation: 2 releases de transition mini.
- Quality gate: tests, perf, diag snapshots, compat metadata/IR.
- Release train: hebdo (canary), mensuel (stable).

---

## 3) Tronc runtime (chemin d’execution unique)
Chemin cible obligatoire:
`driver/cli -> driver/commands -> driver/pipeline -> frontend -> analysis -> middle -> backend -> link -> exit_status`

Action immediate:
- tracer chaque etape avec `phase_start/phase_end`, duree, status, diag_count.
- etablir un objet `CompileReport` consolide en fin de run.

---

## 4) Etapes PRO par vague (avec livrables)

## Vague A - Fondation d’activation (Semaine 1)
### Cible
Rendre impossible la presence de fichiers morts.

### Fichiers
- `mod.vit`, `prelude.vit`, `version.vit`
- `driver/*`

### Travaux
- Manifest d’activation global (liste des modules attendus).
- Verification de wiring au demarrage compilateur.
- Exit codes contractuels par phase.

### Livrables
- `CompileReport` JSON.
- Tableau runtime “module active/inactive”.

### Acceptation
- Un run compile affiche toutes les couches et detecte tout module non branche.

---

## Vague B - Frontend operationnel (Semaines 2-3)
### Fichiers
- `frontend/mod.vit`, `pipeline.vit`, `input.vit`, `source_map.vit`
- `frontend/lexer/{mod,token,cursor,scanner,literals,comments,errors}.vit`
- `frontend/parse/{mod,parser,grammar,recovery,precedence,lookahead,errors}.vit`
- `frontend/ast/{mod,node,item,expr,stmt,pattern,type_expr,visitor,pretty,validate}.vit`
- `frontend/macros/{mod,expand,hygiene,builtin,errors}.vit`

### Travaux
- Brancher tous les sous-modules dans `frontend/pipeline.vit`.
- Recovery parser robuste (limiter diagnostics en cascade).
- Validation AST bloquante avant analysis.
- Macros: expansion/hygiene traceables par span.

### Acceptation
- `tests/lexer_tests.vit`, `parser_tests.vit`, `ast_tests.vit` passent.
- 0 panic sur jeux invalides de base.

---

## Vague C - Analysis decisionnelle (Semaines 4-5)
### Fichiers
- `analysis/mod.vit`, `analysis/pipeline.vit`
- `analysis/sema/*`
- `analysis/typeck/*`
- `analysis/borrowck/*`
- `analysis/const_eval/*`
- `analysis/lint/*`

### Travaux
- Orchestration stricte sema -> typeck -> borrowck -> const_eval -> lint.
- Tous les `errors.vit` utilises dans chemins d’echec reels.
- Contrat de diagnostics par phase (code, severity, labels, suggestion).

### Acceptation
- `tests/typeck_tests.vit`, `borrowck_tests.vit` verts.
- Couverture erreurs critiques (top 30 codes).

---

## Vague D - Middle industriel (Semaines 6-7)
### Fichiers
- `middle/mod.vit`, `middle/pipeline.vit`
- `middle/hir/*`, `middle/mir/*`, `middle/lower/*`, `middle/passes/*`, `middle/optimizations/*`

### Travaux
- Lowering explicite AST->HIR->MIR->IR.
- Pass manager declaratif + ordre versionne.
- Optimisations: stats pass (changed/no-op, duree).

### Acceptation
- `tests/hir_tests.vit`, `mir_tests.vit` verts.
- Aucun IR invalide sans blocage par validate/verify.

---

## Vague E - Backend & artefacts (Semaines 8-9)
### Fichiers
- `backend/mod.vit`, `backend/pipeline.vit`
- `backend/ir/*`, `backend/codegen/*`, `backend/target/*`, `backend/link/*`

### Travaux
- Verification IR obligatoire avant codegen.
- Instruction select/register alloc executes par cible.
- Link final reproductible.

### Acceptation
- `tests/codegen_tests.vit` vert.
- Generation binaire minimale valide sur cible prioritaire (`x86_64`).

---

## Vague F - Infrastructure transverse (Semaines 10-11)
### Fichiers
- `infrastructure/diagnostics/*`
- `infrastructure/diagnostics/fluent/*`
- `infrastructure/errors/*`
- `infrastructure/query/*`
- `infrastructure/incremental/*`
- `infrastructure/session/*`
- `infrastructure/source/*`
- `infrastructure/span/*`
- `infrastructure/symbols/*`
- `infrastructure/metadata/*`

### Travaux
- Diagnostics unifies pour toutes phases.
- Fluent complet: parser FTL + loader fichier + fallback + emitter.
- Query/incremental: au moins 1 query critique en cache.
- Session: options/locale/configs reelles.

### Acceptation
- Changement locale impacte messages sans rebuild compilateur.
- Re-run incrmental montre gain mesurable.

---

## Vague G - Interface externe (Semaine 12)
### Fichiers
- `interface/mod.vit`, `api.vit`, `compiler_host.vit`, `callbacks.vit`, `bridge.vit`

### Travaux
- API stable compile/check/diag.
- Contrat embedding IDE/service.

### Acceptation
- Client externe execute une compile end-to-end via API.

---

## 5) Plan d’activation fichier-par-fichier (procedure standard)
Pour chaque fichier `X.vit`:
1. Declarer contrat (inputs/outputs/errors).
2. Exporter dans `mod.vit` parent.
3. Wiring runtime (appel effectif).
4. Ajouter test nominal.
5. Ajouter test erreur.
6. Ajouter instrumentation (duree + compteur).
7. Ajouter snapshot diag si applicable.
8. Ajouter rule CI (presence test + execution path).
9. Documenter ownership du fichier.

Etat possible: `stub` -> `wired` -> `validated` -> `production`.

---

## 6) Checklist complete par zone

## 6.1 Driver
- `driver/cli.vit`: parse flags, locale, mode strict.
- `driver/commands.vit`: commandes compile/check/lint.
- `driver/pipeline.vit`: orchestration canonique.
- `driver/compile.vit`: execution compile.
- `driver/exit_status.vit`: mapping erreurs.

## 6.2 Frontend
- Lexer: tous tokens critiques couverts.
- Parser: precedence/lookahead/recovery utilises.
- AST: validate bloque invalides.
- Macros: hygiene activee.

## 6.3 Analysis
- Sema: imports/symboles/visibility effectifs.
- Typeck: infer/unify/coercion/traits executes.
- Borrowck: loans/moves/lifetimes/regions executes.
- Const_eval: evaluator + interpreter utilises.
- Lint: rules + levels + report actifs.

## 6.4 Middle
- HIR builder/lower/validate actifs.
- MIR builder/dataflow/transform/validate actifs.
- Pass manager + schedule executes.
- Optimisations: simplify_cfg/dead_code/inline/const_prop/copy_prop executes.

## 6.5 Backend
- IR verify actif.
- Codegen emitter/machine/regalloc/isel/object actifs.
- Targets x86_64/aarch64/riscv64 resolus via triple/features/layout.
- Linker/artifact/symbols actifs.

## 6.6 Infrastructure
- Diagnostics: labels/suggestions/colors/emitter actifs.
- Fluent: `types/catalog/parser/loader/lookup/format/bundle/fallback` actifs.
- Errors: code/severity/registry actifs.
- Query+incremental actifs.
- Session/source/span/symbols/metadata actifs.

## 6.7 Tests
- `tests/*` executes en CI + snapshots versionnes.

---

## 7) KPI PRO (gates)
- Build success rate >= 99% sur suite interne.
- Regression perf compile <= 5% max/release.
- P95 compile time suivi par taille projet.
- 100% fichiers actifs associes a au moins 1 test.
- 100% diagnostics top-severity localisables (Fluent).
- 0 dependency cycle entre couches.

---

## 8) Risques et mitigations
- Risque: fichiers vides/stubs jamais relies.
  - Mitigation: manifest d’activation + gate CI bloquant.
- Risque: explosion diagnostics parser.
  - Mitigation: budget de recovery et cap par fichier.
- Risque: regressions optimisations.
  - Mitigation: differential tests pre/post passes.
- Risque: i18n incoherente.
  - Mitigation: lint FTL + fallback obligatoire + keys coverage.

---

## 9) CI/CD pro (minimum)
Pipelines:
1. `lint-structure`: verifie wiring `mod.vit` + imports + cycles.
2. `unit`: tests par couche.
3. `integration`: compile end-to-end.
4. `golden-diagnostics`: snapshots.
5. `perf-smoke`: temps/memoire baseline.
6. `release-compat`: metadata/IR version checks.

Fail-fast:
- module inactif detecte
- test absent pour fichier actif
- diagnostic code non enregistre

---

## 10) Backlog immediat (ordre d’execution)
1. Finaliser `driver/pipeline.vit` avec `CompileReport` complet.
2. Brancher lecture disque Fluent `.ftl` via infra source/files.
3. Rendre `frontend/pipeline.vit` executable bout-en-bout.
4. Activer `analysis/pipeline.vit` avec erreurs standardisees.
5. Activer `middle/passes/pass_manager.vit` + stats passes.
6. Activer `backend/pipeline.vit` jusqu’a `linker.vit`.
7. Completer matrice tests `tests/*` par fichier actif.
8. Ajouter gate CI “fichier effectif obligatoire”.

---

## 11) Definition of Done finale (compiler enterprise)
Le compiler est “pro” quand:
- toutes les couches compilent et s’executent en chainage complet,
- chaque fichier de la cible roadmap est effectif,
- diagnostics sont localises avec fallback,
- performances et stabilite sont sous SLO,
- CI empeche les regressions structurelles.
