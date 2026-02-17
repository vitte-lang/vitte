# Mot-clé `return`

Niveau: Débutant.

## Définition

`return` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `return 0`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `return` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
entry main at app/core {
  return 0
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc f() -> int {
  return "ok"
}
# invalide: type de retour incompatible.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near return` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `return`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `give` | `return` et `give` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `return` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/00-avant-propos.md`.
- `docs/book/chapters/00-preface.md`.
- `docs/book/chapters/01-demarrer.md`.
- `docs/book/chapters/02-philosophie.md`.
- `docs/book/chapters/03-projet.md`.
- `docs/book/chapters/14-macros.md`.
- `docs/book/chapters/21-projet-cli.md`.
- `docs/book/chapters/23-projet-sys.md`.
- `docs/book/chapters/24-projet-kv.md`.
- `docs/book/chapters/25-projet-arduino.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/29-style.md`.
- `docs/book/chapters/30-faq.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/give.md`.
- `docs/book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.
