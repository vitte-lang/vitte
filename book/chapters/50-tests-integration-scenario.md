# 50. Tests d'integration orientes scenario

Niveau: Avance

Prérequis: `book/chapters/18-tests.md`, `book/chapters/21-projet-cli.md`.
Voir aussi: à définir.

## Objectif

Verifier des parcours complets utilisateur/systeme plutot que des fonctions isolees.

## Demarche

1. Definir scenario nominal et scenario echec.
2. Monter un environnement proche production.
3. Verifier sortie metier et effets de bord.
4. Nettoyer et rerendre le test idempotent.

## Checklist

1. Donnees de test versionnees.
2. Resultat attendu stable.
3. Execution possible en CI.


## Exemples progressifs (N1 -> N3)

### N1 (base): scenario nominal

```vit
entry main at app/cli {
  return 0
}
```

### N2 (intermediaire): scenario erreur

```vit
entry main at app/cli {
  return missing
}
```

### N3 (avance): scenario idempotent

```vit
proc run_once(ok: bool) -> int {
  if not ok { give 1 }
  give 0
}
```

### Anti-exemple

```vit
# scenario depend d'un etat manuel externe
```

## Validation rapide

1. Nominal + erreur couverts.
2. Sortie observable verifiee.
3. Reexecution sans effet parasite.

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
