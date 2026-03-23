# 3. Structure d'un projet

Niveau: Fondations avancées

Prérequis: `book/chapters/02-philosophie.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/04-syntaxe.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Problème structurel

La plupart des dégradations projet viennent d'une cause unique: des dépendances orientées dans le mauvais sens.
La syntaxe ne corrige pas ça. L'architecture, oui.

## Principe directeur

Imposer un graphe de dépendances acyclique et lisible:
- `domain` ne dépend d'aucune couche applicative
- `service` dépend de `domain`
- `io` dépend de `service` (et parfois `domain` pour l'assemblage des données)

Flux de contrôle attendu: `io -> service -> domain`.

## 3.1 Couche domaine: modèle métier stable

```vit
space app/domain

form Ticket {
  id: int
  priority: int
}
```

Responsabilité:
- exprimer le vocabulaire métier sans détail d'exécution.

Critère de qualité:
- un type de domaine doit rester valide même si on change de transport (CLI/API/job).

## 3.2 Couche service: règles métier centralisées

```vit
space app/service

pull app/domain as d

proc is_critical(t: d.Ticket) -> bool {
  give t.priority >= 9
}
```

Responsabilité:
- encapsuler la règle “critique” en un point unique.

Impact:
- si le seuil change (ex: `>= 8`), une seule zone est modifiée.

## 3.3 Couche io: orchestration et projection de sortie

```vit
space app/io

pull app/domain as d
pull app/service as s

// Point d'entree du scenario
entry main at core/app {
  let t: d.Ticket = d.Ticket(1, 9)
  let critical: bool = s.is_critical(t)
  let code: int = critical as int

  return code
}
```

Responsabilité:
- construire les entrées
- appeler le service
- projeter le résultat dans un format de sortie process

Ce que `io` ne doit pas faire:
- redéfinir des règles métier
- manipuler des conventions internes du domaine

## 3.4 Détection rapide des mauvaises frontières

Signal 1:
- une règle métier apparaît dans `entry`.

Signal 2:
- un module `service` importe `io`.

Signal 3:
- un type métier contient des considérations de transport (codes CLI, format HTTP, etc.).

Chacun de ces signaux augmente le coût de changement.

## 3.5 Test d'architecture (30 secondes)

Pour chaque fichier:
1. représente-t-il un concept métier pur ? -> `domain`
2. porte-t-il une décision métier ? -> `service`
3. gère-t-il exécution/assemblage/sortie ? -> `io`

S'il répond “oui” à plusieurs questions, la responsabilité est probablement mal découpée.

## 3.6 Check-list de revue avant commit

1. le graphe des dépendances est-il orienté `io -> service -> domain` ?
2. les règles métier sont-elles uniquement dans `service` ?
3. les types de `domain` sont-ils indépendants du transport ?
4. `entry` est-il court, lisible et purement orchestral ?

## Keywords à revoir

- `book/keywords/space.md`
- `book/keywords/pull.md`
- `book/keywords/form.md`
- `book/keywords/proc.md`
- `book/keywords/entry.md`



## Exemple Étendu


```vit
// Scenario projet: execution complete et verifiable
space demo/projet

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {

  if r.id <= 0 { give Result.Rejected(91) }

  if r.quota < 0 { give Result.Rejected(92) }

  if r.amount < 0 { give Result.Rejected(93) }

  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }

  if capped < 5 { give Result.Rejected(94) }

  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {

  match x {
    case Accepted(v) {
      if v % 13 == 0 { give Result.Rejected(95) }

      give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(x: Result) -> int {

  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)

  return to_exit(s)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.


## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Garde absente ou mal ordonnée | Rejouer avec cas limite | Remonter la garde avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Contrat implicite | Comparer nominal vs limite | Formaliser le contrat dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas limite réel et prévoir sa sortie,
- identifier où ajouter une garde sans casser le nominal.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: gardes d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
