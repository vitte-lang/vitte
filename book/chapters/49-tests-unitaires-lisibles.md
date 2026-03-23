# 49. Ecriture de tests unitaires lisibles

Niveau: Intermediaire

Prérequis: `book/chapters/18-tests.md`.
Voir aussi: à définir.

## Objectif

Ecrire des tests courts et comprenables qui documentent le contrat fonctionnel.

## Structure recommandee

1. Arrange: preparer le contexte.
2. Act: executer une action.
3. Assert: verifier une sortie observable.

## Bonnes pratiques

1. Un comportement par test.
2. Noms explicites orientes intention.
3. Donnees d'entree minimales mais parlantes.


## Exemples progressifs (N1 -> N3)

### N1 (base): test nominal

```vit
proc add(a:int,b:int)->int { give a+b }
```

### N2 (intermediaire): test cas limite

```vit
proc clamp_nonneg(x:int)->int {
  if x < 0 { give 0 }
  give x
}
```

### N3 (avance): tests lisibles

```vit
proc is_even(x:int)->bool { give x % 2 == 0 }
```

### Anti-exemple

```vit
proc ambiguous(x:int)->int { give x }
# comportement non specifie par test
```

## Validation rapide

1. Un comportement par test.
2. Cas limite explicite.
3. Assertions lisibles.

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
