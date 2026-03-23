# 71. Projet complet service système

Niveau: Avancé

Prérequis: `book/chapters/23-projet-sys.md`, `book/chapters/65-contrats-abi-interop-native.md`.
Voir aussi: à définir.

## Objectif

Concevoir un service système fiable (demarrage, supervision, erreurs, sécurité de base).

## Livrables

1. Cycle de vie service explicite.
2. Gestion des erreurs de runtime.
3. Journalisation operationnelle.
4. Tests de résilience minimaux.

## Exemples progressifs (N1 -> N3)

### N1 (base): service nominal

Snippet Vitte:

```vit
proc service_start(ok: bool) -> int {
  if not ok { give 1 }
  give 0
}
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): gestion d'échec explicite

Snippet Vitte:

```vit
proc service_tick(healthy: bool) -> int {
  if not healthy { give 2 }
  give 0
}
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): cycle de vie + surveillance

Snippet Vitte:

```vit
entry main at app/service {
  let code: int = service_start(true)
  if code != 0 { return code }
  return service_tick(true)
}
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
Service sans chemin d'échec explicite, sans supervision et sans tests de résilience.
```

## Validation rapide

1. Vérifier cycle de vie start/tick.
2. Vérifier gestion d'échec.
3. Vérifier checks CI et runbook d'exploitation.

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
