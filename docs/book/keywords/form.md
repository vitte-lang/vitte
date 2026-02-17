# Mot-clé `form`

Niveau: Intermédiaire.

## Définition

`form` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `form Name { id: int }`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `form` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
form User {
  id: int
  name: string
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc bad_form() -> int {
  form
  give 0
}
# invalide: usage hors grammaire attendue pour `form`.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near form` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `form`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pick` | `form` et `pick` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `form` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/00-avant-propos.md`.
- `docs/book/chapters/03-projet.md`.
- `docs/book/chapters/05-types.md`.
- `docs/book/chapters/08-structures.md`.
- `docs/book/chapters/09-modules.md`.
- `docs/book/chapters/10-diagnostics.md`.
- `docs/book/chapters/11-collections.md`.
- `docs/book/chapters/12-pointeurs.md`.
- `docs/book/chapters/13-generiques.md`.
- `docs/book/chapters/15-pipeline.md`.
- `docs/book/chapters/16-interop.md`.
- `docs/book/chapters/17-stdlib.md`.
- `docs/book/chapters/22-projet-http.md`.
- `docs/book/chapters/23-projet-sys.md`.
- `docs/book/chapters/24-projet-kv.md`.
- `docs/book/chapters/25-projet-arduino.md`.
- `docs/book/chapters/26-projet-editor.md`.
- `docs/book/chapters/28-conventions.md`.
- `docs/book/chapters/29-style.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pick.md`.
- `docs/book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.
