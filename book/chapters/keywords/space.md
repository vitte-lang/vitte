# Mot-clé `space`

Niveau: Intermédiaire.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`space` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`space` déclare l’espace de module courant au niveau top-level.

## Syntaxe

Forme canonique: `space module/path`.

## Exemple nominal

Entrée:
- Déclaration top-level valide.

```vit
space app/core
proc run() -> int {
  give 0
}
```

Sortie observable:
- Le module courant est `app/core`; les déclarations suivantes y sont rattachées.

## Exemple invalide

Entrée:
- Cas hors grammaire (nom manquant).

```vit
space
proc run() -> int {
  give 0
}
# invalide: `space` exige un `module_path`.
```

Sortie observable:
- Le parseur rejette la déclaration.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `space` fixe le module courant; `pull` importe un autre module. |

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

- Écrire `space` comme un bloc.
- Changer de module au milieu d’un fichier sans le documenter.
- Mélanger séparateurs de chemin sans cohérence.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour fixer explicitement le module propriétaire des déclarations qui suivent.
- Quand l’éviter: dans un bloc (`proc`, `entry`), car `space` n’est pas une instruction.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near space` | Chemin module absent ou mal formé. | Fournir un `module_path` valide (`a/b` ou `a.b`). |
| `unexpected statement` | `space` placé dans un bloc. | Remonter la déclaration au niveau top-level. |
| `invalid module path` | Segments invalides. | Utiliser uniquement des identifiants valides. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `space` fixe le module courant; `pull` importe un autre module. |

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pull.md`.
- `docs/book/keywords/use.md`.
- `docs/book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
