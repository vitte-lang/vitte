# 48. Tracage et observabilite

Niveau: Avance

Prerequis: `book/chapters/47-debugage-reproductible.md`.

## Objectif

Rendre le comportement du programme observable en production et en test.

## Axes

1. Logs structures avec contexte minimal utile.
2. Correlation par identifiant de requete.
3. Metriques sur erreurs/latence/debit.
4. Traces sur chemins critiques.

## Checklist

1. Chaque erreur critique est tracable.
2. Les logs sont actionnables.
3. Les metriques permettent une alerte utile.


## Exemples progressifs (N1 -> N3)

### N1 (base): message contextualise

```vit
proc handle(req_id: int) -> int { give req_id }
```

### N2 (intermediaire): point d'erreur observable

```vit
proc handle_checked(ok: bool, code: int) -> int {
  if not ok { give code }
  give 0
}
```

### N3 (avance): correlation metrique/log

```vit
proc handle_trace(req_id: int, ok: bool) -> int {
  if not ok { give req_id }
  give 0
}
```

### Anti-exemple

```vit
proc handle_silent() -> int { give 0 }
```

## Validation rapide

1. Identifier contexte minimal dans logs.
2. Associer erreur a metrique.
3. Verifier exploitabilite en incident.
