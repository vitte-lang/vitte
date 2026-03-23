# 28. Conventions de code

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/27-grammaire.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/27-grammaire.md`, `book/chapters/29-style.md`, `book/glossaire.md`.

## Problème Concret

Situation réelle: pour Conventions de code, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Conventions de code**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Conventions de code** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Conventions de code**.

## Exercice court

Prenez un exemple du chapitre sur **Conventions de code**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Conventions de code**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 28.1 Nommer les procédures selon leur intention

```vit
proc parse_port(x: int) -> int {

  if x < 0 { give -1 }

  if x > 65535 { give -1 }

  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc parse_port(x: int) -> int {` : la règle est défini pour `parse_port`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x < 0 { give -1 }` : cette test traite le cas d'erreur avant le calcul.
3. `if x > 65535 { give -1 }` : cette test traite le cas d'erreur avant le calcul.
4. `give x` : la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `x < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans test bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: faire porter l'intention métier par le nom de la procédure.

Le verbe `parse` signale une règle de validation, et les tests en tête rendent le flux immédiat à relire.

À l'exécution, `parse_port(8080)=8080`, `parse_port(70000)=-1`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 28.2 Nommer les structures selon le domaine

```vit
form User {
  id: int
  name: string
}
```

Lecture ligne par ligne (débutant):
1. `form User {` : cette ligne ouvre la structure `User` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `id: int` : cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `name: string` : cette ligne déclare le champ `name` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie stable.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: utiliser des noms de structure et de champs qui décrivent le domaine, pas l'implémentation.

Le schéma devient auto-documenté: la structure se lit comme une mini spécification métier.

À l'exécution, cette déclaration n'exécute rien, mais elle fixe une règle de données clair au compile-time.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 28.3 Nommer les prédicats en booléen explicite

```vit
proc is_admin(role: int) -> bool {
  give role == 9
}
```

Lecture ligne par ligne (débutant):
1. `proc is_admin(role: int) -> bool {` : la règle est posé pour `is_admin`: entrées `role: int` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
2. `give role == 9` : la sortie est renvoyée immédiatement `role == 9` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: sans test bloquante, la branche principale renvoie `role == 9`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: rendre les prédicats immédiatement reconnaissables dans les conditions.

Le préfixe `is_` réduit la charge cognitive: dans un `if`, on lit directement une question booléenne.

À l'exécution, `is_admin(9)=true` et `is_admin(1)=false`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Les noms expriment l'intention, les tests sont en tête et les prédicats booléens sont reconnaissables instantanément. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, règle explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous pouvez deviner le rôle d'une fonction en lisant uniquement son nom.
- vous savez distinguer un prédicat booléen d'une transformation.
- vous pouvez relire un fichier et repérer les conventions violées en quelques secondes.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/bool.md`.
- `book/keywords/continue.md`.
- `book/keywords/false.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **conventions de code**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc parse_port(x: int) -> int {

  if x < 0 { give -1 }

  if x > 65535 { give -1 }

  give x
}
```

Lecture ligne par ligne:
1. `proc parse_port(x: int) -> int {` -> pose une règle clair de fonction.
2. `if x < 0 { give -1 }` -> sépare nominal et cas d'erreur.
3. `if x > 65535 { give -1 }` -> sépare nominal et cas d'erreur.
4. `give x` -> renvoie la sortie vérifiable.
5. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
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
// Scenario conventions: execution complete et verifiable
space demo/conventions

form ModuleSpec { name_len: int major: int minor: int }
pick Resolve { case Loaded(path_code: int) case Missing(code: int) }

proc normalize_len(n: int) -> int {

  if n <= 0 { give 0 }

  give n
}

// Résolution: valide la référence et produit une décision de chargement
proc resolve(m: ModuleSpec) -> Resolve {
  let n: int = normalize_len(m.name_len)

  if n == 0 { give Resolve.Missing(71) }

  if m.major <= 0 { give Resolve.Missing(72) }

  if m.minor < 0 { give Resolve.Missing(73) }
  let code: int = 100 + (m.major * 10) + m.minor

  give Resolve.Loaded(code)
}

// Conversion finale vers un code de sortie
proc to_exit(r: Resolve) -> int {

  match r {
    case Loaded(_) { give 0 }
    case Missing(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let m: ModuleSpec = ModuleSpec(8, 1, 2)
  let r: Resolve = resolve(m)

  return to_exit(r)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `normalize_len`: lit `n: int` et renvoie `int`.
- `resolve`: lit `m: ModuleSpec` et renvoie `Resolve`.
- `to_exit`: lit `r: Resolve` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `resolve` est appelé pour traiter l'étape suivante.
3. `to_exit` est appelé pour traiter l'étape suivante.
4. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `n <= 0`.
- Test évalué: `n == 0`.
- Test évalué: `m.major <= 0`.
- Test évalué: `m.minor < 0`.
- Sélection par `match r`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> resolve -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> resolve renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `normalize_len` lit `n: int` puis renvoie `int`.
- `resolve` lit `m: ModuleSpec` puis renvoie `Resolve`.
- `to_exit` lit `r: Resolve` puis renvoie `int`.

- Variables créées (valeur initiale):
- `n: int` démarre avec `normalize_len(m.name_len)`.
- `code: int` démarre avec `100 + (m.major * 10) + m.minor`.
- `m: ModuleSpec` démarre avec `ModuleSpec(8, 1, 2)`.
- `r: Resolve` démarre avec `resolve(m)`.

- Conditions qui changent le chemin:
- si `n <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `n == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `m.major <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `m.minor < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: n=normalize_len(m.name_len) -> code=100 + (m.major * 10) + m.minor -> m=ModuleSpec(8, 1, 2) -> r=resolve(m)
- enchaînement: resolve -> to_exit
- sortie finale sur ce chemin: `to_exit(r)`.

- Trace d'erreur (valeurs exemple):
- si `n <= 0` devient vrai, la fonction renvoie immédiatement `0`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

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
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
