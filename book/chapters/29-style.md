# 29. Style d'architecture

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/28-conventions.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/28-conventions.md`, `book/chapters/30-faq.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Style d'architecture**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Style d'architecture**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Style d'architecture**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Style d'architecture** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Style d'architecture**.

## Exercice court

Prenez un exemple du chapitre sur **Style d'architecture**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Style d'architecture**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 29.1 Écrire un domaine pur sans dépendance d'infrastructure

```vit
space app/domain
form Order {
  amount: int
}
proc approve(o: Order) -> bool {
  give o.amount > 0
}
```

Lecture ligne par ligne (débutant):
1. `space app/domain` : cette ligne définit une étape explicite du flux.
2. `form Order {` : cette ligne ouvre la structure `Order` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
3. `amount: int` : cette ligne déclare le champ `amount` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
5. `proc approve(o: Order) -> bool {` : le contrat est défini pour `approve`: entrées `o: Order` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
6. `give o.amount > 0` : la branche renvoie immédiatement `o.amount > 0` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `o.amount > 0`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: isoler un domaine pur, testable sans dépendance d'infrastructure.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `approve(Order(10))=true` et `approve(Order(0))=false`.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 29.2 Traduire le métier dans une couche service

```vit
space app/service
pull app/domain as d
proc handle(amount: int) -> int {
  let ok: bool = d.approve(d.Order(amount))

  if ok { give 200 }

  give 422
}
```

Lecture ligne par ligne (débutant):
1. `space app/service` : cette ligne définit une étape explicite du flux.
2. `pull app/domain as d` : cette ligne définit une étape explicite du flux.
3. `proc handle(amount: int) -> int {` : le contrat est posé pour `handle`: entrées `amount: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
4. `let ok: bool = d.approve(d.Order(amount))` : cette ligne crée la variable `ok` de type `bool` pour nommer explicitement une étape intermédiaire du raisonnement.
5. `if ok { give 200 }` : cette garde traite le cas limite avant le calcul.
6. `give 422` : la sortie est renvoyée immédiatement `422` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `ok` est vrai, la sortie devient `200`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `422`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: faire jouer au service un rôle de traduction entre métier et sortie applicative.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `handle(10)=200` et `handle(0)=422`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 29.3 Garder un point d'entrée mince dans la couche IO

```vit
space app/io
pull app/service as s

// Point d'entree du scenario
entry main at core/app {
  let code: int = s.handle(10)

  return code
}
```

Lecture ligne par ligne (débutant):
1. `space app/io` : cette ligne définit une étape explicite du flux.
2. `pull app/service as s` : cette ligne définit une étape explicite du flux.
3. `entry main at core/app {` : cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre.
4. `let code: int = s.handle(10)` : cette ligne crée la variable `code` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
5. `return code` : cette ligne termine l'exécution du bloc courant avec le code `code`, utile pour observer le résultat global du scénario.
6. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return code`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `code`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: garder un point d'entrée mince, limité à l'orchestration.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, le scénario montre un retour final `200`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Domaine pur, service de traduction et entry minimal forment une architecture stable à long terme. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez dire dans quelle couche doit vivre une règle métier.
- vous pouvez modifier l'IO sans toucher au domaine.
- vous pouvez testér la logique principale sans démarrer l'application complète.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/as.md`.
- `book/keywords/at.md`.
- `book/keywords/bool.md`.
- `book/keywords/continue.md`.
- `book/keywords/entry.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **style d'architecture**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
space app/domain
form Order {
  amount: int
}
proc approve(o: Order) -> bool {
  give o.amount > 0
}
```

Lecture ligne par ligne:
1. `space app/domain` -> participe au déroulé du traitement.
2. `form Order {` -> participe au déroulé du traitement.
3. `amount: int` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `proc approve(o: Order) -> bool {` -> pose un contrat clair de fonction.
6. `give o.amount > 0` -> renvoie la sortie vérifiable.
7. `}` -> participe au déroulé du traitement.

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
// Scenario style: execution complete et verifiable
space demo/style

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


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
