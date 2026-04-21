# Contrat compilateur <-> stdlib

Ce document définit le contrat stable entre le compilateur Vitte et la surface runtime de la stdlib.

Terminologie utilisée dans la doc et la CLI :
- « Surface de liaison Vitte » remplace « libc surface ».
- « liaison native » remplace « interop C ».

## 1. Source de vérité

- Fichier de surface ABI versionné : `docs/book/stdlib_abi_surface_v1.txt` (nom historique).
- Version ABI suivante : `docs/book/stdlib_abi_surface_v2.txt` (nom historique).
- Racine source de la stdlib : arborescence source dédiée.
- En-tête ABI runtime : `src/compiler/backends/runtime/vitte_runtime.hpp`.

## 2. Profils stdlib stables

`--runtime-profile` contrôle quels modules stdlib sont autorisés.
`--stdlib-profile` est conservé comme alias historique.

- `core` (historique `minimal`) : uniquement `core/**`.
- `desktop` (historique `full`) : tous les modules stdlib.
- `system` (historique `kernel`) : `core/**` + `kernel/**`.
- `arduino` : `core/**` + `arduino/**`.

Si un import n'est pas autorisé, le compilateur émet `E1010`.
Si un module stdlib est manquant, le compilateur émet `E1014`.

Niveaux de module :
- `public` : surface d'import stable.
- `internal` : espace de noms privé (`*/internal/*`), bloqué hors propriétaire (`E1016`).
- `experimental` : activation uniquement avec `--allow-experimental` (`E1015`).

## 3. Strictness des imports

`--strict-imports` (alias `--strict-bridge`) impose l'hygiène des imports :

- alias explicite requis sur `use`/`pull` (`E1011`).
- les alias explicites doivent être utilisés (`E1012`).
- les chemins relatifs non canoniques sont rejetés (`E1013`).

## 4. Frontière d'erreurs liaison runtime/native

- Couche parse : `E000x`.
- Couche resolve : `E100x` (inclut les contraintes stdlib/import).
- Couche IR : `E200x`.
- Couche backend/chaîne d'outils : `E300x`.

## 5. Outils

- Exporter la carte des symboles stdlib :
  - `vitte check --dump-stdlib-map <file.vit>`
- Exporter l'index complet des modules :
  - `vitte check --dump-module-index <file.vit>`
- Graphe et doctor :
  - `vitte mod graph <file.vit>`
  - `vitte mod doctor <file.vit>`
- Vérifier la surface ABI par rapport à la source stdlib :
  - `tools/lint_stdlib_api.py`
- Diff de compatibilité ABI :
  - `tools/check_stdlib_abi_compat.py`
- Instantanés de profil :
  - `tools/stdlib_profile_snapshots.sh`

## 6. Nom de la surface de liaison

Pour la documentation et les exemples de liaison système/native, préférer les noms orientés stdlib :
- `print`
- `read`
- `path`
- `env`
- `bytes`
- `alloc`
- `process`

Plan de modules bridge dans la doc :
- `bridge/io`
- `bridge/memory`
- `bridge/system`
- `bridge/time`
- `bridge/net`
