# 72. Projet embarque (contraintes mémoire/temps)

Niveau: Avancé

Prérequis: `book/chapters/25-projet-arduino.md`, `book/chapters/45-performance-allocations-copies.md`.

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
