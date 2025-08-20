# RFC 0031 — Energy Efficiency and Green Computing

## Résumé
Ce RFC définit les principes et outils permettant d’écrire du code **éco-efficace** en **Vitte**, avec une consommation énergétique réduite.  
Objectif : rendre Vitte adapté aux systèmes contraints, aux datacenters, et à la transition écologique du numérique.

## Motivation
La consommation énergétique des logiciels est devenue un enjeu majeur :  
- serveurs et datacenters → forte empreinte carbone,  
- systèmes embarqués et IoT → batteries limitées,  
- optimisation nécessaire pour les architectures modernes.  

Un langage moderne doit intégrer l’**efficacité énergétique** comme principe de conception.

## Design proposé

### Profilage énergétique
- Outil intégré :  
```sh
vitpm energy profile my_app.vitte
```  
- Mesure CPU, mémoire, I/O, GPU.  
- Export JSON/CSV pour intégration CI/CD.  

### Annotations `#[energy_hint]`
- Permet de guider le compilateur vers des choix optimisés :  
```vitte
#[energy_hint(low_power)]
fn boucle() {
    for i in 0..1000 { work() }
}
```

### Mode de compilation
- `vitpm build --opt energy` → priorise efficacité énergétique.  
- Variantes : `--opt perf`, `--opt balance`.  

### API `std::energy`
- Monitoring en temps réel.  
- Exposition d’événements basse énergie.  

```vitte
use std::energy

fn main() {
    let usage = energy::current()
    print("Energy usage: {}", usage)
}
```

### Gestion mémoire optimisée
- Analyse statique pour limiter les copies inutiles.  
- Recyclage d’allocations (object pooling).  

### Concurrence et énergie
- Scheduler aware : privilégie cores basse conso.  
- Async tasks suspendues quand inactives.  

### Exemple complet : serveur éco-efficace
```vitte
use std::net
use std::energy

#[energy_hint(low_power)]
fn handle(conn: net::Connection) {
    let req = conn.read()
    conn.write("OK")
}

fn main() {
    let server = net::Server::bind("0.0.0.0:8080")
    server.run(handle)
    print("Energy: {}", energy::current())
}
```

## Alternatives considérées
- **Ignorer énergie** : rejeté, contre les enjeux actuels.  
- **Passer uniquement par matériel** : rejeté, les optimisations logicielles comptent.  
- **Outils externes uniquement** : rejeté, intégration native nécessaire.  

## Impact et compatibilité
- Impact fort : adoption dans IoT, embarqué, edge computing, cloud vert.  
- Compatible avec optimisations classiques (`--opt perf`).  
- Peut influencer priorités du compilateur (tradeoff performance/énergie).  

## Références
- [Green Software Foundation](https://greensoftware.foundation/)  
- [Carbon Aware SDK](https://github.com/Green-Software-Foundation/carbon-aware-sdk)  
- [Energy-efficient computing](https://ieeexplore.ieee.org/document/7967175)  
