# Mot-clé `unsafe`

Niveau: Avancé.

## Définition

`unsafe` est un mot-clé du langage Vitte. Cette fiche donne un usage opérationnel avec un contrat lisible et testable.

## Syntaxe

Forme canonique: `unsafe { ... }`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: quand `unsafe` rend l’intention plus explicite et vérifiable.
- Quand l’éviter: quand son usage masque le contrat ou duplique une logique déjà portée ailleurs.

## Exemple nominal

Entrée:
- Cas nominal contrôlé et déterministe.

```vit
proc cpu_nop() {
  unsafe { asm("nop") }
}
```

Sortie observable:
- Le flux suit la branche attendue et produit une sortie stable.

## Exemple invalide

Entrée:
- Cas volontairement hors contrat.

```vit
proc bad(flag: bool) {
  unsafe {
    if flag { asm("hlt") }
  }
}
# invalide conceptuel: validation métier placee dans `unsafe`.
```

Sortie observable:
- Le compilateur (ou la validation) doit rejeter ce cas avec un diagnostic explicite.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near unsafe` | Forme syntaxique incomplète ou mal placée. | Revenir à la forme canonique et vérifier les délimiteurs. |
| `type mismatch` | Contrat d’entrée/sortie incohérent autour de `unsafe`. | Aligner les types attendus avant exécution. |
| `unreachable or incomplete branch` | Couverture de cas incomplète ou branche morte. | Ajouter la branche manquante (`otherwise`) ou simplifier le flux. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `asm` | `unsafe` et `asm` se complètent, mais n’ont pas la même responsabilité de contrôle/retour. |

## Pièges

- Utiliser `unsafe` par habitude au lieu de justifier son rôle dans le flux.
- Mélanger la logique métier et la logique de contrôle sans frontière explicite.
- Oublier de tester un cas invalide dédié.

## Utilisé dans les chapitres

- `docs/book/chapters/12-pointeurs.md`.
- `docs/book/chapters/16-interop.md`.
- `book/chapters/23-projet-sys.md`.


## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/asm.md`.
- `book/glossaire.md`.
- `docs/book/chapters/06-procedures.md`.

## Contre-exemple explicite

```vit
proc danger(x: int) -> int {
  unsafe {
    if x < 0 { give 0 }
    asm("nop")
  }
  give x
}
# contre-exemple: logique métier et zone unsafe melangees.
```

Pourquoi c’est problématique: le code devient non total, fragile en maintenance, et plus difficile à diagnostiquer.
