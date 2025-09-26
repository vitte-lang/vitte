# RFC 0033 — Augmented Reality & Virtual Reality

## Résumé
Ce RFC définit l’intégration de la **réalité augmentée (AR)** et de la **réalité virtuelle (VR)** dans le langage **Vitte**.  
Objectif : permettre aux développeurs de créer des expériences immersives et interactives en utilisant des API unifiées, performantes et sûres.

## Motivation
AR et VR deviennent incontournables dans :  
- le gaming,  
- l’éducation et la formation,  
- l’architecture et le design,  
- les simulateurs et jumeaux numériques,  
- le métavers et les environnements sociaux.  

Vitte doit proposer des outils modernes pour exploiter ce domaine.

## Design proposé

### Module `std::xr`
- Gestion des scènes, caméras, objets 3D.  
- Support AR (overlay sur le monde réel) et VR (environnements immersifs).  

```vitte
use std::xr::{Scene, Camera, Object3D}

fn main() {
    let mut scene = Scene::new()
    let cam = Camera::default()
    let cube = Object3D::cube(size=1.0)
    scene.add(cube)
    scene.render(cam)
}
```

### Entrées utilisateurs XR
- Support des contrôleurs VR, gestes AR, eye tracking.  

```vitte
use std::xr::input

fn main() {
    if input::gesture("pinch") {
        print("User pinched")
    }
}
```

### Rendu optimisé
- Utilisation de Vulkan/Metal/DirectX via `std::gfx`.  
- Reprojection et rendu stéréoscopique.  

### AR — ancrage et suivi
- Détection de plans, suivi SLAM.  
- Exemple :  

```vitte
use std::xr::ar

fn main() {
    let plane = ar::detect_plane()
    let obj = Object3D::sphere(0.5)
    ar::anchor(obj, plane)
}
```

### VR — environnements immersifs
- Chargement de mondes complexes.  
- Exemple :  

```vitte
use std::xr::vr

fn main() {
    let world = vr::load("scene.glb")
    vr::enter(world)
}
```

### Audio spatial
- Module `std::xr::audio`.  
- Exemple :  

```vitte
use std::xr::audio

fn main() {
    audio::play("ambient.ogg", position=(0,0,5))
}
```

### Exemple complet : expérience AR minimaliste
```vitte
use std::xr::{Scene, Camera, Object3D, ar}

fn main() {
    let mut scene = Scene::new()
    let cam = Camera::ar_mode()
    let plane = ar::detect_plane()
    let cube = Object3D::cube(0.3)
    ar::anchor(cube, plane)
    scene.add(cube)
    scene.render(cam)
}
```

## Alternatives considérées
- **Pas de support XR** : rejeté, domaine en forte croissance.  
- **Passer uniquement par moteurs externes (Unity/Unreal)** : rejeté, manque d’intégration native.  
- **Support limité à VR** : rejeté, AR est complémentaire.  

## Impact et compatibilité
- Impact fort : Vitte devient un langage viable pour XR natif.  
- Compatible avec OpenXR, WebXR, Vulkan.  
- Peut séduire le secteur gaming et industriel.  

## Références
- [OpenXR](https://www.khronos.org/openxr/)  
- [WebXR](https://immersiveweb.dev/)  
- [Unity XR](https://unity.com/xr)  
- [Unreal Engine VR](https://www.unrealengine.com/)  
