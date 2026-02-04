# vitte_target — Backend de résolution de cible

Le backend **`vitte_target`** est responsable de la **détermination complète et cohérente de la cible de compilation** dans le compilateur Vitte.

Il constitue la **source de vérité** pour :
- l’architecture (x86_64, arm64, wasm…),
- le système (Linux, Windows, macOS, WASI…),
- l’ABI (SysV, Win64, WASM…),
- le format objet (ELF, COFF, Mach-O),
- les capacités associées (stack, pointeurs, threading, etc.).

Ce backend est consommé par :
- le **driver** (`vittec`),
- le **symbol mangling**,
- les **backends de codegen** (LLVM, Cranelift),
- les **runtimes** (alloc, sanitizers),
- le **linking**.

---




👉 `vitte_target` **ne génère pas de code**.  
Il décrit **où et comment** le code sera généré.

---

## Responsabilités

- Résolution de la cible **native** (host)
- Parsing de **target triples**
- Support de cibles **custom**
- Normalisation des informations de target
- Validation de cohérence (arch / OS / ABI)
- Fourniture d’un **contexte cible stable**

---




## Schémas de résolution (`schemes/`)

Les **schémas** définissent *comment* une cible est obtenue.

### `native`
- Détection de la machine hôte
- Utilisé par défaut
- Exemple : `x86_64-linux-gnu`

### `triple`
- Parsing explicite d’un triple
- Utilisé via `--target <triple>`
- Exemple :


### `custom`
- Cible entièrement définie par configuration
- Utilisée pour :
- embedded
- expérimentation
- bootstrap avancé

---

## API publique (vue d’ensemble)

Exemple côté driver :

```vit
pull compiler/backends/vitte_target

let target = vitte_target::resolve_from_cli(opts)
target.validate()
