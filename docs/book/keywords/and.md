# Mot-clé `and`

Niveau: Débutant.

## Définition

`and` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `and ...`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `and` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
proc can_edit(is_admin: bool, unlocked: bool) -> bool {
  give is_admin and unlocked
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc bad(a: int, b: int) -> bool {
  give a and b
}
# invalide: `and` attend des bool.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near and` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `and`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `or` | `and` et `or` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `and` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/08-structures.md`.
- `docs/book/chapters/24-projet-kv.md`.
- `docs/book/chapters/26-projet-editor.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/or.md`.
- `docs/book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.
