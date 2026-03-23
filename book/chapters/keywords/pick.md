# Mot-clé `pick`

Niveau: Intermédiaire.

## Lecture rapide

Repère concret: `pick` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `pick` quand il sert à définir un modèle de données; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `pick` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `pick` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`pick` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec une règle lisible et testable.

## Syntaxe

Forme canonique: `pick Result { case Ok case Err }`.

## Lecture algorithmique

Lecture conseillée: traquez où `pick` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Cas nominal: `pick` est utilisé dans le bon contexte et la branche attendue est exécutée.

```vit
// Exemple concret: cas nominal puis cas invalide

pick Result {
  case Ok
  case Err
}
```

Sortie observable:
- Pour l'entrée nominale, la branche attendue est prise et la sortie correspond au calcul prévu.

## Exemple invalide

Entrée:
- Cas d'erreur: usage invalide de `pick`; la validation doit refuser le snippet avec un diagnostic exploitable.

```vit
// Exemple concret: cas nominal puis cas invalide

proc bad_pick() -> int {
  pick
  // Valeur retournee par cette branche
  give 0
}
# invalide: usage hors grammaire attendue pour `pick`.
```

Sortie observable:
- Pour l'entrée invalide, la validation doit échouer avec un message exploitable pour corriger le code.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Refactor rapide

Avant:
```vit
// Exemple concret: cas nominal puis cas invalide
# usage fragile à corriger
```

Après:
```vit
// Exemple concret: cas nominal puis cas invalide
# usage clair et testable
```

## Pièges

- Utiliser `pick` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `pick` réduit une ambiguïté de lecture dans le flux d'exécution.
- Quand l’éviter: quand son usage masque la règle ou duplique une logique déjà portée ailleurs.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near pick` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Règle d’entrée/sortie incohérent autour de `pick`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `form` | `pick` et `form` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/form.md`.
- `book/glossaire.md`.
- `book/chapters/06-procedures.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
