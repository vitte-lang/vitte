# Mot-clé `space`

Niveau: Intermédiaire.

## Définition

`space` déclare l’espace de module courant au niveau top-level.

## Syntaxe

Forme canonique: `space module/path`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour fixer explicitement le module propriétaire des déclarations qui suivent.
- Quand l’éviter: dans un bloc (`proc`, `entry`), car `space` n’est pas une instruction.

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

## Pièges

- Écrire `space` comme un bloc.
- Changer de module au milieu d’un fichier sans le documenter.
- Mélanger séparateurs de chemin sans cohérence.

## Utilisé dans les chapitres

- `docs/book/chapters/03-projet.md`.
- `docs/book/chapters/09-modules.md`.
- `docs/book/chapters/29-style.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pull.md`.
- `docs/book/keywords/use.md`.
- `docs/book/chapters/09-modules.md`.
