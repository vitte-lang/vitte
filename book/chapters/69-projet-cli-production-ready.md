# 69. Projet complet CLI production-ready

Niveau: Intermédiaire

Prérequis: `book/chapters/21-projet-cli.md`, `book/chapters/58-pipeline-ci-projet-vitte.md`.
Voir aussi: à définir.

## Objectif

Construire une CLI robuste avec packaging, tests, diagnostics et CI.

## Livrables

1. Commandes principales et aide.
2. Gestion d'erreurs utilisateur claire.
3. Tests unitaires + intégration.
4. Pipeline CI vert.

## Exemples progressifs (N1 -> N3)

### N1 (base): commande CLI nominale

Snippet Vitte:

```vit
entry main at app/cli {
  return 0
}
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): erreur utilisateur contrôlée

Snippet Vitte:

```vit
proc parse_arg(ok: bool) -> int {
  if not ok { give 2 }
  give 0
}
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): exécution complète CI

Snippet Vitte:

```vit
proc run_cli(ok: bool) -> int {
  if not ok { give 2 }
  give 0
}
entry main at app/cli { return run_cli(true) }
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
CLI sans code de sortie stable, sans tests de cas d'erreur, sans validation CI.
```

## Validation rapide

1. Vérifier codes de sortie nominal/erreur.
2. Vérifier commandes CI vertes.
3. Vérifier documentation utilisateur minimale.

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
