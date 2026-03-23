# 31. Erreurs de build

Niveau: Avancé.

Prérequis: chapitre précédent `book/chapters/30-faq.md` et `book/grammar-surface.ebnf`.
Voir aussi: `book/chapters/30-faq.md`, `book/chapters/16-interop.md`, `book/chapters/23-projet-sys.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Erreurs de build**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Erreurs de build**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Erreurs de build**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Erreurs de build** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Erreurs de build**.

## Exercice court

Prenez un exemple du chapitre sur **Erreurs de build**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Erreurs de build**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local triage pipeline: Avancé.
- Niveau local correction syntaxique: Intermédiaire.
- Niveau local runtime/ABI: Avancé.

Ce chapitre sert de guide opérationnel pour diagnostiquer un échec de build Vitte en moins de cinq minutes, de la première erreur brute jusqu'au patch validé.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `book/checklist-editoriale.md`. Complément: `book/erreurs-classiques.md`.

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
| `type mismatch` | resolve/ir | contrat de type incohérent. | Aligner type attendu et type fourni. |
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
- `E001x` (famille type mismatch): contrat de type violé.
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

- `book/keywords/emit.md`.
- `book/keywords/use.md`.
- `book/keywords/space.md`.
- `book/keywords/make.md`.
- `book/keywords/share.md`.

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

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.


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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas limite, verrouiller la garde au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
