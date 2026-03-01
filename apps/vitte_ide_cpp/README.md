# vitte-ide (C++)

IDE TUI style Geany pour projets Vitte, multi-fichiers, orienté flux rapide: edit/build/debug/diagnostics.

## Fonctions principales

- Onglets multi-fichiers réels
  - ouverture de plusieurs fichiers
  - navigation onglets (`Ctrl+Tab` selon terminal, `[` / `]` fallback)
  - marqueur `*` sur onglet non sauvegardé
- Split view
  - split on/off (`v`)
  - orientation verticale/horizontale (`h`)
  - pane actif (`t`)
  - follow symbol dans l'autre split (`f`)
- Projet + recherche globale
  - find in files (`/`) via `rg`
  - replace in files avec preview + confirmation (`%`)
  - palette rapide fuzzy live (`Ctrl+P`) pour fichiers + `@symbol` + `:line` + `#diag`
- Outline/symboles
  - extraction `proc/form/entry/trait`
  - jump via panneau diag/outline (`Ctrl+R`)
- Navigation cross-file
  - go to definition (`F12` ou `g`)
  - historique back/forward (`H` / `L`)
  - find references cross-file (`U`)
  - rename symbol projet avec preview diff (`N`)
- Diagnostics multi-fichiers
  - check courant (`c`)
  - diagnostics projet (`D`)
  - open file+line par `Enter` sur diagnostic
  - panneau Problems filtrable (`P`: all/current/errors)
- Build targets type Geany
  - `check/build/run/test` (`c`/`b`/`x`/`u`)
  - config cible dans `.vitte-cache/vitte-ide/targets.conf`
  - args/env/profile configurable
- Session projet
  - restore/save auto fichiers ouverts, curseurs, split, breakpoints
- Auto-save + file watcher
  - auto-save périodique
  - reload intelligent sur modif externe (prompt)
- Suggestions intelligentes multi-fichiers (`s`)
  - index global symboles
  - ranking par imports `use` + proximité dossier + contexte

## Build

```bash
make vitte-ide
```

Binaire:

```bash
bin/vitte-ide
```

## Run

```bash
bin/vitte-ide /chemin/projet
# ou
bin/vitte-ide
```

## Raccourcis

- `Tab`: changer de pane (`FILES` / `CODE` / `DIAG`)
- `Ctrl+Tab` / `Ctrl+Shift+Tab`: onglet suivant/précédent (selon terminal)
- `[` / `]`: onglet précédent/suivant (fallback)
- `X`: fermer onglet actif
- `i`: mode insertion (`Esc` pour sortir)
- `w`: save buffer actif
- `k`: toggle breakpoint ligne courante
- `v`: split on/off
- `h`: orientation split V/H
- `t`: basculer pane actif du split
- `f`: follow symbol dans autre split
- `/`: find in files
- `%`: replace in files (preview + confirm)
- `Ctrl+P`: quick palette live (fichiers / `@symbol` / `:line` / `#diag`)
- `Ctrl+R`: outline fichier courant
- `F12` / `g`: go to definition
- `H` / `L`: back / forward
- `U`: find references cross-file
- `N`: rename symbol projet (preview + confirm)
- `c`: check
- `b`: build
- `x`: run
- `u`: test
- `F5/F6/F7/F8`: run/check/build/test
- `D`: diagnostics projet
- `P`: cycle filtre Problems (all/current/errors)
- `T`: éditer config targets
- `a`: auto-check on/off
- `q`: quitter

## Build targets

Fichier: `.vitte-cache/vitte-ide/targets.conf`

Exemple:

```ini
name=default
check=vitte check {file}
build=vitte build {file} -o {out}
run={out}
test=
profile=debug
args=
env=
```

Placeholders:

- `{file}` chemin absolu du fichier actif
- `{rel}` chemin relatif projet
- `{out}` binaire de sortie (`.vitte-cache/vitte-ide/<stem>_ide.out`)
- `{profile}` profil cible
- Minimap + breadcrumbs
  - minimap texte dans chaque pane code
  - breadcrumbs fichier > symbole courant
