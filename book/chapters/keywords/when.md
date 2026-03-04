# Mot-clé `when`

Niveau: Intermédiaire.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`when` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`when` intervient dans `select` et dans la forme `when expr is pattern`.

## Syntaxe

Forme canonique: `when pattern block` ou `when expr is pattern block`.

## Exemple nominal

Entrée:
- Forme `when expr is pattern`.

```vit
pick Resp { case Ok, case Err }
proc on_resp(r: Resp) -> int {
  when r is Ok { give 0 }
  give 1
}
```

Sortie observable:
- La branche `when` s’active seulement si le pattern correspond.

## Exemple invalide

Entrée:
- Clause incomplète.

```vit
when r is { give 0 }
# invalide: pattern manquant après `is`.
```

Sortie observable:
- Le parseur rejette la clause.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `select` | `select` porte la séquence; `when` porte chaque branche. |

## Refactor rapide

Avant:
```vit
# usage fragile à corriger
```

Après:
```vit
# usage clair et testable
```

## Pièges

- Écrire `when` sans bloc.
- Mettre un littéral non valide comme pattern.
- Confondre `when` et `case`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour exprimer une branche basée sur un pattern.
- Quand l’éviter: pour un test booléen direct simple (`if`).

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near when` | Pattern manquant. | Fournir `when pattern { ... }` ou `when expr is pattern { ... }`. |
| `invalid pattern` | Pattern non conforme. | Utiliser un identifiant/pattern qualifié valide. |
| `when outside statement context` | Position invalide. | Utiliser `when` uniquement dans un contexte `stmt`. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `select` | `select` porte la séquence; `when` porte chaque branche. |

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/select.md`.
- `docs/book/keywords/is.md`.
- `docs/book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
