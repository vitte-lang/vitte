# 31. Erreurs de build

Niveau: Avancé.

Prérequis: chapitre précédent `docs/book/chapters/30-faq.md` et `docs/book/grammar-surface.ebnf`.
Voir aussi: `docs/book/chapters/30-faq.md`, `docs/book/chapters/16-interop.md`, `docs/book/chapters/23-projet-sys.md`.

## Problème Concret

Situation réelle: Erreurs de build se comprend mieux en rejouant le programme comme un algorithme exécutable. Vous lisez les données entrantes, la condition évaluée, puis la valeur renvoyée.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Erreurs de build**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Erreurs de build** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Erreurs de build**.

## Exercice court

Prenez un exemple du chapitre sur **Erreurs de build**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Erreurs de build**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local triage pipeline: Avancé.
- Niveau local correction syntaxique: Intermédiaire.
- Niveau local runtime/ABI: Avancé.

Ce chapitre sert de guide opérationnel pour diagnostiquer un échec de build Vitte en moins de cinq minutes, de la première erreur brute jusqu'au patch validé.

Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

## 31.1 Matrice mot-clé -> top-level/stmt/expr

| Mot-clé | Top-level | Stmt | Expr |
| --- | --- | --- | --- |
| `space` | oui | non | non |
| `pull` | oui | non | non |
| `use` | oui | non | non |
| `share` | oui | non | non |
| `const` | oui | non | non |
| `make` | oui (global) | oui (local) | non |
| `let` | oui (global) | oui (local) | non |
| `emit` | non | oui | non |
| `return` | non | oui | non |

## 31.2 Erreurs pipeline réelles

| Message | Couche | Cause fréquente | Correction minimale |
| --- | --- | --- | --- |
| `expected top-level declaration` | parse | statement au top-level. | Encapsuler dans `entry`/`proc`. |
| `unknown symbol` | resolve | symbole non déclaré/importé. | Ajouter déclaration/import correct. |
| `type mismatch` | resolve/ir | règle de type incohérent. | Aligner type attendu et type fourni. |
| `undefined symbol` (link) | backend/cpp | runtime/symbole natif absent. | Ajouter définition/link/runtime manquant. |
| `toolchain invocation failed` | backend | génération ou toolchain cassée. | Inspecter `vitte_out.cpp` et flags. |

## 31.3 Minimal reproducers

<<< minimal reproducer parse >>>

```vit
emit 1
# casse: statement top-level.
```

Lecture ligne par ligne (débutant):
1. `emit 1` : statement placé au top-level.
2. `# casse: statement top-level.` : commentaire de diagnostic local.

Fix:

```vit
entry main at app/repro {
  emit 1

  return 0
}
```

Lecture ligne par ligne (débutant):
1. `entry main at app/repro {` : ouvre un bloc exécutable valide au top-level.
2. `emit 1` : statement désormais dans un bloc autorisé.
3. `return 0` : termine explicitement le scénario.
4. `}` : ferme proprement le bloc d'entrée.

<<< minimal reproducer link >>>

```vit
entry main at app/repro {
  return native_missing(1)
}
# parse OK, link KO: symbole absent.
```

Lecture ligne par ligne (débutant):
1. `entry main at app/repro {` : la forme top-level est valide.
2. `return native_missing(1)` : appel d'un symbole externe non fourni.
3. `}` : fin correcte du bloc.

Fix: fournir `native_missing` dans le runtime ou remplacer l'appel.

## 31.4 ABI/runtime checklist (commun kernel/interop)

Checklist commune:
- signatures ABI cohérentes entre Vitte et natif.
- symboles runtime exportés et linkés.
- headers/runtime présents (`vitte_runtime.hpp`, types de base).
- mode freestanding respecté (pas de dépendance userland implicite).
- conventions d'appel homogènes sur toutes les cibles.

## 31.5 Glossaire erreurs fréquentes

- `E0007`: forme syntaxique inattendue au top-level.
- `E001x` (famille type mismatch): règle de type violé.
- `undefined symbol`: symbole absent à l'édition de liens.
- `invalid module path`: chemin module/import mal formé.

## 31.6 Playbook 5 minutes

1. Lire la première erreur et identifier la couche.
2. Réduire le fichier au plus petit reproducer.
3. Corriger une seule cause locale.
4. Relancer `vitte build`.
5. Valider la doc avec `qa_book.py` puis `qa_book.py --strict`.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/chapters/keywords/emit.md`.
- `docs/book/chapters/keywords/use.md`.
- `docs/book/chapters/keywords/space.md`.
- `docs/book/chapters/keywords/make.md`.
- `docs/book/chapters/keywords/share.md`.

## Objectif

Savoir mapper rapidement un message d'erreur à la bonne couche du pipeline pour corriger vite et proprement.

## Exemple

Exemple concret: un `emit` top-level échoue au parseur; le même `emit` dans `entry` compile.

## Pourquoi

Un triage par couche évite les corrections aléatoires et réduit les régressions.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la première couche fautive rejette tôt, puis la correction est appliquée localement et vérifiée par rerun pipeline.

## À faire

1. Créez un reproducer parse et corrigez-le.
2. Créez un reproducer link et documentez le symbole manquant.
3. Exécutez le playbook 5 minutes sur un incident réel.

## Corrigé minimal

- Parse: remettre la forme grammaticale conforme.
- Resolve: corriger imports/symboles/types.
- Backend/link: corriger runtime/ABI/toolchain.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **erreurs de build**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
emit 1
# casse: statement top-level.
```

Lecture ligne par ligne:
1. `emit 1` -> participe au déroulé du traitement.
2. `# casse: statement top-level.` -> participe au déroulé du traitement.

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
// Scenario erreurs build: execution complete et verifiable
space demo/erreurs-build

form Event { code: int severity: int payload_len: int }
pick Diagnostic { case Info(code: int) case Warn(code: int) case Error(code: int) }

// Classification: mappe un événement vers un niveau explicite
proc classify(e: Event) -> Diagnostic {

  if e.code == 0 { give Diagnostic.Info(0) }

  if e.severity <= 2 { give Diagnostic.Warn(e.code) }

  give Diagnostic.Error(e.code)
}

// Redaction: borne la charge utile avant diffusion
proc redact(e: Event) -> int {

  if e.payload_len < 0 { give 81 }

  if e.payload_len > 4096 { give 82 }

  give 0
}

proc handle(e: Event) -> int {
  let r: int = redact(e)

  if r != 0 { give r }
  let d: Diagnostic = classify(e)

  match d {
    case Info(_) { give 0 }
    case Warn(_) { give 0 }
    case Error(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: Event = Event(17, 3, 120)

  return handle(e)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `classify`: lit `e: Event` et renvoie `Diagnostic`.
- `redact`: lit `e: Event` et renvoie `int`.
- `handle`: lit `e: Event` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `handle` est appelé pour traiter l'étape suivante.
3. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `e.code == 0`.
- Test évalué: `e.severity <= 2`.
- Test évalué: `e.payload_len < 0`.
- Test évalué: `e.payload_len > 4096`.
- Test évalué: `r != 0`.
- Sélection par `match d`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> handle -> sortie 0`.
- Exemple erreur: `entrée invalide -> handle renvoie un code d'erreur -> sortie non nulle`.

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
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `classify` lit `e: Event` puis renvoie `Diagnostic`.
- `redact` lit `e: Event` puis renvoie `int`.
- `handle` lit `e: Event` puis renvoie `int`.

- Variables créées (valeur initiale):
- `r: int` démarre avec `redact(e)`.
- `d: Diagnostic` démarre avec `classify(e)`.
- `e: Event` démarre avec `Event(17, 3, 120)`.

- Conditions qui changent le chemin:
- si `e.code == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.severity <= 2` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_len < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_len > 4096` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `r != 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: r=redact(e) -> d=classify(e) -> e=Event(17, 3, 120)
- enchaînement: handle
- sortie finale sur ce chemin: `handle(e)`.

- Trace d'erreur (valeurs exemple):
- si `e.code == 0` devient vrai, la fonction renvoie immédiatement `Diagnostic.Info(0)`.

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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas d'erreur, verrouiller le test au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
