

# vitte-fuzz

Outil **de fuzzing et de test aléatoire** pour l’écosystème Vitte.

`vitte-fuzz` permet de tester la robustesse du compilateur, de l’analyseur et du runtime de Vitte via des entrées aléatoires ou semi-générées. Il vise à détecter les crashes, comportements indéfinis et régressions avant publication.

---

## Objectifs

- Génération pseudo-aléatoire de programmes Vitte valides ou presque valides.
- Tests de stabilité et de non-régression sur les modules clés.
- Intégration avec `cargo fuzz` et `vitte-analyzer`.
- Détection automatisée des crashes, panics et dépassements de mémoire.
- Génération de corpus de tests reproductibles.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `gen`          | Générateur de programmes aléatoires conformes à la grammaire |
| `runner`       | Exécution du compilateur et capture des erreurs |
| `minimize`     | Réduction automatique du cas de test fautif |
| `report`       | Génération de rapports JSON/HTML |
| `cli`          | Interface en ligne de commande |
| `tests`        | Cas de fuzz ciblés et benchmarks |

---

## Exemple d’utilisation

```bash
vitte fuzz run vitte-compiler
vitte fuzz run vitte-parser --timeout 5
vitte fuzz minimize crash_1234.vit
```

---

## Intégration

- `vitte-compiler` : vérification des parsing et codegen.
- `vitte-analyzer` : détection de comportements non définis.
- `vitte-runtime` : fuzz des primitives internes et du GC.
- `cargo fuzz` : moteur de fuzz externe compatible.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-parser = { path = "../vitte-parser", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

arbitrary = "1.3"
libfuzzer-sys = "0.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-fuzz
```

- Tests de génération pseudo-aléatoire.
- Détection de crashs connus.
- Vérification de reproductibilité des corpus.

---

## Roadmap

- [ ] Intégration de `Atheris` (Python fuzz) pour comparaison.
- [ ] Couverture LLVM pour `vitte-compiler`.
- [ ] Rapport HTML interactif avec visualisation du flux.
- [ ] Intégration continue sur GitHub Actions.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau