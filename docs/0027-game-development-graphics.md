# RFC 0027 — Game Development and Graphics

## Résumé
Ce RFC définit le support du **développement de jeux vidéo** et des **graphiques** dans le langage **Vitte**.  
Objectif : fournir un ensemble d’API et d’outils pour créer des moteurs de jeux, des jeux 2D/3D et des applications graphiques interactives.

## Motivation
Le jeu vidéo est un domaine clé pour la popularité d’un langage :  
- besoin de performances natives,  
- accès direct au GPU (Vulkan, Metal, DirectX, OpenGL),  
- outils pour la 2D, la 3D, l’audio et la physique,  
- intégration avec moteurs existants.  

## Design proposé

### Module `std::gfx`
- Gestion du rendu GPU.  
- API unifiée multi-backend : Vulkan, OpenGL, Metal, DirectX.  

```vitte
use std::gfx

fn main() {
    let win = gfx::Window::new("Hello Vitte", 800, 600)
    let ctx = win.context()
    ctx.clear_color(0.1, 0.2, 0.3, 1.0)
    win.run()
}
```

### Module `std::gfx::2d`
- Sprites, textures, animations.  
- Exemple :  
```vitte
use std::gfx::2d

fn main() {
    let win = gfx::Window::new("2D Game", 640, 480)
    let player = Sprite::load("player.png")
    win.run_loop(|ctx| {
        ctx.draw(&player, (100, 200))
    })
}
```

### Module `std::gfx::3d`
- Meshes, shaders, caméras, lumières.  
- Exemple cube rotatif :  
```vitte
use std::gfx::3d

fn main() {
    let win = gfx::Window::new("3D Cube", 800, 600)
    let cube = Mesh::cube()
    win.run_loop(|ctx| {
        ctx.rotate(&cube, (0.0, 1.0, 0.0))
        ctx.draw(&cube)
    })
}
```

### Audio et Physique
- Module `std::audio` pour sons et musique.  
- Module `std::physics` pour collisions et dynamiques.  

```vitte
use std::audio

fn main() {
    let s = audio::Sound::load("explosion.wav")
    s.play()
}
```

### Input et Contrôles
- Gestion clavier, souris, manettes.  
```vitte
use std::input

fn main() {
    let key = input::wait_key()
    print("Key pressed: {}", key)
}
```

### Game Engine Framework (`vitengine`)
- ECS (Entity Component System).  
- Scènes et ressources.  
- Scripting en Vitte ou via API externe.  

### Exemple complet : jeu minimal
```vitte
use vitengine::{Game, Sprite}

fn main() {
    Game::new("Vitte Pong", |ctx| {
        let ball = Sprite::load("ball.png")
        ctx.run_loop(|c| {
            c.draw(&ball, (320,240))
        })
    }).run()
}
```

### CI/CD et déploiement
- `vitpm build --target wasm32` → jeu jouable dans navigateur.  
- `vitpm build --target aarch64-apple-ios` → app iOS.  
- `vitpm build --target windows` → .exe jouable.  

## Alternatives considérées
- **Pas de support graphique** : rejeté, trop limitant.  
- **Passer uniquement par moteurs externes (Unity/Unreal)** : rejeté, manque d’identité propre.  
- **API propriétaire unique** : rejeté, besoin de multiplateforme.  

## Impact et compatibilité
- Impact majeur : ouvre Vitte au développement de jeux.  
- Compatible avec moteurs externes (Unreal, Godot) via bindings.  
- Permet de créer un écosystème Vitte pour jeux indés et AAA.  

## Références
- [SDL2](https://libsdl.org/)  
- [Vulkan API](https://www.vulkan.org/)  
- [Godot Engine](https://godotengine.org/)  
- [Unity DOTS ECS](https://unity.com/dots)  
