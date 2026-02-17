# Mot-clé `in`

Niveau: Débutant.

## Définition

`in` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `in ...`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `in` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
proc has_positive(xs: int[]) -> bool {
  for x in xs {
    if x > 0 { give true }
  }
  give false
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc bad(xs: int[]) -> int {
  for in xs { give 0 }
  give 1
}
# invalide: variable de boucle manquante.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near in` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `in`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `if` | `in` et `if` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `in` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/11-collections.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/if.md`.
- `docs/book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.
