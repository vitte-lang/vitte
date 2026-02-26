# Mot-clé `set`

Niveau: Débutant.

## Définition

`set` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `set x = x + 1`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `set` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
proc count3() -> int {
  let i: int = 0
  set i = i + 3
  give i
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc bad_set() -> int {
  set
  give 0
}
# invalide: usage hors grammaire attendue pour `set`.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near set` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `set`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `let` | `set` et `let` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `set` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/00-avant-propos.md`.
- `docs/book/chapters/01-demarrer.md`.
- `docs/book/chapters/04-syntaxe.md`.
- `docs/book/chapters/06-procedures.md`.
- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/08-structures.md`.
- `docs/book/chapters/11-collections.md`.
- `docs/book/chapters/14-macros.md`.
- `book/chapters/17-stdlib.md`.
- `book/chapters/19-performance.md`.
- `docs/book/chapters/20-repro.md`.
- `book/chapters/23-projet-sys.md`.
- `docs/book/chapters/24-projet-kv.md`.
- `docs/book/chapters/26-projet-editor.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/let.md`.
- `book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.
