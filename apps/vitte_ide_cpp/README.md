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
  - palette rapide fuzzy live (`Ctrl+P`) pour fichiers + `@symbol` + `:line` + `#diag` + actions `>>`
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
  - pane actif/focus et layout split restaurés
- Auto-save + file watcher
  - auto-save périodique
  - reload intelligent sur modif externe (prompt)
- Démarrage instantané
  - scan fichiers progressif en arrière-plan (non bloquant)
  - index symboles progressif
  - UI utilisable immédiatement
- Layout adaptatif
  - fenêtre TUI qui s’adapte à la taille terminal/machine
- Suggestions intelligentes multi-fichiers (`s`)
  - index global symboles
  - ranking par imports `use` + proximité dossier + contexte
- Mini-LSP local
  - hover symbole (`K`) avec kind/signature locale
- Multi-cursor minimal
  - sélectionner occurrences symbole (`Ctrl+D`)
  - appliquer remplacement groupé (`Ctrl+E`)
- Undo persistant (branche simple)
  - undo (`z`) / redo (`Y`)
  - historique stocké sous `.vitte-cache/vitte-ide/undo/`
- Preview markdown/docs
  - preview `README.md` ou chemin custom (`O`)
- Complétion inline (`Ctrl+Space`)
  - popup de suggestions au curseur
  - insertion directe dans le buffer
- Recherche locale rapide (`Ctrl+F`, `n`, `N`)
  - recherche dans fichier courant
  - navigation match suivant/précédent
- Goto ligne/colonne (`Ctrl+G`)
- Save rapide
  - buffer actif (`Ctrl+S`)
  - dirty buffers (`Ctrl+W`, fallback terminal)
- Édition rapide
  - déplacer ligne (`Alt+Up/Alt+Down`)
  - toggle commentaire ligne (`Ctrl+/`)
  - rename local fichier courant (`F2`)

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
# mode démarrage rapide:
bin/vitte-ide --light
# flags ciblés:
bin/vitte-ide --no-autocheck --no-session
# forcer profil Geany dans targets.conf:
bin/vitte-ide --geany-defaults
# diagnostic utilisateur rapide:
bin/vitte-ide --safe-mode
# profiling UI:
bin/vitte-ide --profile-ui
```

## Raccourcis

- `Tab`: changer de pane (`FILES` / `CODE` / `DIAG`)
- `Ctrl+Tab` / `Ctrl+Shift+Tab`: onglet suivant/précédent (selon terminal)
- `[` / `]`: onglet précédent/suivant (fallback)
- `X`: fermer onglet actif
- `i`: mode insertion (`Esc` pour sortir)
- `w` / `Ctrl+S`: save buffer actif
- `Ctrl+W`: save dirty buffers
- `k`: toggle breakpoint ligne courante
- `v`: split on/off
- `h`: orientation split V/H
- `t`: basculer pane actif du split
- `f`: follow symbol dans autre split
- `Alt+Up` / `Alt+Down`: déplacer la ligne courante
- `Ctrl+/`: commenter/décommenter ligne
- `/`: find in files
- `%`: replace in files (preview + confirm)
- `Ctrl+F`: find in current file
- `n` / `N`: match suivant / précédent (find current file)
- `Ctrl+G`: goto `line[:col]`
- `Ctrl+Space`: popup complétion inline
- `Ctrl+P`: quick palette live (fichiers / `@symbol` / `:line` / `#diag`)
  - `>>check`, `>>build`, `>>run`, `>>save`, `>>hover`, `>>docs preview`, etc.
- `Ctrl+R`: outline fichier courant
- `K`: mini-LSP hover local
- `Ctrl+D`: multi-cursor select symbole
- `Ctrl+E`: multi-cursor apply replace
- `z` / `Y`: undo / redo
- `F12` / `g`: go to definition
- `H` / `L`: back / forward
- `U`: find references cross-file
- `F2`: rename symbol local (fichier courant)
- `R`: rename symbol projet (preview + confirm)
- `c`: check
- `b`: build
- `x`: run
- `u`: test
- `F4` / `Shift+F4`: next / prev diagnostic
- `F5/F6/F7/F8`: run/check/build/test
- `D`: diagnostics projet
- `J`: export diagnostics JSON (`.vitte-cache/vitte-ide/diagnostics.json`)
- `O`: preview markdown/docs dans le panneau bas
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
run=vitte run {file}
test=vitte test {project}
profile=debug
args=
env=
```

Placeholders:

- `{file}` chemin absolu du fichier actif
- `{rel}` chemin relatif projet
- `{project}` racine du projet
- `{out}` binaire de sortie (`.vitte-cache/vitte-ide/<stem>_ide.out`)
- `{profile}` profil cible
- Minimap + breadcrumbs
  - minimap texte dans chaque pane code
  - breadcrumbs fichier > symbole courant
