# RFC 0017 — Testing and Benchmarking

## Résumé
Ce RFC définit le système de **tests** et de **benchmarks** intégré au langage **Vitte**.  
Il fournit un cadre unifié pour l’écriture, l’exécution et l’automatisation des tests unitaires, d’intégration et de performance.

## Motivation
Un langage moderne doit embarquer un système de test robuste :  
- éviter la dépendance à des frameworks externes,  
- garantir la qualité logicielle et la stabilité des bibliothèques,  
- fournir des outils standardisés pour CI/CD,  
- permettre des mesures de performance reproductibles.  

## Design proposé

### Tests unitaires
Les tests s’écrivent dans les modules avec l’attribut `@test`.  

```vitte
@test
fn addition() {
    assert_eq!(2 + 2, 4)
}
```

### Tests d’intégration
Les tests peuvent être placés dans un dossier `tests/`.  

```
mon_projet/
 ├── src/
 └── tests/
      ├── basic.vit
      └── network.vit
```

Exemple :  
```vitte
@test
fn test_http() {
    let resp = http::get("https://example.com")
    assert!(resp.status == 200)
}
```

### Assertions
- `assert!(cond)` : vérifie une condition.  
- `assert_eq!(a, b)` : compare deux valeurs.  
- `assert_ne!(a, b)` : vérifie que les valeurs diffèrent.  

### Benchmarks
L’attribut `@bench` permet d’évaluer la performance.  

```vitte
@bench
fn tri() {
    let mut v = [5, 3, 1, 4, 2]
    v.sort()
}
```

### Exécution des tests et benchmarks
- `vitpm test` exécute tous les tests.  
- `vitpm test --filter nom` exécute un sous-ensemble.  
- `vitpm bench` exécute les benchmarks avec mesures précises.  

### Rapports
- Sortie par défaut en texte.  
- Option `--format=json` pour CI/CD.  
- Génération possible de rapports HTML.  

### Exemple d’utilisation CI
```yaml
- run: vitpm test --format=json > results.json
- run: vitpm bench --output bench.html
```

## Alternatives considérées
- **Framework externe uniquement** : rejeté pour cohérence et accessibilité.  
- **Tests uniquement via macros** : rejetés pour manque de lisibilité.  
- **Pas de benchmarks intégrés** : rejeté car essentiel pour langage orienté performance.  

## Impact et compatibilité
- Impact fort sur la fiabilité de l’écosystème.  
- Compatible avec CI/CD modernes.  
- Encourage la culture du test et de la mesure de performance.  
- Ajoute une faible complexité à `vitpm`.  

## Références
- [Rust Testing](https://doc.rust-lang.org/book/ch11-00-testing.html)  
- [Go Testing](https://pkg.go.dev/testing)  
- [JUnit](https://junit.org/)  
