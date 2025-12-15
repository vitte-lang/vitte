# toolchains/

Descripteurs toolchains (clang/gcc/msvc) + flags par target (standard C23).

# sdk/toolchains (max)

Ce dossier contient les **descripteurs de toolchains** et les **politiques de flags** utilisées par `muffin` et `vittec` pour construire :
- runtime/VM (C23)
- outils (CLI)
- plugins/dylibs

L’objectif est d’avoir un modèle :
- **déterministe** (mêmes inputs → mêmes flags)
- **portable** (macOS/Linux/Windows/*BSD)
- **extensible** (nouveaux compilos/targets sans casser l’existant)

Standard C (contrat)
- Le standard de base pour le runtime/outillage C est **C23**.
- Flags recommandés :
  - GCC : `-std=c23`
  - Clang : `-std=c2x` (fallback) ou `-std=c23` si supporté par la version
  - MSVC : pas de flag strict équivalent ; documenter le niveau C supporté et activer les warnings.

---

## 1) Contenu attendu

Ce répertoire doit contenir :

- `toolchains.schema.json` : schéma JSON des descripteurs
- `toolchains.json` : registre de toolchains connues (system/llvm/msvc/mingw…)
- `policies/` : politiques de flags (warning levels, hardening, sanitizers)
- `snippets/` : fragments réutilisables (ex: `common_warnings.json`)

Exemple d’arborescence :

```
toolchains/
  README.md
  toolchains.schema.json
  toolchains.json
  policies/
    warnings.json
    hardening.json
    sanitizers.json
    lto.json
  snippets/
    c23_base.json
    common_warnings_clang.json
    common_warnings_gcc.json
    common_warnings_msvc.json
```

---

## 2) Modèle : toolchain descriptor

Un descripteur toolchain définit :
- comment détecter un compilateur
- comment invoquer compile/link/archive
- quels flags appliquer par défaut

Champs recommandés :
- `name` : string (id stable) — ex: `system`, `llvm`, `msvc`, `mingw`
- `family` : string — ex: `clang`, `gcc`, `msvc`
- `detect` : objet (cmds de détection)
- `commands` : objet (cc/ld/ar/ranlib)
- `defaults` : objet (cflags/ldflags/defines)
- `features` : caps (asan/ubsan/tsan/lto/thinlto)

---

## 3) Format JSON recommandé

### 3.1 toolchains.json

Exemple minimal :

```json
{
  "toolchains": [
    {
      "name": "system",
      "family": "clang",
      "detect": { "cc": ["cc", "clang"] },
      "commands": { "cc": "cc", "ld": "cc", "ar": "ar", "ranlib": "ranlib" },
      "defaults": {
        "cflags": ["-std=c23"],
        "ldflags": [],
        "defines": ["VITTE_BUILD"]
      },
      "features": { "asan": true, "ubsan": true, "tsan": true, "lto": true, "thinlto": true }
    }
  ]
}
```

### 3.2 policies/*.json

- `warnings.json` : niveaux de warnings selon family (clang/gcc/msvc)
- `hardening.json` : stack protector, fortify, relro/now, etc.
- `sanitizers.json` : asan/ubsan/tsan flags compile/link
- `lto.json` : flags LTO (thin/full)

---

## 4) Résolution (muffin/vittec)

Ordre de sélection recommandé :

1) toolchain explicitement choisie dans MUF (ex: `target.toolchain = "msvc"`) (future)
2) `workspace.toolchain` (future)
3) variable d’environnement (ex: `VITTE_TOOLCHAIN`)
4) fallback `system`

Une fois la toolchain choisie, les flags finaux proviennent de :

- flags **target** (dans `sysroot/share/vitte/targets/*.json`)
- + defaults toolchain
- + policies activées par profile (warnings/hardening/sanitizers/lto)

Règles :
- concat stable (ordre fixe)
- déduplication stable (optionnelle)
- aucune “auto magie” silencieuse : toute policy activée doit être traçable

---

## 5) Liens avec la spec

Références :
- `spec/targets/flags.md` : base des flags par OS/toolchain
- `spec/targets/triples.md` : matrice des triples
- `spec/muf/muf_semantics.md` : target/profile/toolchain

---

## 6) Notes

- Les toolchains sont des **inputs** : toute modification change les artefacts.
- En release, publier les toolchains + policies dans le SDK pour reproduction.

---

## 7) Migration C99 → C23

- Remplacer les defaults `-std=c99` par `-std=c23` (ou `-std=c2x` côté clang si nécessaire).
- Garder une policy de compat si un target impose un standard plus bas (ex: toolchain ancienne).