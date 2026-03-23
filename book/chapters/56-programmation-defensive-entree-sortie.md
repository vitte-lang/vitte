# 56. Programmation defensive en entree/sortie

Niveau: Intermediaire

Prérequis: `book/chapters/10-diagnostics.md`, `book/chapters/47-debugage-reproductible.md`.
Voir aussi: à définir.

## Objectif

Eviter que des donnees externes invalides degradent la stabilite du systeme.

## Regles

1. Valider les entrees tot.
2. Normaliser formats et unites.
3. Timeouts et retries explicites sur I/O.
4. Erreurs retournees avec contexte utile.

## Checklist

1. Aucune hypothese implicite sur l'entree.
2. Chemin d'echec defini et teste.
3. Logs suffisamment precis pour diagnostiquer.

## Exemples progressifs (N1 -> N3)

### N1 (base): validation entree

```vit
proc parse_nonempty(ok: bool) -> bool { give ok }
```

### N2 (intermediaire): chemin d'echec

```vit
proc read_or_default(ok: bool, v: int, d: int) -> int {
  if not ok { give d }
  give v
}
```

### N3 (avance): politique stable

```vit
proc call_with_guard(tries: int) -> int {
  if tries <= 0 { give -1 }
  give 0
}
```

### Anti-exemple

```vit
proc trust_input(v: int) -> int { give v }
```

## Validation rapide

1. Valider entree.
2. Definir echec explicite.
3. Verifier comportements retries/timeout.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **programmation defensive en entree/sortie**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc parse_nonempty(ok: bool) -> bool { give ok }
```

Lecture ligne par ligne:
1. `proc parse_nonempty(ok: bool) -> bool { give ok }` -> pose un contrat clair de fonction.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->
