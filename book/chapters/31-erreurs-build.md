# 31. Erreurs de build

Niveau: Avancé.

Prérequis: chapitre précédent `book/chapters/30-faq.md` et `docs/book/grammar-surface.ebnf`.
Voir aussi: `book/chapters/30-faq.md`, `docs/book/chapters/16-interop.md`, `book/chapters/23-projet-sys.md`.

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

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

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
| `undefined symbol` (clang/link) | backend/clang | runtime/symbole natif absent. | Ajouter définition/link/runtime manquant. |
| `clang invocation failed` | backend | génération ou toolchain cassée. | Inspecter `vitte_out.cpp` et flags. |

## 31.3 Minimal reproducers

<<< minimal reproducer parse >>>

```vit
emit 1
# casse: statement top-level.
```

Lecture ligne par ligne (débutant):
1. `emit 1` -> Comportement: statement placé au top-level. -> Preuve: le parseur attend une déclaration top-level et rejette ce fichier.
2. `# casse: statement top-level.` -> Comportement: commentaire de diagnostic local. -> Preuve: le message explique pourquoi l'erreur vient de la grammaire, pas du linker.

Fix:

```vit
entry main at app/repro {
  emit 1
  return 0
}
```

Lecture ligne par ligne (débutant):
1. `entry main at app/repro {` -> Comportement: ouvre un bloc exécutable valide au top-level. -> Preuve: la structure respecte la grammaire.
2. `emit 1` -> Comportement: statement désormais dans un bloc autorisé. -> Preuve: le parseur accepte cette ligne.
3. `return 0` -> Comportement: termine explicitement le scénario. -> Preuve: la sortie est déterministe pour le test.
4. `}` -> Comportement: ferme proprement le bloc d'entrée. -> Preuve: le fichier est syntaxiquement complet.

<<< minimal reproducer link >>>

```vit
entry main at app/repro {
  return native_missing(1)
}
# parse OK, link KO: symbole absent.
```

Lecture ligne par ligne (débutant):
1. `entry main at app/repro {` -> Comportement: la forme top-level est valide. -> Preuve: le parse passe.
2. `return native_missing(1)` -> Comportement: appel d'un symbole externe non fourni. -> Preuve: l'échec survient au link avec `undefined symbol`.
3. `}` -> Comportement: fin correcte du bloc. -> Preuve: la cause n'est pas syntaxique mais runtime/édition de liens.

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

- `docs/book/keywords/emit.md`.
- `docs/book/keywords/use.md`.
- `docs/book/keywords/space.md`.
- `docs/book/keywords/make.md`.
- `docs/book/keywords/share.md`.

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
