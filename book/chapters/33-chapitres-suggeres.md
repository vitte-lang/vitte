# 33. Chapitres suggérés (max)

Prérequis: à définir.
Voir aussi: à définir.
Niveau: Tous niveaux

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Proposer une réserve de chapitres additionnels pour étendre le livre selon vos priorités (langage, outillage, qualité, projets).

## Suggestions de chapitres

1. Modele mental du compilateur
2. Anatomie d'un message d'erreur
3. Strategies de nommage en Vitte
4. Conventions de modules a grande echelle
5. Refactoring guide par les types
6. Design d'API publiques stables
7. Gestion de compatibilite (breaking/non-breaking)
8. Versionnement et migration de code
9. Patterns de procedures robustes
10. Anti-patterns frequents en code Vitte
11. Performance: mesurer avant d'optimiser
12. Performance: allocations et copies
13. Performance: structures de donnees adaptees
14. Debogage reproductible
15. Tracage et observabilite
16. Ecriture de tests unitaires lisibles
17. Tests d'integration orientes scenario
18. Tests de non-regression compilateur
19. Tests de grammaire (valid/invalid corpus)
20. Property-based testing pour le parser
21. Fuzzing de l'entree source
22. Securite memoire et invariants
23. Programmation defensive en entree/sortie
24. Gestion des erreurs semantiques
25. Pipeline CI pour un projet Vitte
26. Revue de code orientée risques
27. Documentation technique durable
28. Génération de diagrammes de grammaire
29. Lecture avancée de l’EBNF du langage
30. Résolution d’ambiguïtés syntaxiques
31. Mapping AST -> IR (vue pratique)
32. Contrats ABI et interop native
33. Macros: cas d’usage et limites
34. Génériques: design et compromis
35. Architecture hexagonale en Vitte
36. Projet complet CLI production-ready
37. Projet complet HTTP production-ready
38. Projet complet service système
39. Projet embarqué (contraintes mémoire/temps)
40. Plan de montée en compétence équipe
41. Documenter les documentations (meta-doc)
42. Politique de version documentaire
43. Runbook de maintenance documentaire
44. QA documentaire automatique (liens/snippets/commandes)
45. Ownership et SLA de mise a jour de la doc

## Sélection prioritaire (phase 1)

1. Modèle mental du compilateur
2. Anatomie d’un message d’erreur
3. Stratégies de nommage en Vitte
4. Conventions de modules à grande échelle
5. Refactoring guidé par les types
6. Design d’API publiques stables
7. Gestion de compatibilité (breaking/non-breaking)
8. Versionnement et migration de code
9. Patterns de procédures robustes

## Mini-objectif par chapitre prioritaire

1. Modèle mental du compilateur: comprendre le pipeline source -> parse -> validation -> génération.
2. Anatomie d’un message d’erreur: lire rapidement cause, position, et correction probable.
3. Stratégies de nommage en Vitte: rendre API et modules cohérents à l’échelle d’un projet.
4. Conventions de modules à grande échelle: structurer `space/pull/use/share` sans dérive.
5. Refactoring guidé par les types: utiliser les annotations et erreurs de type pour sécuriser les changements.
6. Design d’API publiques stables: limiter les ruptures et clarifier les contrats.
7. Gestion de compatibilité (breaking/non-breaking): classifier et annoncer les changements correctement.
8. Versionnement et migration de code: planifier la transition avec étapes, garde-fous et exemples.
9. Patterns de procédures robustes: écrire des `proc` lisibles, testables et résistantes aux cas limites.

## Chapitres créés

1. `book/chapters/34-modele-mental-compilateur.md`
2. `book/chapters/35-anatomie-message-erreur.md`
3. `book/chapters/36-strategies-nommage-vitte.md`
4. `book/chapters/37-conventions-modules-echelle.md`
5. `book/chapters/38-refactoring-guide-types.md`
6. `book/chapters/39-design-api-publiques-stables.md`
7. `book/chapters/40-compatibilite-breaking.md`
8. `book/chapters/41-versionnement-migration-code.md`
9. `book/chapters/42-patterns-procedures-robustes.md`

## Priorisation recommandée

1. Fondations: 1, 2, 4, 6, 9
2. Qualité: 16, 17, 18, 19, 26
3. Compiler/grammaire: 28, 29, 30, 31
4. Mise en prod: 11, 12, 14, 25, 32
5. Projets: 36, 37, 38, 39

## Utilisation

Prenez 5 chapitres prioritaires, assignez un niveau cible (débutant/intermédiaire/avancé), puis transformez chaque suggestion en plan court: objectif, exemple minimal, pièges, exercice, checklist.

## Chapitres crees (phase 2)

10. `book/chapters/43-anti-patterns-code-vitte.md`
11. `book/chapters/44-performance-mesurer-avant-optimiser.md`
12. `book/chapters/45-performance-allocations-copies.md`
13. `book/chapters/46-performance-structures-adaptees.md`
14. `book/chapters/47-debugage-reproductible.md`
15. `book/chapters/48-tracage-observabilite.md`
16. `book/chapters/49-tests-unitaires-lisibles.md`
17. `book/chapters/50-tests-integration-scenario.md`
18. `book/chapters/51-tests-non-regression-compilateur.md`
19. `book/chapters/52-tests-grammaire-valid-invalid.md`
20. `book/chapters/53-property-based-testing-parser.md`
21. `book/chapters/54-fuzzing-entree-source.md`
22. `book/chapters/55-securite-memoire-invariants.md`
23. `book/chapters/56-programmation-defensive-entree-sortie.md`
24. `book/chapters/57-gestion-erreurs-semantiques.md`

## Chapitres crees (phase 3)

25. `book/chapters/58-pipeline-ci-projet-vitte.md`
26. `book/chapters/59-revue-code-orientee-risques.md`
27. `book/chapters/60-documentation-technique-durable.md`
28. `book/chapters/61-generation-diagrammes-grammaire.md`
29. `book/chapters/62-lecture-avancee-ebnf.md`
30. `book/chapters/63-resolution-ambiguites-syntaxiques.md`
31. `book/chapters/64-mapping-ast-vers-ir.md`
32. `book/chapters/65-contrats-abi-interop-native.md`
33. `book/chapters/66-macros-cas-usage-limites.md`
34. `book/chapters/67-generiques-design-compromis.md`
35. `book/chapters/68-architecture-hexagonale-vitte.md`
36. `book/chapters/69-projet-cli-production-ready.md`
37. `book/chapters/70-projet-http-production-ready.md`
38. `book/chapters/71-projet-service-systeme.md`
39. `book/chapters/72-projet-embarque-contraintes.md`
40. `book/chapters/73-montee-competence-equipe.md`

## Chapitres crees (phase 4)

41. `book/chapters/74-documenter-les-documentations.md`

## Statut par chapitre (roadmap editoriale)

| # | Chapitre | Statut |
| --- | --- | --- |
| 1 | Modele mental du compilateur | cree |
| 2 | Anatomie d'un message d'erreur | cree |
| 3 | Strategies de nommage en Vitte | cree |
| 4 | Conventions de modules a grande echelle | cree |
| 5 | Refactoring guide par les types | cree |
| 6 | Design d'API publiques stables | cree |
| 7 | Gestion de compatibilite (breaking/non-breaking) | cree |
| 8 | Versionnement et migration de code | cree |
| 9 | Patterns de procedures robustes | cree |
| 10 | Anti-patterns frequents en code Vitte | cree |
| 11 | Performance: mesurer avant d'optimiser | cree |
| 12 | Performance: allocations et copies | cree |
| 13 | Performance: structures de donnees adaptees | cree |
| 14 | Debogage reproductible | cree |
| 15 | Tracage et observabilite | cree |
| 16 | Ecriture de tests unitaires lisibles | cree |
| 17 | Tests d'integration orientes scenario | cree |
| 18 | Tests de non-regression compilateur | cree |
| 19 | Tests de grammaire (valid/invalid corpus) | cree |
| 20 | Property-based testing pour le parser | cree |
| 21 | Fuzzing de l'entree source | cree |
| 22 | Securite memoire et invariants | cree |
| 23 | Programmation defensive en entree/sortie | cree |
| 24 | Gestion des erreurs semantiques | cree |
| 25 | Pipeline CI pour un projet Vitte | cree |
| 26 | Revue de code orientee risques | cree |
| 27 | Documentation technique durable | a renforcer |
| 28 | Generation de diagrammes de grammaire | cree |
| 29 | Lecture avancee de l'EBNF du langage | cree |
| 30 | Resolution d'ambiguites syntaxiques | cree |
| 31 | Mapping AST -> IR (vue pratique) | cree |
| 32 | Contrats ABI et interop native | cree |
| 33 | Macros: cas d'usage et limites | cree |
| 34 | Generiques: design et compromis | cree |
| 35 | Architecture hexagonale en Vitte | a renforcer |
| 36 | Projet complet CLI production-ready | cree |
| 37 | Projet complet HTTP production-ready | a renforcer |
| 38 | Projet complet service systeme | cree |
| 39 | Projet embarque (contraintes memoire/temps) | a renforcer |
| 40 | Plan de montee en competence equipe | a renforcer |
| 41 | Documenter les documentations (meta-doc) | a renforcer |
| 42 | Politique de version documentaire | a creer |
| 43 | Runbook de maintenance documentaire | a creer |
| 44 | QA documentaire automatique (liens/snippets/commandes) | a creer |
| 45 | Ownership et SLA de mise a jour de la doc | a creer |

## Exemple

Exemple de départ: reprenez le plus petit snippet de ce chapitre et vérifiez le comportement attendu.

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

Thème: **chapitres suggérés (max)**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
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



## Exemple Étendu


```vit
// Scenario chapitres suggeres: execution complete et verifiable
space demo/chapitres-suggeres

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {

  if x.id <= 0 { give Eval.Rejected(21) }

  if x.quota < 0 { give Eval.Rejected(22) }

  if x.value < 0 { give Eval.Rejected(23) }

  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }

  if capped < 0 { give 0 }

  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {

  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)

      if out >= 10 { give Eval.Accepted(out) }

      give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(r: Eval) -> int {

  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)

  return to_exit(d)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.


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
