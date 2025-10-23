

# vitte-test

Framework **de tests unitaires, d’intégration et de validation** pour le langage Vitte.  
`vitte-test` fournit un système modulaire de test couvrant la compilation, l’exécution et la vérification des résultats, avec intégration directe dans le runtime et le CLI.

---

## Objectifs

- Offrir une **infrastructure de test complète et rapide** pour tout projet Vitte.  
- Supporter les tests unitaires, d’intégration et de régression.  
- Intégration directe avec `vitte-cli`, `vitte-compiler`, `vitte-runtime` et `vitte-bench`.  
- Fournir des assertions expressives et des rapports détaillés.  
- Support de la parallélisation et de l’exécution sélective des tests.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `runner`      | Gestionnaire principal d’exécution des tests |
| `assert`      | Ensemble d’assertions (égalité, panique, flot, temps) |
| `report`      | Génération de rapports (console, JSON, JUnit, HTML) |
| `discover`    | Détection automatique des tests dans les projets |
| `sandbox`     | Exécution isolée et sécurisée des tests Vitte |
| `bench`       | Tests de performance et benchmarks intégrés |
| `tests`       | Auto-validation du moteur de test |

---

## Exemple d’utilisation

### Exemple en Vitte

```vitte
#[test]
fn addition_basique() {
    assert_eq(2 + 2, 4);
}

#[test]
fn division_par_zero() {
    assert_panic(|| {
        let _ = 1 / 0;
    });
}
```

### Exécution des tests

```bash
vitte test
```

### Sortie exemple

```
Running 2 tests
✓ addition_basique ... OK
✓ division_par_zero ... PANIC détectée

Résultat : 2 tests, 0 échec, 1 panic contrôlée (0.004s)
```

---

## Intégration

- `vitte-cli` : commande `vitte test` avec filtres et options.  
- `vitte-compiler` : compilation automatique des fichiers de test.  
- `vitte-runtime` : exécution isolée et sandboxée.  
- `vitte-bench` : intégration des tests de performance.  
- `vitte-studio` : affichage graphique des rapports et résultats.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
regex = "1.10"
chrono = "0.4"
rayon = "1.10"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `prettytable` pour affichage formaté, `criterion` pour benchmarks, `indicatif` pour progression CLI.

---

## Tests

```bash
cargo test -p vitte-test
```

- Tests unitaires et d’intégration.  
- Tests de compatibilité avec le runtime et le compilateur.  
- Tests de tolérance d’erreur et sandbox.  
- Benchmarks sur grands ensembles de tests.

---

## Roadmap

- [ ] Export des rapports en JUnit/HTML.  
- [ ] Mode watch (`vitte test --watch`).  
- [ ] Support complet des benchmarks.  
- [ ] Intégration CI/CD (GitHub Actions, GitLab CI).  
- [ ] Visualisation des résultats dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau