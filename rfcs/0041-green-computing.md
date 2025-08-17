# RFC 0041 — Green Computing & Energy-Aware Programming

## Résumé
Ce RFC définit un cadre pour le **Green Computing** et la **programmation économe en énergie** dans le langage **Vitte**.  
Objectif : fournir des outils, API et bonnes pratiques pour optimiser la consommation énergétique des logiciels, réduire l’empreinte carbone et favoriser des applications durables.

## Motivation
Le numérique représente une part croissante des émissions mondiales de CO₂.  
- Les datacenters consomment massivement de l’électricité.  
- Les appareils mobiles nécessitent des optimisations batterie.  
- Les systèmes embarqués exigent une efficacité énergétique maximale.  

Vitte doit encourager une programmation éco-responsable par conception.

## Design proposé

### Profil d’énergie
- Nouveaux attributs pour mesurer et limiter l’énergie consommée.  

```vitte
#[energy_limit(10J)]
fn calcul() {
    // code optimisé
}
```

### Module `std::energy`
- API pour mesurer consommation CPU, mémoire, batterie.  

```vitte
use std::energy

fn main() {
    let e = energy::measure(|| {
        heavy_computation()
    })
    print("Énergie consommée: {} J", e)
}
```

### Mode d’exécution
- `--eco` : active optimisations basse consommation.  
- `--perf` : favorise performance brute.  

```sh
vitte run --eco main.vit
```

### Concurrence économe
- Ordonnancement orienté **low-power**.  
- API pour tâches adaptatives.  

```vitte
use std::energy::task

fn main() {
    task::spawn_low_power(|| {
        sensor_read()
    })
}
```

### Compilation & analyse
- Compilation avec rapports énergétiques.  
- Commande :  
```sh
vitte build --energy-report
```

### Exemple complet : application éco
```vitte
use std::energy

fn main() {
    #[energy_limit(5J)]
    fn analyse() {
        compute_matrix()
    }

    let usage = energy::measure(analyse)
    print("Analyse terminée avec {} J", usage)
}
```

## Alternatives considérées
- **Ignorer énergie** : rejeté, enjeu écologique majeur.  
- **Confier optimisation au matériel** : rejeté, pas suffisant.  
- **Monitoring externe uniquement** : rejeté, manque de granularité.  

## Impact et compatibilité
- Impact fort : adoption par mobile, embarqué, cloud.  
- Compatible avec métriques existantes (RAPL, ARM PMU).  
- Améliore image éthique et durable de Vitte.  

## Références
- [Green Software Foundation](https://greensoftware.foundation/)  
- [ACM SIGEnergy](https://energy.acm.org/)  
- [RAPL Intel](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-power-governor.html)  
