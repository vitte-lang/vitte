# Mot-clé `field`

Niveau: Intermédiaire.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`field` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`field` est utilisé dans la forme legacy de `form`/`trait` avec la syntaxe `as`.

## Syntaxe

Forme canonique: `field nom as type` (dans `legacy_form`).

## Exemple nominal

Entrée:
- Déclaration legacy valide.

```vit
form User
  field id as int
  field name as string
.end
```

Sortie observable:
- La structure est acceptée avec deux champs typés.

## Exemple invalide

Entrée:
- Forme mélangeant syntaxes.

```vit
form User
  field id: int
.end
# invalide: en mode legacy, `field` utilise `as`.
```

Sortie observable:
- Le parseur rejette la ligne de champ.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `form` | `form` déclare le conteneur; `field` déclare un champ legacy. |

## Refactor rapide

Avant:
```vit
# usage fragile à corriger
```

Après:
```vit
# usage clair et testable
```

## Pièges

- Utiliser `field` avec `:`.
- Mélanger `.end` et `{ ... }`.
- Oublier la migration vers la forme moderne.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour maintenir du code legacy `.end`.
- Quand l’éviter: dans la forme brace moderne (`name: type`).

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near field` | Champ hors contexte legacy. | Utiliser `field ... as ...` dans `.end`. |
| `missing type after as` | Type absent. | Ajouter `as type_expr`. |
| `mixed form styles` | Mélange brace/legacy. | Choisir un style unique par déclaration. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `form` | `form` déclare le conteneur; `field` déclare un champ legacy. |

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/trait.md`.
- `docs/book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
