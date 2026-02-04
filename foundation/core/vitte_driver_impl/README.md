# vitte_driver_impl

Implémentation **concrète** du driver du compilateur **Vitte**.

Ce crate contient **toute la logique d’exécution réelle** du pipeline de compilation :
- cache incrémental
- exécution des étapes
- invocation des linkers
- exécution runtime
- logging interne

👉 Il est volontairement **séparé** de `vitte_driver`, qui expose l’API publique et les politiques.

---

## Positionnement dans l’architecture


Le découpage permet :
- une API stable
- des implémentations remplaçables
- un bootstrap progressif (stage0 → stageN)

---

## Responsabilités

`vitte_driver_impl` est responsable de :

- exécuter le **pipeline réel**
- intégrer le **cache incrémental**
- appeler les **linkers** (lld, futur system)
- exécuter les binaires générés (`run`)
- fournir un **logging interne**
- rester **déterministe et bootstrap-friendly**

Il **ne décide pas** :
- du backend à utiliser
- des options de compilation
- des politiques de warnings

Ces décisions relèvent de `vitte_driver`.

---


---

## Pipeline d’exécution

Le pipeline concret suit strictement l’ordre :

1. Parse
2. Lowering
3. Type checking
4. Monomorphisation
5. MIR
6. SSA
7. Constant evaluation
8. Optimisation (si activée)
9. Codegen (avec cache incrémental)
10. Linking
11. Run (optionnel)

Voir :
- `impl/pipeline/compile.vit`
- `impl/pipeline/link.vit`

---

## Cache incrémental

Implémenté dans :

impl/cache/incremental.vit

Caractéristiques :
- fingerprint basé sur le contenu
- index disque stable (`index.vitte`)
- invalidation fine par dépendances
- sûr pour CI / IDE / bootstrap

---

## Linkers supportés

Actuellement :
- **LLD** (ELF / COFF / Mach-O)

Implémentation :
impl/link/lld.vit


Prévu :
- linker système (`cc`, `ld`)
- WASM (`wasm-ld`)
- linkers custom

---

## Runtime

L’exécution des binaires générés est gérée par :

impl/runtime/run.vit


Fonctionnalités :
- arguments
- environnement
- cwd
- dry-run
- intégration diagnostics

---

## Logging

Logging interne minimaliste, sans dépendances externes :

utils/log.vit


- niveaux : error / warn / info / debug / trace
- couleurs ANSI optionnelles
- initialisé depuis `DriverOptions`

---

## Tests

Test de fumée bout-en-bout :

tests/driver_impl_smoke.vit


Ce test valide :
- le wiring global
- le pipeline concret
- la génération d’artefacts
- l’exécution runtime (optionnelle)

---

## Utilisation typique

Ce crate n’est **pas destiné à être utilisé directement**.

Utilisation normale :

```vit
pull vitte/driver

vitte_driver délègue ensuite automatiquement à vitte_driver_impl.