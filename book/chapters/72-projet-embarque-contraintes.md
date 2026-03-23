# 72. Projet embarque (contraintes mémoire/temps)

Niveau: Avancé

Prérequis: `book/chapters/25-projet-arduino.md`, `book/chapters/45-performance-allocations-copies.md`.
Voir aussi: à définir.

## Objectif

Livrer un projet embarque respectant budget mémoire et contraintes de latence.

## Axes

1. Profil mémoire strict.
2. Boucles temps critique deterministes.
3. Gestion defensive des erreurs I/O.
4. Vérification sur cible reelle.

## Documentation a produire

1. Budget mémoire (statique/dynamique) par module.
2. Budget temps par boucle critique.
3. Matrice des modes de panne et comportement attendu.
4. Procedure de vérification sur cible (pas uniquement emulateur).
5. Contraintes hardware minimales et versions supportees.

## Budgets cibles (exemple)

| Composant | RAM max | Flash max | Latence cible |
| --- | --- | --- | --- |
| Acquisition capteur | 8 KB | 24 KB | <= 2 ms |
| Traitement signal | 16 KB | 48 KB | <= 5 ms |
| Communication serie | 6 KB | 20 KB | <= 3 ms |
| Boucle complete | 32 KB | 96 KB | <= 10 ms |

## Criteres de rejet build

1. Build KO si RAM estimee > budget global.
2. Build KO si Flash estimee > budget global.
3. Build KO si latence p95 > cible sur banc de test.
4. Build KO si une vérification cible reelle manque.

## Exemples progressifs (N1 -> N3)

### N1 (base): boucle capteur simple

```vit
proc sample_once(v: int) -> int {
  give v
}
```

Objectif:
1. Chemin nominal minimal, sans allocation dynamique.

### N2 (intermediaire): garde temps/reponse

```vit
proc read_checked(ok: bool, value: int) -> int {
  if not ok { give -1 }
  give value
}
```

Objectif:
1. Rendre explicite le chemin d'echec capteur.

### N3 (avance): budget + controle système

```text
- RAM cible: <= 32 KB
- Flash cible: <= 96 KB
- boucle complete p95: <= 10 ms
- vérification sur carte reelle obligatoire
```

Objectif:
1. Verifier contraintes mémoire/latence avant release.

### Anti-exemple (invalide production embarquee)

```text
- allocations non bornees en boucle
- aucune mesure p95
- validation uniquement sur emulateur
```

Pourquoi c'est un probleme:
1. Risque de depassement mémoire.
2. Risque de latence non deterministe.

## Validation rapide

1. Verifier N1 (pas d'allocation superflue).
2. Verifier N2 (cas echec couvre).
3. Verifier N3 (budgets et mesures respectes).

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

Thème: **projet embarque (contraintes mémoire/temps)**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc sample_once(v: int) -> int {
  give v
}
```

Lecture ligne par ligne:
1. `proc sample_once(v: int) -> int {` -> pose un contrat clair de fonction.
2. `give v` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

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
