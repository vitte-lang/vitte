# 26. Projet guide Éditeur (version poche)

Prérequis: `docs/book/poche/SOMMAIRE.md`.
Voir aussi: `docs/book/chapters/26-projet-editor.md`.

## Objectif

Comprendre projet guide éditeur de manière simple pour agir rapidement.

## Idée clé en 3 points

1. Identifier le besoin concret.
2. Appliquer la forme minimale correcte.
3. Vérifier le résultat avec un test court.

## Quand l'utiliser

1. Quand vous avez besoin d'une solution lisible immédiatement.
2. Quand vous voulez valider une base avant d'aller plus loin.
3. Quand vous devez expliquer rapidement le sujet à quelqu'un d'autre.

## Exemple minimal

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

## Erreurs fréquentes

1. Vouloir couvrir tous les cas d'un coup.
2. Mélanger plusieurs changements dans la même étape.
3. Oublier de relire le message d'erreur exact.

## Mini-exercice (5 minutes)

Modifiez l'exemple pour créer une variante de projet guide éditeur, puis vérifiez le résultat attendu.

## Checklist rapide

1. Le code compile.
2. Le résultat est compréhensible.
3. Le cas limite principal est testé.

## Corrigé minimal

Partir du cas nominal, faire une seule modification, recompiler, puis valider la sortie.

## Aller plus loin

Version complète: `docs/book/chapters/26-projet-editor.md`.
