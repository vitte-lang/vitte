# Mot-clé `proc`

Niveau: Débutant.

## Définition

`proc` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `proc nom(a: int) -> int { ... }`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `proc` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc bad_proc() -> int {
  proc
  give 0
}
# invalide: usage hors grammaire attendue pour `proc`.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near proc` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `proc`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `if` | `proc` et `if` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `proc` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/00-avant-propos.md`.
- `docs/book/chapters/00-preface.md`.
- `docs/book/chapters/01-demarrer.md`.
- `docs/book/chapters/02-philosophie.md`.
- `docs/book/chapters/03-projet.md`.
- `docs/book/chapters/04-syntaxe.md`.
- `docs/book/chapters/05-types.md`.
- `docs/book/chapters/06-procedures.md`.
- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/08-structures.md`.
- `docs/book/chapters/09-modules.md`.
- `docs/book/chapters/10-diagnostics.md`.
- `docs/book/chapters/11-collections.md`.
- `docs/book/chapters/12-pointeurs.md`.
- `docs/book/chapters/13-generiques.md`.
- `docs/book/chapters/14-macros.md`.
- `docs/book/chapters/15-pipeline.md`.
- `docs/book/chapters/16-interop.md`.
- `docs/book/chapters/17-stdlib.md`.
- `docs/book/chapters/18-tests.md`.
- `docs/book/chapters/19-performance.md`.
- `docs/book/chapters/20-repro.md`.
- `docs/book/chapters/20a-architecture-globale.md`.
- `docs/book/chapters/21-projet-cli.md`.
- `docs/book/chapters/22-projet-http.md`.
- `docs/book/chapters/23-projet-sys.md`.
- `docs/book/chapters/24-projet-kv.md`.
- `docs/book/chapters/25-projet-arduino.md`.
- `docs/book/chapters/26-projet-editor.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/28-conventions.md`.
- `docs/book/chapters/29-style.md`.
- `docs/book/chapters/30-faq.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/if.md`.
- `docs/book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.
