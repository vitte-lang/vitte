# 26. Projet guide Éditeur

Niveau: Avancé.

Prérequis: chapitre précédent `docs/book/chapters/25-projet-arduino.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/25-projet-arduino.md`, `docs/book/chapters/27-grammaire.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.


Ce chapitre construit un moteur d'éditeur exploitable en contexte réel: commandes enrichies, undo/redo exact, invariants explicites, résultat typé, historique cohérent, observabilité et extension maîtrisée.

Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Input -> Validate -> Apply -> Record History -> Project.
- Entrée: état courant + commande enrichie.
- Traitement: garde, mutation atomique, mise à jour d'historique.
- Sortie: nouvel état cohérent + vue.
- Invariant: même entrée + même état => même sortie.

## 26.0 Contrat éditorial minimal

Contrat retenu:
- `Document` est une liste ordonnée de lignes.
- `Cursor` reste toujours borné sur le document.
- Toute commande mutante est atomique (tout ou rien).
- `undo/redo` repose sur des commandes assez riches pour inversion exacte.
- Toute nouvelle commande annule `future`.

Invariants:
- `row` dans `[0, max_row]`.
- `col` dans `[0, len(line[row])]`.
- `past` contient l'historique exécuté; `future` contient uniquement des commandes annulées.
- Après une nouvelle mutation valide: `future = []`.

## 26.1 Modèle canonique et `EditResult`

Frontière: ce bloc ne connaît pas le terminal; il modélise uniquement l'état et les transitions.

```vit
form Cursor {
  row: int
  col: int
}

form Document {
  lines: string[]
}

form EditorState {
  doc: Document
  cursor: Cursor
  read_only: bool
}

pick EditorError {
  case ErrReadOnly
  case ErrInvalid
}

pick EditResult {
  case Ok(state: EditorState)
  case Err(error: EditorError)
}

pick Command {
  case Left
  case Right
  case InsertChar(ch: string, row: int, col: int)
  case Backspace(row: int, col: int, deleted: string)
  case NewLine(row: int, col: int)
}

form History {
  past: Command[]
  future: Command[]
}
```

Encadré de stabilité:
> Ce modèle est canonique et ne changera plus dans ce chapitre.  
> Toutes les sections suivantes ne font qu'ajouter des règles d'exécution et de vérification.

Pourquoi ce bloc existe:
1. `EditResult` impose un traitement explicite des erreurs métier au lieu d'erreurs implicites.
2. `Backspace(..., deleted)` capture l'information minimale pour garantir un undo exact.
3. `History` rend la trajectoire d'édition vérifiable et donc testable.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: `Err(ErrInvalid)`, sans mutation partielle.

## 26.2 Bornes et normalisation

Frontière: ce bloc impose les contraintes de position avant toute mutation.

```vit
form Bounds {
  max_row: int
  max_col: int
}

proc bounds_from_doc(d: Document) -> Bounds {
  let last_row: int = d.lines.len() - 1
  if last_row < 0 { give Bounds(0, 0) }
  let last_col: int = d.lines[last_row].len()
  give Bounds(last_row, last_col)
}

proc clamp_cursor(c: Cursor, d: Document) -> Cursor {
  let b: Bounds = bounds_from_doc(d)
  let r: int = c.row
  let k: int = c.col
  if r < 0 { set r = 0 }
  if r > b.max_row { set r = b.max_row }
  let line_len: int = d.lines[r].len()
  if k < 0 { set k = 0 }
  if k > line_len { set k = line_len }
  give Cursor(r, k)
}
```

## 26.3 Noyau d'édition multi-lignes

Frontière: ce bloc applique les mutations texte; il ne gère ni historique ni rendu.

```vit
proc insert_at(s: EditorState, ch: string, row: int, col: int) -> EditorState {
  let c: Cursor = clamp_cursor(Cursor(row, col), s.doc)
  let line: string = s.doc.lines[c.row]
  let left: string = line.slice(0, c.col)
  let right: string = line.slice(c.col, line.len())
  s.doc.lines[c.row] = left + ch + right
  give EditorState(s.doc, Cursor(c.row, c.col + 1), s.read_only)
}

proc newline_split(s: EditorState, row: int, col: int) -> EditorState {
  let c: Cursor = clamp_cursor(Cursor(row, col), s.doc)
  let line: string = s.doc.lines[c.row]
  let left: string = line.slice(0, c.col)
  let right: string = line.slice(c.col, line.len())
  s.doc.lines[c.row] = left
  s.doc.lines.insert(c.row + 1, right)
  give EditorState(s.doc, Cursor(c.row + 1, 0), s.read_only)
}

proc backspace_apply(s: EditorState, row: int, col: int) -> EditorState {
  let c: Cursor = clamp_cursor(Cursor(row, col), s.doc)
  if c.col > 0 {
    let line: string = s.doc.lines[c.row]
    let left: string = line.slice(0, c.col - 1)
    let right: string = line.slice(c.col, line.len())
    s.doc.lines[c.row] = left + right
    give EditorState(s.doc, Cursor(c.row, c.col - 1), s.read_only)
  }

  if c.row == 0 { give s }

  let prev: string = s.doc.lines[c.row - 1]
  let cur: string = s.doc.lines[c.row]
  let join_col: int = prev.len()
  s.doc.lines[c.row - 1] = prev + cur
  s.doc.lines.remove_at(c.row)
  give EditorState(s.doc, Cursor(c.row - 1, join_col), s.read_only)
}
```

## 26.4 Backspace exact et inversion formelle

Frontière: ce bloc calcule des inverses exacts; il ne projette pas la vue.

Principe:
- `Backspace(row, col, deleted)` stocke ce qui a été supprimé.
- Si suppression intra-ligne: `deleted` contient le caractère retiré.
- Si fusion de lignes: `deleted` contient `"\n"` pour représenter la coupure supprimée.

```vit
proc inverse_of(cmd: Command) -> Command {
  match cmd {
    case Left { give Right }
    case Right { give Left }
    case InsertChar(ch, row, col) {
      give Backspace(row, col + ch.len(), ch)
    }
    case NewLine(row, col) {
      give Backspace(row + 1, 0, "\n")
    }
    case Backspace(row, col, deleted) {
      if deleted == "\n" {
        give NewLine(row - 1, col)
      }
      give InsertChar(deleted, row, col - deleted.len())
    }
    otherwise { give Left }
  }
}
```

Résultat: l'inversion de `Backspace` ne dépend plus d'un `?` ni d'une heuristique.

Tableau unique des commandes:

| Commande | Précondition | Effet | Inverse |
| --- | --- | --- | --- |
| `Left` | Curseur borné. | Déplace le curseur d'une colonne à gauche (avec clamp). | `Right`. |
| `Right` | Curseur borné. | Déplace le curseur d'une colonne à droite (avec clamp). | `Left`. |
| `InsertChar(ch,row,col)` | `ch` non vide, position normalisée. | Insère `ch` à `(row,col)`, avance le curseur. | `Backspace(row, col + len(ch), ch)`. |
| `Backspace(row,col,deleted)` | Position normalisée, `deleted` conforme à la suppression. | Supprime à gauche ou fusionne deux lignes si `col == 0`. | `InsertChar(deleted, row, col - len(deleted))` ou `NewLine(row - 1, col)` si `deleted == "\\n"`. |
| `NewLine(row,col)` | Position normalisée. | Coupe la ligne en deux et place le curseur en début de ligne suivante. | `Backspace(row + 1, 0, "\\n")`. |

## 26.5 Application typée et atomicité

Frontière: ce bloc valide et applique une commande avec retour explicite.

```vit
proc is_mutating(cmd: Command) -> bool {
  match cmd {
    case InsertChar(_, _, _) { give true }
    case Backspace(_, _, _) { give true }
    case NewLine(_, _) { give true }
    otherwise { give false }
  }
}

proc apply_command(s: EditorState, cmd: Command) -> EditResult {
  if s.doc.lines.len() == 0 {
    give Err(ErrInvalid)
  }

  if s.read_only and is_mutating(cmd) {
    give Err(ErrReadOnly)
  }

  match cmd {
    case Left {
      let c: Cursor = clamp_cursor(Cursor(s.cursor.row, s.cursor.col - 1), s.doc)
      give Ok(EditorState(s.doc, c, s.read_only))
    }
    case Right {
      let c: Cursor = clamp_cursor(Cursor(s.cursor.row, s.cursor.col + 1), s.doc)
      give Ok(EditorState(s.doc, c, s.read_only))
    }
    case InsertChar(ch, row, col) { give Ok(insert_at(s, ch, row, col)) }
    case Backspace(row, col, _) { give Ok(backspace_apply(s, row, col)) }
    case NewLine(row, col) { give Ok(newline_split(s, row, col)) }
    otherwise { give Err(ErrInvalid) }
  }
}
```

Règle d'atomicité:
- `Err(...)` => état initial intact.
- `Ok(state)` => commande entièrement appliquée.

Exemples concrets de refus:
- `ErrInvalid`: `doc.lines = []` puis `apply_command(s, Right)` retourne `Err(ErrInvalid)`.
- `ErrReadOnly`: `read_only = true` puis `apply_command(s, InsertChar("x", 0, 0))` retourne `Err(ErrReadOnly)`.

## 26.6 Undo/Redo complet et cohérence historique

Frontière: ce bloc manipule historique + moteur; il ne fait pas de rendu.

```vit
proc push_history(h: History, cmd: Command) -> History {
  h.past.push(cmd)
  h.future = []
  give h
}

proc apply_with_history(s: EditorState, h: History, cmd: Command) -> EditResult {
  let r: EditResult = apply_command(s, cmd)
  match r {
    case Ok(next) {
      let h2: History = push_history(h, cmd)
      give Ok(next)
    }
    case Err(e) { give Err(e) }
    otherwise { give Err(ErrInvalid) }
  }
}

proc undo_step(s: EditorState, h: History) -> EditResult {
  if h.past.len() == 0 { give Ok(s) }
  let cmd: Command = h.past[h.past.len() - 1]
  h.past.remove_at(h.past.len() - 1)
  let inv: Command = inverse_of(cmd)
  let r: EditResult = apply_command(s, inv)
  match r {
    case Ok(next) {
      h.future.push(cmd)
      give Ok(next)
    }
    case Err(e) { give Err(e) }
    otherwise { give Err(ErrInvalid) }
  }
}

proc redo_step(s: EditorState, h: History) -> EditResult {
  if h.future.len() == 0 { give Ok(s) }
  let cmd: Command = h.future[h.future.len() - 1]
  h.future.remove_at(h.future.len() - 1)
  let r: EditResult = apply_command(s, cmd)
  match r {
    case Ok(next) {
      h.past.push(cmd)
      give Ok(next)
    }
    case Err(e) { give Err(e) }
    otherwise { give Err(ErrInvalid) }
  }
}
```

Invariant explicite de cohérence historique:
- toute nouvelle commande valide vide `future`.
- `undo` déplace exactement une commande de `past` vers `future`.
- `redo` fait le mouvement inverse.

## 26.7 Scénario multi-lignes complet pas à pas

Document initial:
- Ligne 0: `"abc"`.
- Curseur: `(0, 3)`.

Étape 1. `NewLine(0, 3)`:
- Avant: `"abc"`.
- Après: ligne 0 `"abc"`, ligne 1 `""`.
- Curseur: `(1, 0)`.

Étape 2. `InsertChar("x", 1, 0)`:
- Avant: ligne 1 vide.
- Après: ligne 1 `"x"`.
- Curseur: `(1, 1)`.

Étape 3. `Backspace(1, 0, "\n")` (fusion):
- Avant: ligne 0 `"abc"`, ligne 1 `"x"`.
- Après: ligne unique `"abcx"`.
- Curseur: `(0, 3)` (position de jonction).

Étape 4. `undo_step`:
- Inverse de fusion: `NewLine(0, 3)`.
- Retour à deux lignes: `"abc"` et `"x"`.

Étape 5. `redo_step`:
- Ré-applique la fusion.
- Retour stable à `"abcx"`.

Test multi-lignes piège (ligne vide intermédiaire):
- Document initial: ligne 0 `"ab"`, ligne 1 `""`, ligne 2 `"cd"`, curseur `(2, 0)`.
- Commande: `Backspace(2, 0, "\n")`.
- Attendu: ligne 1 devient `"cd"`, ligne 2 est supprimée, curseur sur `(1, 0)`.
- Vérification: aucune donnée perdue malgré la ligne vide intermédiaire.

## 26.8 Test de propriété `undo(redo(s)) == s`

Sous-ensemble de commandes valides:
- commandes issues d'un état cohérent;
- pas de `row/col` hors bornes après normalisation;
- historique compatible (`redo` seulement si `future` non vide).

Pseudo-test:

```vit
proc prop_undo_redo_identity(s: EditorState, h: History) -> bool {
  let a: EditResult = redo_step(s, h)
  match a {
    case Ok(s1) {
      let b: EditResult = undo_step(s1, h)
      match b {
        case Ok(s2) { give hash_state(s2) == hash_state(s) }
        otherwise { give false }
      }
    }
    case Err(_) { give true }
    otherwise { give false }
  }
}
```

Objectif: prouver qu'une séquence `redo` puis `undo` conserve l'état quand elle est applicable.

## 26.9 Replay déterministe long (50+ commandes)

```vit
proc replay(s0: EditorState, cmds: Command[]) -> EditResult {
  let i: int = 0
  let s: EditorState = s0
  loop {
    if i >= cmds.len() { break }
    let r: EditResult = apply_command(s, cmds[i])
    match r {
      case Ok(next) { set s = next }
      case Err(e) { give Err(e) }
      otherwise { give Err(ErrInvalid) }
    }
    set i = i + 1
  }
  give Ok(s)
}
```

Scénario recommandé:
- 60 commandes mélangées (`Left`, `Right`, `InsertChar`, `NewLine`, `Backspace`).
- Vérifier état final, curseur final, `past/future`, puis rejouer exactement la même suite.
- Attendu: état final strictement identique (même hash).

## 26.10 Complexité et migration vers rope/gap buffer

Coûts actuels (tableau de lignes + chaînes):
- `insert_at`: O(L) sur la ligne active.
- `backspace_apply`: O(L) en suppression locale; O(Lprev + Lcur) en fusion.
- `newline_split`: O(L) sur la ligne coupée.

Quand migrer:
- documents très volumineux;
- insertions fréquentes au milieu de lignes longues;
- latence interactive perceptible.

Options:
- `gap buffer`: simple pour édition locale autour du curseur.
- `rope`: meilleur pour gros textes et concaténations/coupes répétées.

Impact API:
- garder `Document` opaque et exposer des primitives (`insert`, `delete`, `split`, `join`).
- préserver les commandes et `EditResult`; seules les fonctions internes changent.

Encadré Compatibilité future:
- ne pas changer la forme publique de `Command`.
- ne pas changer la sémantique de `EditResult`.
- conserver l'invariant `new command => future = []`.
- préserver le comportement de `inverse_of`.
- garder `visible_lines` pure (sans mutation).

## 26.11 Concurrence et threads

État actuel:
- moteur mono-thread, déterministe, sérialisé.

Règle recommandée:
- toutes les commandes passent par une file unique.
- un seul worker applique `Validate -> Apply -> Record`.
- la projection lit des snapshots immuables.

Bénéfice:
- pas de race condition sur `doc/cursor/history`.
- replay et debug restent reproductibles.

## 26.12 Journal d'événements minimal

Événement:

```vit
form EditEvent {
  command: Command
  before_hash: string
  after_hash: string
  ts: int
}
```

Usage:
- audit: comprendre une divergence.
- replay: rejouer une timeline.
- debug: localiser la transition qui casse un invariant.

Format minimal conseillé:
- un événement par commande acceptée.
- pas d'événement si `Err(...)`.

Format de log texte stable (une ligne par événement):
- `ts=<unix_ms> before=<hash> after=<hash> cmd=<CommandCompact>`.
- Exemple: `ts=1710000123456 before=a91f after=b172 cmd=InsertChar(x,0,3)`.

## 26.13 Projection de vue pure

```vit
form Viewport {
  top: int
  height: int
}

proc visible_lines(d: Document, v: Viewport) -> string[] {
  let out: string[] = []
  let i: int = v.top
  loop {
    if i >= d.lines.len() { break }
    if i >= v.top + v.height { break }
    out.push(d.lines[i])
    set i = i + 1
  }
  give out
}
```

Règle: la projection ne mute jamais `Document`.

## 26.14 Contrats testables

Assertions exactes à automatiser:
1. Bornes: après toute commande acceptée, `0 <= row <= max_row` et `0 <= col <= len(line[row])`.
2. Atomicité: si `apply_command` retourne `Err(_)`, le hash d'état reste identique.
3. Symétrie undo/redo: sur un historique compatible, `undo(redo(s)) == s`.
4. Historique: après toute nouvelle commande mutante valide, `future.len() == 0`.
5. Pureté projection: `visible_lines` ne modifie jamais le hash de `Document`.
6. Déterminisme replay: rejouer le même script produit exactement le même hash final.

## 26.15 Checklist finale spécifique éditeur

Checklist de fin:
- bornes curseur validées à chaque entrée.
- atomicité garantie (`Ok` ou `Err`, jamais état partiel).
- undo/redo symétriques et testés.
- lecture seule renvoie `ErrReadOnly` pour mutation.
- historique cohérent (`new command => future = []`).
- projection strictement pure.
- replay long déterministe validé.
- journal d'événements activé.

## 26.16 Erreurs de conception à éviter

Anti-patterns explicites:
- validation dupliquée dans plusieurs couches au lieu d'un seul point de vérité.
- mapping de sortie de type HTTP-like trop tôt, avant stabilisation de l'état métier.
- mutation cachée dans la projection (`visible_lines` qui écrirait dans `doc`).
- inversion incomplète de commandes (absence de `deleted` pour `Backspace`).
- écriture concurrente directe dans `doc/cursor/history` sans file sérialisée.


## Table erreur -> diagnostic -> correction

| Erreur | Diagnostic | Correction |
| --- | --- | --- |
| Entrée invalide | Validation absente ou trop tardive. | Centraliser la validation en entrée de pipeline. |
| État incohérent | Mutation partielle ou invariant non vérifié. | Appliquer le principe d'atomicité et rejeter sinon. |
| Sortie inattendue | Couche projection mélangée avec la logique métier. | Séparer `apply` (métier) et `project` (sortie). |

## À retenir

Un éditeur robuste n'est pas seulement un rendu de texte; c'est une machine d'état stricte, typée, atomique et rejouable. Le couple `Command` enrichi + `EditResult` rend les erreurs explicites, fiabilise undo/redo et prépare l'évolution vers des structures plus performantes sans casser l'API métier.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: `Err(ErrInvalid)` et état inchangé. Si `read_only` est actif sur une mutation, `Err(ErrReadOnly)`.

## À faire

1. Implémentez un hash d'état stable (`doc + cursor + history`) pour les tests de propriété.
2. Ajoutez une sérialisation JSON de `EditorState`, `History`, `EditEvent`.
3. Exécutez un replay de 100 commandes et comparez les hashes finaux sur deux runs.

## Corrigé minimal

- `Backspace(..., deleted)` élimine l'heuristique et rend l'inversion exacte.
- `EditResult` impose un flux explicite pour erreurs métier.
- La cohérence historique est obtenue si toute commande valide pousse dans `past` et vide `future`.

## Micro challenge final: sélection de texte

Ajoutez un pipeline `select -> transform -> apply -> project` sans casser le cœur existant.

Contraintes:
- `select` calcule une plage (`start`, `end`) sans muter le document.
- `transform` produit une commande composée (ex: upper/lower/delete range).
- `apply` reste atomique et retourne `EditResult`.
- `project` expose la sélection et le résultat sans logique métier cachée.

## Plan d'implémentation en 5 étapes

1. MVP: modèle canonique + bornes + commandes de base (`Left`, `Right`, `InsertChar`, `NewLine`, `Backspace`).
2. Undo exact: enrichir `Backspace` avec `deleted` et finaliser `inverse_of`.
3. Replay: ajouter historique complet + tests déterministes longs.
4. Observabilité: brancher `EditEvent` et logs stables une ligne par transition.
5. Optimisation structure: migrer l'interne vers gap buffer ou rope sans casser l'API publique.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/and.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/break.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/entry.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

