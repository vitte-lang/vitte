# vitte(1) — Manuel de référence

> Version générée à partir de `crates/vitte-cli/src/main.rs` et `crates/vitte-cli/src/lib.rs`

## NOM
`vitte` — interface en ligne de commande unifiée du langage **Vitte** (compilation, exécution, formatage et inspection du bytecode VITBC).

## SYNOPSIS
```
vitte [OPTIONS GLOBALES] <commande> [sous-options] [arguments]
vitte [OPTIONS GLOBALES] fichier.vitte
vitte [OPTIONS GLOBALES] fichier.vitbc
```

La seconde forme laisse `vitte` déduire automatiquement la commande en fonction de l’extension :
- `*.vitte`, `*.vit`, `*.vt` → alias de `vitte compile`
- `*.vitbc`, `*.vitb` → alias de `vitte run`

## DESCRIPTION GÉNÉRALE
`vitte` est conçu comme un outil multi-usage piloté par sous-commandes. Chaque sous-commande active un pipeline spécifique (compilation, VM, formateur, inspection…). Le comportement concret dépend des **features** activées au moment de la compilation du binaire :

- `vm` : fournit les hooks de compilation/VM par défaut (bytecode VBC0 stub).
- `fmt` : active le formateur officiel.
- `modules` : permet de lister les modules intégrés.
- `trace` : connecte le logger `env_logger`.
- `color` : colore les diagnostics sur la sortie d’erreur.

Les builds distribués activent toutes ces features par défaut.

## OPTIONS GLOBALES
- `-v`, `--verbose` (répétable) : augmente la verbosité (`warn` → `info` → `debug` → `trace`).  
  Active uniquement lorsque la feature `trace` est compilée (`crates/vitte-cli/src/main.rs:29`).
- `-q`, `--quiet` : supprime les messages info/trace et force le niveau `error`.
- `--color <auto|always|never>` : contrôle l’émission des couleurs ANSI (`ColorChoice`, `crates/vitte-cli/src/main.rs:36`).
- `--version` : affiche la version.
- `-h`, `--help` : affiche l’aide globale ou celle d’une sous-commande.

En mode couleur, `vitte` ajuste automatiquement `CLICOLOR_FORCE` / `NO_COLOR` (`crates/vitte-cli/src/main.rs:357`). Le niveau de log est exporté via `RUST_LOG` lorsque `trace` est actif (`crates/vitte-cli/src/main.rs:344`).

## RACCOURCIS & DÉTECTIONS
- Alias intégrés : `vitte build …` ⇨ `vitte compile …`, `vitte dsasm …` ⇨ `vitte disasm …` (`crates/vitte-cli/src/main.rs:397`).
- Lorsque seul un fichier est fourni, la commande est inférée (voir **SYNOPSIS**).
- Les arguments placés après `--` avec `vitte run` sont transférés tels quels au programme invité.

## COMMANDES

### compile
```
vitte compile [OPTIONS] [SOURCE]
```
- **Entrées acceptées** : fichier `.vitte`/`.vit`/`.vt`, ou `-` (stdin). L’absence d’argument lit depuis `stdin` (`crates/vitte-cli/src.lib.rs:256`).
- **Options détaillées** :
  - `-o`, `--output <FICHIER>` ⇒ sortie explicite (`-` ⇒ `stdout`). Sans option, `default_bytecode_path` renomme la source (`crates/vitte-cli/src.lib.rs:263-268`, `:1191-1195`).
  - `--auto` ⇒ force la stratégie « même nom + .vitbc », y compris si `--output` est présent.
  - `-O`, `--optimize` ⇒ active `CompileOptions { optimize: true }` (`crates/vitte-cli/src.lib.rs:260`).
  - `--debug` ⇒ renseigne `emit_debug` (selon le hook).
  - `--mkdir` / `--overwrite` ⇒ contrôle création et écrasement (`crates/vitte-cli/src.lib.rs:276-283`).
  - `--time` ⇒ affiche `TIME compile: … ms` (`crates/vitte-cli/src.lib.rs:292-294`).

**Sorties**
- `stdout` (`-o -`) avec flush explicite (`crates/vitte-cli/src.lib.rs:271-274`).
- Fichier `.vitbc` écrit de façon atomique (`write_bytes_atomic`, `crates/vitte-cli/src.lib.rs:284`, `:492-517`).
- `out.vitbc` lorsque la source provient de `stdin` avec `--auto` (`crates/vitte-cli/src.lib.rs:264-266`).

Résultat standard : `COMPILE <chemin> (<taille> octets)` (`crates/vitte-cli/src.lib.rs:286`).

**Remarque hook** : sans hook `compile`, diagnostic `HOOK001`.

**Points de vigilance**
- `--auto` n’a d’effet que pour `compile` ; sans lui, `stdin` ⇒ `stdout`.
- `--mkdir`/`--overwrite` sécurisent la manipulation des fichiers.
- Les écritures atomiques évitent les fichiers partiels.
- Diagnostics fréquents : `HOOK001`, conflit d’écriture sans `--overwrite`.

### run
```
vitte run [OPTIONS] [PROGRAM] [-- <ARGS>...]
```
- **Entrées acceptées** : bytecode `.vitbc` (chemin) ou source lorsque `--auto-compile` est activé (`crates/vitte-cli/src.lib.rs:309-346`). `-` signifie « fichier nommé - » côté bytecode ; stdin n’est supporté qu’en mode auto-compile.
- **Options détaillées** :
  - `--auto-compile` ⇒ compile la source en mémoire avant exécution (`Hooks::compile`).
  - `-O`, `--optimize` ⇒ passe `optimize=true` au compilateur auto (`crates/vitte-cli/src.lib.rs:328`).
  - `--time` ⇒ mesure l’exécution et affiche `TIME run: … ms` (`crates/vitte-cli/src.lib.rs:366-368`).
  - `--` séparateur ⇒ tout ce qui suit est transféré dans `RunOptions::args` (`crates/vitte-cli/src.lib.rs:344`, `:191`).
- **Sorties** : `RUN exit=<code>` (succès) ou `RUN exit=N` (warning) (`crates/vitte-cli/src.lib.rs:369-372`). Le code de retour du programme est propagé (`Ok(code)`).
- **Flux d’exécution** :
  1. Lecture du bytecode ou compilation à la volée (`compiler(&src, …)`).
  2. Invocation du hook `run_bc` (VM) via `Hooks::run_bc`.
  3. Affichage optionnel du temps et normalisation du code de sortie (`crates/vitte-cli/src.lib.rs:362-373`).
- **Diagnostics & limitations** :
  - `HOOK002` si la VM n’est pas fournie (`crates/vitte-cli/src.lib.rs:303`).
  - `HOOK001` si `--auto-compile` est demandé sans hook de compilation (`crates/vitte-cli/src.lib.rs:320-344`).
  - `RUN100` / `RUN101` lorsque l’entrée n’est pas un bytecode et que `--auto-compile` manque (`crates/vitte-cli/src.lib.rs:348-357`).
  - Pas de lecture directe de bytecode via stdin pour l’instant (design volontaire).
### repl
```
vitte repl [--prompt <TEXTE>]
```
- **Comportement par défaut** : `cli::repl::fallback` ouvre une boucle ligne par ligne avec historique en mémoire et commandes `:help` / `:quit` (`crates/vitte-cli/src.lib.rs:1334-1366`).
- **Option** :
  - `--prompt <TEXTE>` ⇒ personnalise l’invite (défaut `vitte> `).
- **Sortie** : le code de retour correspond à la valeur renvoyée par le hook (`Result<i32>`). Par défaut, `0` si l’utilisateur quitte proprement.
- **Hooks personnalisés** :
  - `Hooks::repl` peut être remplacé pour brancher un REPL avancé (complétion, scripting, etc.).
  - Signature attendue : `fn(&str) -> Result<i32>` (`crates/vitte-cli/src.lib.rs:174-175`).
- **Limitations** : le fallback ne connaît pas le langage, il se contente d’afficher l’entrée (mode “echo”).
### fmt
```
vitte fmt [OPTIONS] [SOURCE]
```
- **Entrées acceptées** : fichier unique ou `-` (stdin). Pour un répertoire, script externe recommandé (`find`, `git ls-files`, etc.).
- **Options détaillées** :
  - `-o`, `--output <FICHIER>` ⇒ destination explicite (`-` ⇒ `stdout`).
  - `--in-place` ⇒ réécrit la source (impossible avec stdin).
  - `--check` ⇒ ne modifie pas les fichiers, échec si diff.
- **Pipeline** :
  1. Lecture du contenu (`read_source`).
  2. Invocation du hook `fmt` (`FormatFn`), qui renvoie le texte formaté (`crates/vitte-cli/src.lib.rs:377-389`).
  3. Écriture (stdout, fichier, in-place) via `write_text_atomic` (`crates/vitte-cli/src.lib.rs:392-404`).
  4. Message `FMT …` et code de sortie éventuel (`crates/vitte-cli/src.lib.rs:409-422`).
- **Sorties** :
  - `FMT écrit -> chemin` lorsque le fichier est réécrit.
  - `FMT check OK (…)` en mode `--check`.
  - `Err(formatting diff)` si un diff est détecté et qu’on est en `--check`.
- **Diagnostics & cas particuliers** :
  - `HOOK003` si le formateur n’est pas disponible.
  - `--in-place` + stdin ⇒ erreur dédiée.
  - `Output::Auto` n’est pas autorisé (n’a pas de sens pour `fmt`).
- **Personnalisation** : un formateur maison peut être branché via `Hooks::fmt`, par exemple pour injecter un style alterné ou un formatteur rapide basé sur AST.

### inspect
```
vitte inspect [OPTIONS] [BYTECODE]
```
- **Entrées acceptées** : fichier `.vitbc`, `-` (stdin) ou flux pipe. `read_stdin_bytes` impose au moins un octet (`INS102`, `crates/vitte-cli/src.lib.rs:465-477`).
- **Sections activables** (toutes cumulables) :
  - `--summary`, `--header`, `--sections`, `--size` → méta-informations générales.
  - `--symbols`, `--consts`, `--imports`, `--exports`, `--deps`, `--entry` → aspects symboliques (stub dans la version actuelle).
  - `--strings`, `--hexdump`, `--disasm` → analyse du payload.
  - `--target`, `--meta`, `--debug`, `--verify` → informations additionnelles (BLAKE3, debug, intégrité).
  - `--dump-all` → active tous les drapeaux ci-dessus (`InspectOptions::ensure_defaults`).
  - `--json` → sérialise via `InspectionReport` (`serde_json::to_string_pretty`, `crates/vitte-cli/src.lib.rs:980-1000`).
- **Sorties** :
  - Texte multi-sections (ordre déterminé par les options).
  - JSON structuré avec clés `format`, `size`, `sections`, `strings`, etc.
- **Fonctionnement interne** :
  1. Lecture du bytecode (`fs::read` ou stdin).
  2. Analyse via `inspect::analyze` (format VBC0 vs inconnu) (`crates/vitte-cli/src.lib.rs:708-748`).
  3. Rendu texte (`render_summary`, `render_sections`, …) ou JSON (`render_json`).
- **Diagnostics & limites** :
  - `HOOK004` si aucun inspecteur n’est branché.
  - `INS100` lorsque l’entrée est une source.
  - `INS102` si stdin est vide.
  - Formats inconnus ⇒ message “Format non reconnu — inspection limitée.”
- **Astuce** : combiner `--dump-all --json` et `jq` pour filtrer des sections, ou `--verify` pour valider l’intégrité BLAKE3 dans des pipelines CI.
- **Extension** : remplacer `Hooks::inspect` pour supporter d’autres formats (ex : future version du bytecode, metadata signées).

### disasm
```
vitte disasm [OPTIONS] [BYTECODE]
```
- **Entrées acceptées** : fichier `.vitbc` ou chemin arbitraire. Le désassembleur actuel ne lit pas le bytecode depuis stdin (diagnostic `DIS101`).
- **Options** :
  - `-o`, `--output <FICHIER>` ⇒ redirige le texte généré (`stdout` par défaut).
- **Sorties** :
  - Pseudo-désassemblage VBC0 en colonnes : offset, opcode (`OP_XX`), opérandes hex (`crates/vitte-cli/src.lib.rs:1070-1078`).
  - Message `DISASM ok` sur `stderr`.
- **Diagnostics & limites** :
  - `HOOK005` si aucun hook `disasm` n’est disponible.
  - `DIS100` / `DIS101` pour les entrées invalides (source ou stdin).
  - `Output::Auto`/`InPlace` non pris en charge.
- **Personnalisation** : `Hooks::disasm` peut être relié à un véritable désassembleur (ex : génération d’assembleur humainement lisible, mapping symbolique).
- **Tuyauterie** : le désassembleur lit entièrement le fichier en mémoire via `fs::read` avant traitement (`crates/vitte-cli/src.lib.rs:1053`).

### modules
```
vitte modules [--json]
```
- `--json` : rend la liste des modules intégrés au format JSON.

Sans option, affiche un tableau texte. Si la feature `modules` a été désactivée lors de la compilation du binaire, la commande échoue avec `HOOK006` (`crates/vitte-cli/src/lib.rs:1091`).

**Diagnostics & limites**
- `HOOK006` : feature `modules` désactivée (`crates/vitte-cli/src/lib.rs:1089-1094`).
- Lorsque la liste est vide, un message explicite indique comment builder `vitte-modules` avec les features souhaitées (`crates/vitte-cli/src/lib.rs:1104-1109`).
- Le rendu JSON est proche de `serde_json::to_string_pretty` ; il peut être exploité via `jq` (`crates/vitte-cli/src/lib.rs:1119-1129`).

## MATRICE DES FEATURES & HOOKS

| Feature Cargo | Activée par défaut | Hooks ajoutés | Commandes concernées | Diagnostic affiché si absente |
|---------------|-------------------|---------------|----------------------|--------------------------------|
| `vm`          | ✅                 | `compile`, `run_bc`, `disasm` stubs VBC0 (`crates/vitte-cli/src/main.rs:295-323`) | `run`, `compile --auto`, `disasm` | `HOOK001`, `HOOK002`, `HOOK005` |
| `fmt`         | ✅                 | `fmt` via `vitte_fmt::format_source` (`crates/vitte-cli/src/main.rs:317-320`) | `fmt` | `HOOK003` |
| `modules`     | ✅ (builds officiels) | Accès à `vitte_modules::modules()` (`crates/vitte-cli/src/lib.rs:1099-1129`) | `modules` | `HOOK006` |
| `trace`       | ✅                 | Initialisation `env_logger` (`crates/vitte-cli/src/lib.rs:197-205`) | toutes (verbosité, `--time`) | Aucun (messages plus succints) |
| `color`       | ✅                 | Couleurs ANSI (`crates/vitte-cli/src/lib.rs:1204-1312`) | diagnostics | Aucun |

Construire sans ces features désactive les hooks correspondants. Les diagnostics `HOOK00x` guident l’utilisateur pour réactiver la feature manquante (`crates/vitte-cli/src/lib.rs:247-1094`).

## MESSAGES & CODES DE RETOUR
- `COMPILE`, `RUN`, `FMT`, `DISASM` : tags de statut envoyés sur `stderr` (colorés si possible).
- `TIME` : chronométrages optionnels (`--time`).
- Code de sortie :
  - `0` : succès ou programme invité ayant retourné 0.
  - `>0` : erreur CLI, absence de hook, fichier introuvable, diff `--check`, programme invité non nul, etc.

En cas d’échec critique (`anyhow`), le message détaillé est affiché sur `stderr` (`crates/vitte-cli/src/main.rs:370`).

### Diagnostics connus

| Code  | Commande(s) | Résolution suggérée | Référence |
|-------|-------------|---------------------|-----------|
| `HOOK001` | `compile`, `run --auto-compile` | Recompiler avec la feature `vm` / brancher `Hooks::compile` | `crates/vitte-cli/src/lib.rs:247`, `:320` |
| `HOOK002` | `run` | Fournir une VM (`vm` feature) | `crates/vitte-cli/src/lib.rs:303` |
| `HOOK003` | `fmt` | Activer la feature `fmt` ou fournir un formateur | `crates/vitte-cli/src/lib.rs:382` |
| `HOOK004` | `inspect` | Activer `engine`/`vm` ou brancher un inspecteur | `crates/vitte-cli/src/lib.rs:432` |
| `HOOK005` | `disasm` | Activer la feature `vm`/`disasm` | `crates/vitte-cli/src/lib.rs:1047` |
| `HOOK006` | `modules` | Recompiler avec `--features modules` | `crates/vitte-cli/src/lib.rs:1089` |
| `RUN100` / `RUN101` | `run` | Fournir un bytecode ou `--auto-compile` | `crates/vitte-cli/src/lib.rs:348-357` |
| `INS100` / `INS102` | `inspect` | Fournir un bytecode non vide | `crates/vitte-cli/src/lib.rs:448`, `:473` |
| `DIS100` / `DIS101` | `disasm` | Fournir un bytecode fichier | `crates/vitte-cli/src/lib.rs:1060-1066` |

Chaque diagnostic est formaté via `emit_diagnostic`, avec couleurs si la feature `color` est active (`crates/vitte-cli/src/lib.rs:1280-1312`).

## VARIABLES D’ENVIRONNEMENT
- `NO_COLOR` / `CLICOLOR_FORCE` : peuvent être définies avant l’exécution ; `vitte` les ajuste selon `--color`.
- `RUST_LOG` : lu/écrit pour contrôler le logger (si feature `trace`).
- `VITTE_LOG` (projeté) : les hooks internes peuvent se baser sur les mêmes conventions que `env_logger`.

## HOOKS ET EXTENSIBILITÉ
Les sous-commandes délèguent leur cœur de logique à des hooks (cf. `Hooks`, `crates/vitte-cli/src/lib.rs:154`). En build personnalisée :

- Remplacer `Hooks::compile` pour brancher un compilateur réel.
- Injecter une VM complète via `Hooks::run_bc`.
- Fournir un formateur, un désassembleur ou un inspecteur spécialisés.
- Étendre `ModulesTask` pour interroger d’autres registres.
- Les signatures attendues (`CompileFn`, `RunFn`, `FormatFn`, etc.) sont déclarées dans `crates/vitte-cli/src/lib.rs:170-181` et retournent des `Result`.
- Le module `cli::registry` fournit un chargeur d’index JSON de modules additionnels (`crates/vitte-cli/src/lib.rs:1004-1038`).

Sans hook correspondant, la commande échoue et affiche un diagnostic explicite (`HOOK00x`).

## ENTRÉES / SORTIES & CONVENTIONS
- `cli::Input` / `InputKind` décrivent les sources possibles (chemin, stdin, buffer) (`crates/vitte-cli/src/lib.rs:118-138`).
- `cli::Output` gère les destinations (`stdout`, fichier, écriture in-place, auto) (`crates/vitte-cli/src/lib.rs:140-152`).
- Les écritures utilisent des fichiers temporaires renommés pour éviter la corruption (`crates/vitte-cli/src/lib.rs:492-517`).
- `default_bytecode_path` reprend le nom du fichier source en remplaçant l’extension (`crates/vitte-cli/src/lib.rs:1191-1195`).
- `status_ok/info/warn` standardisent les logs CLI (couleurs si disponibles) (`crates/vitte-cli/src/lib.rs:1204-1289`).

## EXEMPLES
```
# Compiler depuis stdin vers stdout
cat main.vitte | vitte compile -o - > main.vitbc

# Exécuter en compilant automatiquement
vitte run --auto-compile examples/hello.vitte -- --message "Bonjour"

# Formater un fichier en place et vérifier en CI
vitte fmt --in-place src/main.vitte
vitte fmt --check src/main.vitte

# Inspecter un bytecode complet en JSON
vitte inspect --dump-all --json build/app.vitbc

# Désassembler un artefact vers un fichier texte
vitte disasm build/app.vitbc -o build/app.asm

# Obtenir la liste des modules intégrés
vitte modules
vitte modules --json

# Inspecter rapidement un fichier reçu
vitte inspect --summary --hexdump build/app.vitbc

# Vérifier l’intégrité d’un bytecode via un pipe
curl -sSf https://example.org/app.vitbc | vitte inspect --verify --summary

# Exploiter la sortie JSON des modules
vitte modules --json | jq '.[] | select(.tags[]? == "experimental")'
```

## VOIR AUSSI
- `docs/CLI/guide.md` pour un tutoriel plus narratif.
- Les tests snapshot `crates/vitte-cli/tests/cli_syntax.rs` pour des exemples d’entrées/sorties.
- Le crate `vitte-cli` (`lib.rs`) pour l’API embeddable.
