# Mot-clé `macro`

Niveau: Avancé.

## Définition

`macro` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `macro nom(args) { ... }`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `macro` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
macro inc(x) { x + 1 }
proc f(v: int) -> int {
  give inc(v)
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
macro dup(x) { x + x }
proc f(v: int) -> int {
  give dup(v,)
}
# invalide: expansion avec arguments mal formes.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near macro` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `macro`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `if` | `macro` et `if` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `macro` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/13-generiques.md`.
- `docs/book/chapters/14-macros.md`.
- `docs/book/chapters/15-pipeline.md`.
- `docs/book/chapters/16-interop.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/if.md`.
- `docs/book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.

## Contre-exemple explicite

```vit
macro calc(x) { x + 1 }
proc f() -> int {
  give calc()
}
# contre-exemple: macro invoquee sans contrat d arguments stable.
```

Pourquoi c’est problématique: le code devient non total, fragile en maintenance, et plus difficile à diagnostiquer.
