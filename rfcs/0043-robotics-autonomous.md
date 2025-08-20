# RFC 0043 — Robotics & Autonomous Systems

## Résumé
Ce RFC définit l’intégration de la **robotique** et des **systèmes autonomes** dans le langage **Vitte**.  
Objectif : offrir une API unifiée et performante pour le contrôle des robots, la navigation autonome, la perception sensorielle et l’intégration avec l’IA embarquée.

## Motivation
Les robots sont au cœur de multiples domaines :  
- Industrie 4.0 (automatisation, bras robotisés),  
- Transport (véhicules autonomes, drones),  
- Santé (chirurgie assistée, exosquelettes),  
- Défense et exploration spatiale.  

Vitte doit fournir une fondation standardisée pour développer des systèmes robotiques sûrs et interopérables.

## Design proposé

### Module `std::robot`
- Contrôle des actionneurs et capteurs.  
- Abstraction multi-plateformes (ROS, microcontrôleurs, simulateurs).  

```vitte
use std::robot::{Motor, Sensor}

fn main() {
    let m = Motor::new("wheel-left")
    let s = Sensor::new("lidar")
    m.set_speed(0.5)
    print("Distance: {}", s.read())
}
```

### Navigation autonome
- Module `robot::nav` avec algorithmes : SLAM, A*.  

```vitte
use std::robot::nav

fn main() {
    let map = nav::slam("lidar")
    let path = nav::astar(map, start=(0,0), goal=(5,5))
    print("Chemin trouvé: {:?}", path)
}
```

### Perception avancée
- Vision par ordinateur, reconnaissance objets.  
- Intégration IA embarquée (`std::ai`).  

```vitte
use std::ai::model
use std::robot::camera

fn main() {
    let img = camera::capture()
    let m = model::load("object-detector.tflite")
    print("Objets détectés: {}", m.run(img))
}
```

### Exemple complet : drone autonome
```vitte
use std::robot::{Motor, Sensor, nav}
use std::ai::model

fn main() {
    let lidar = Sensor::new("lidar")
    let motors = [Motor::new("front"), Motor::new("back")]
    let map = nav::slam("lidar")
    let path = nav::astar(map, (0,0), (10,10))

    for step in path {
        if lidar.read() < 1.0 {
            motors[0].stop()
            motors[1].stop()
            break
        }
        motors[0].set_speed(0.8)
        motors[1].set_speed(0.8)
    }
}
```

## Alternatives considérées
- **Support limité (ROS seulement)** : rejeté, manque de flexibilité.  
- **API purement bas-niveau** : rejeté, pas ergonomique.  
- **Librairies externes uniquement** : rejeté, besoin d’intégration native.  

## Impact et compatibilité
- Impact fort : adoption par robotique académique et industrielle.  
- Compatible ROS2, Gazebo, microROS.  
- Prépare Vitte aux systèmes cyber-physiques.  

## Références
- [ROS2](https://docs.ros.org/en/foxy/index.html)  
- [Gazebo Simulator](https://gazebosim.org/home)  
- [SLAM Algorithms](https://en.wikipedia.org/wiki/Simultaneous_localization_and_mapping)  
- [IEEE Robotics & Automation Society](https://www.ieee-ras.org/)  
