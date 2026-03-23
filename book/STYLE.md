# Guide de style éditorial (Vitte Book)

## Votre
- Précis, concret, orienté exécution.
- Éviter les formulations vagues et les répétitions automatiques.
- Expliquer l'intention avant le détail syntaxique.

## Longueur cible
- `Objectif`: 1 paragraphe (3 à 5 lignes).
- `Exemple`: 1 bloc principal + 1 variante invalide.
- `Pourquoi`: 1 paragraphe court (3 phrases maximum).
- `Test mental`: 1 question + 1 réponse attendue.
- `À faire`: 2 à 4 exercices courts.
- `Corrigé minimal`: 3 points vérifiables.
- Taille chapitre complet: équivalent `10` à `30` pages papier.
- Référence de calcul: `350` mots/page.
- Bornes opérationnelles: `3500` à `10500` mots par chapitre.
- Contrôle automatique: `make book-length-check`.

## Format des exemples
- Utiliser une progression stable: entrée -> traitement -> sortie.
- Toujours préciser les préconditions et invariants.
- Ajouter un cas invalide explicite au moins une fois par section technique.

## Ponctuation
- Toute puce se termine par un signe de ponctuation.
- Titres homogènes: `## ...` en français.
- Éviter les points d'exclamation hors citations.

## Trame obligatoire des chapitres
- `## Objectif`
- `## Exemple`
- `## Pourquoi`
- `## Test mental`
- `## À faire`
- `## Corrigé minimal`

## Trame obligatoire des fiches mot-clé
- `## Définition`
- `## Syntaxe`
- `## Quand l’utiliser / Quand l’éviter`
- `## Exemple nominal`
- `## Exemple invalide`
- `## Erreurs compilateur fréquentes`
- `## Mot-clé voisin`
- `## Pièges`
- `## Utilisé dans les chapitres`
- `## Voir aussi`

Règle anti-redondance:
- Ne pas dupliquer `## Différences proches` et `## Mot-clé voisin`.
- Conserver uniquement `## Mot-clé voisin`.

## Balisage global
- Inclure un bandeau `Prérequis` en tête de chapitre.
- Inclure une ligne `Voir aussi` vers chapitres connexes.
- Ajouter un `Checkpoint synthèse` tous les 2 à 3 chapitres.

## Liens et chemins
- Référencer le livre avec le préfixe `book/` (pas `docs/book/`).
- Préférer des chemins stables et relatifs à la racine du repo.
