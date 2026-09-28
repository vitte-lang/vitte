# Vitte Roadmap 161-200 (Version Pro)

## Cadre de pilotage

Portee: finaliser la transition de Vitte depuis un compilateur avance vers une plateforme langage complete, sans sacrifier la fiabilite.

Principe directeur: toute complexite ajoutee doit rester strictement inferieure a la confiance systeme qu'elle apporte.

Definition of done globale:
- impact fonctionnel measurable
- diagnostics lisibles et actionnables
- tests de non-regression et de robustesse
- documentation technique synchronisee
- risque residuel explicite et accepte

## Streams strategiques

### Stream A - Type System, Inference, Generics, Traits (161-165, 168)

**161. Advanced Type Inference**
- Objectif: gagner en ergonomie sans perte de surete statique.
- Livrables:
  - inference des variables locales
  - inference des retours simples
  - propagation coherente des contraintes
  - moteur d'unification unique
  - diagnostics d'ambiguite precis
  - regles de fallback explicites
- Acceptation:
  - pas de regression de soundness sur la suite typing
  - diagnostics ambiguite avec suggestion concrete

**162. Generics Foundation**
- Objectif: introduire des generiques controles et predictibles.
- Livrables:
  - parametres generiques
  - substitution de types
  - monomorphisation minimale
  - contraintes simples (bornes de base)
  - diagnostics generiques clairs
- Acceptation:
  - exemples standards generiques compilent
  - erreurs de contraintes localisees avec spans fiables

**163. Generic Monomorphization**
- Objectif: generer des instances specialisees sans duplication inutile.
- Livrables:
  - cache d'instances
  - cles de specialisation deterministes
  - elimination des doublons
  - specialisation MIR
  - specialisation backend
- Acceptation:
  - build deterministe pour memes entrees
  - reduction des doublons verifiee par metriques

**164. Trait / Interface Foundation**
- Objectif: fournir une abstraction typee stable.
- Livrables:
  - definitions de traits/interfaces
  - resolution de methodes
  - verification des impl
  - diagnostics de recouvrement
  - bornes de traits minimales
- Acceptation:
  - coherence des impl garantie
  - ambiguite de resolution signalee proprement

**165. Dynamic Dispatch Support**
- Objectif: supporter le polymorphisme runtime de facon sure.
- Livrables:
  - vtables
  - fat pointers
  - dispatch dynamique runtime
  - verification object safety
  - ABI stable pour types dyn
- Acceptation:
  - appels dyn valides sur cibles supportees
  - rejet explicite des cas non object-safe

**168. Const Generics Foundation**
- Objectif: supporter des parametres constants dans les types.
- Livrables:
  - parametres const
  - integration eval const
  - tableaux generiques
  - diagnostics overflow
- Acceptation:
  - evaluation const deterministe
  - erreurs overflow reproductibles et comprehensibles

### Stream B - Pattern Semantics & Exhaustiveness (166-167)

**166. Advanced Pattern Matching**
- Objectif: rendre le `match` robuste et complet.
- Livrables:
  - verification d'exhaustivite
  - detection de branches inatteignables
  - patterns imbriques
  - guards (si actives)
  - diagnostics orientes correction
- Acceptation:
  - aucun faux negatif critique sur exhaustivite
  - branches mortes detectees avec precision

**167. Exhaustiveness Engine**
- Objectif: garantir la couverture totale des variantes.
- Livrables:
  - arbre de decision
  - couverture de variantes
  - analyse wildcard
  - detection unreachable
- Acceptation:
  - moteur reutilisable pour `match`/`select`
  - complexite maitrisee sur gros enums

### Stream C - Macro & Frontend Pipeline (169-170)

**169. Macro System Foundation**
- Objectif: ajouter de la metaprogrammation sans fragiliser le langage.
- Livrables:
  - macros hygieniques
  - expansion de tokens
  - limites de recursion
  - diagnostics avec spans macro
  - trace d'expansion
- Acceptation:
  - hygiene validee sur cas de capture de noms
  - recursion stoppee avec message actionnable

**170. Macro Expansion Pipeline**
- Objectif: integrer l'expansion dans le frontend de maniere stable.
- Pipeline cible:
  - source -> lexer -> macro expand -> parser -> AST
- Verifications:
  - spans corrects de bout en bout
  - diagnostics preserves apres expansion
  - protection recursion safe

### Stream D - Async, Coroutines, Concurrency (171-174)

**171. Async Foundation**
- Objectif: etablir les primitives async.
- Livrables:
  - fonctions async
  - lowering vers futures
  - lowering `await`
  - support MIR async
  - diagnostics misuse async

**172. Coroutine Lowering**
- Objectif: convertir async en machine a etats fiable.
- Livrables:
  - MIR de generateur
  - points de suspension/reprise
  - captures de locals
  - surete des drops

**173. Concurrency Memory Model**
- Objectif: formaliser le comportement memoire concurrent.
- A documenter:
  - atomics
  - synchronisation
  - visibilite inter-threads
  - regles d'ordering
  - garanties data-race

**174. Parallel Borrow Analysis**
- Objectif: rendre borrow checking compatible parallele.
- Livrables:
  - equivalents `Send`/`Sync`
  - checks mutation partagee
  - regles d'ownership thread
  - diagnostics concurrence

### Stream E - Backends, ABI, Perf, Execution Modes (175-181)

**175. Advanced Optimization Passes**
- inline, simplification de boucles, branch folding, scalar replacement, dead store elimination, value numbering.
- Acceptation: gains mesurables sur benchs de reference sans regression semantique.

**176. LLVM Backend Experimental**
- emission LLVM IR, metadata debug, pont d'optimisation, lowering cible, integration linker.
- Acceptation: flag experimental isole + parity fonctionnelle minimale.

**177. Native ASM Backend**
- allocation registres, convention d'appel, layout stack, selection d'instructions, prologue/epilogue.
- Acceptation: subset cible stable + tests ABI verts.

**178. Register Allocator**
- liveness analysis, spills, classes de registres, diagnostics.
- Acceptation: correction prioritaire avant performance.

**179. Low-level ABI Test Suite**
- alignement stack, convention d'appel, passage structs, retours, varargs si supporte.
- Acceptation: matrice ABI par cible avec rapport explicite.

**180. JIT Foundation**
- interpreteur MIR, backend JIT experimental, loader runtime, resolveur de symboles.

**181. MIR Interpreter**
- execution ops MIR, modele memoire minimal, debug step-by-step, reutilisation const eval.

### Stream F - Tooling Platform (182-185)

**182. Compiler Embedding API**
- `compile_from_memory`, callbacks diagnostics, source loader custom, hooks backend, incremental embedding.

**183. IDE Integration Complete**
- semantic highlighting, diagnostics incrementaux, completion intelligente, rename, references, code actions.

**184. Refactor Engine**
- rename symbole, extract function, organize imports, inline variable, rewrites sures.

**185. Advanced Formatter**
- styles configurables, commentaires stables, formatting macro, determinisme, edition-aware.

### Stream G - Analysis, Safety, Security (186-189)

**186. Static Analysis Framework**
- nullability (si supportee), misuse lifetime/unsafe, logique inatteignable, smells performance.

**187. Security Analysis Passes**
- detection overflows, patterns memoire dangereux, casts suspects, groundwork taint, warnings API insecurisees.

**188. Unsafe Code Model**
- blocs/fonctions/traits unsafe, diagnostics explicites, tracking unsafe dans MIR.

**189. Verified Unsafe Boundaries**
- logs d'audit unsafe, resumes unsafe, call graph unsafe, warnings de propagation.

### Stream H - Runtime, Interop, Targets, Research, Formalization, Platform (190-200)

**190. Advanced Runtime Services**
- allocateur, threading, timers, scheduler async, panic hooks, abstraction I/O.

**191. GC Experimental Mode**
- runtime GC optionnel, bridge ownership/GC, diagnostics de compatibilite, metriques perf.

**192. Foreign Function Interface**
- declarations extern, verification ABI, linkage symboles, frontieres unsafe FFI, diagnostics mismatch.

**193. Bindgen Foundation**
- parse minimal headers C, generation declarations Vitte, checks ABI, validation layout.

**194. Embedded Target Support**
- `no_std`, allocateurs custom, runtime freestanding, scripts linker, memory maps.

**195. Kernel Mode Foundation**
- mode sans runtime, panic handlers custom, subset interrupt-safe, subset kernel memory-safe.

**196. Compiler Research Sandbox**
- branches experimentales, passes MIR isolees, namespace syntaxe instable, diagnostics sandbox.

**197. Verified Compiler Pipeline**
- preuves partielles d'invariants, passes verifies, property tests, checks equivalence semantique.

**198. Formal Semantic Model**
- semantique operationnelle, semantique ownership, groundwork soundness des types, semantique execution MIR.

**199. Vitte Language Platform**
- compiler, stdlib, runtime, package manager, tooling IDE/CI, docs, ecosystem.

**200. Vitte Platform Complete Foundation**
- Criteres obligatoires:
  - compilateur stable
  - self-host credible
  - backend reel
  - diagnostics professionnels
  - tooling complet
  - stdlib maintenable
  - runtime maintenable
  - ecosysteme package minimal
  - support IDE
  - builds deterministes
  - replay/debug/fuzz/stress actifs
  - documentation technique forte
  - securite minimale verifiee
  - architecture extensible
  - gouvernance technique definie
- Gate finale:
  - toute feature doit justifier son cout en complexite par un gain net de fiabilite mesurable.

## Sequencement recommande (macro-priorites)

P0 (fiabilite coeur): 161, 162, 164, 166, 167, 173, 188
P1 (capacites langage): 163, 165, 168, 169, 170, 171, 172, 174
P2 (execution/perf): 175, 178, 179, 181, puis 176/177/180
P3 (plateforme): 182, 183, 184, 185, 186, 187, 189
P4 (expansion): 190 a 195
P5 (confiance long terme): 196, 197, 198, 199, 200

## KPIs de suivi

- soundness regressions: 0 tolere
- deterministic build rate: 100%
- crash-free compile sessions (corpus CI): > 99.9%
- diagnostic actionable rate (triage interne): > 95%
- perf compile (median): tendance stable ou meilleure par release
- unsafe surface growth: suivie, justifiee, et bornee

## Risques critiques a surveiller

- explosion de complexite inference/generics
- derive de coherence trait/impl
- fragilite spans/diagnostics via macros
- regressions subtiles en lowering async/coroutines
- divergences ABI entre backends
- extension unsafe plus rapide que les garde-fous

## Regle absolue de gouvernance

La complexite ajoutee doit toujours etre inferieure a la confiance gagnee dans la fiabilite du systeme.
