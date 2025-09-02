# RFC 0026 — Mobile and Cross-Platform Development

## Résumé
Ce RFC définit le support de la **programmation mobile** et du **développement multiplateforme** dans le langage **Vitte**.  
Objectif : permettre aux développeurs d’écrire des applications Android, iOS, desktop et web avec un socle unique.

## Motivation
Les développeurs veulent :  
- cibler plusieurs plateformes sans réécrire leur code,  
- partager un maximum de logique applicative,  
- garder des performances natives,  
- accéder facilement aux APIs système (caméra, GPS, capteurs).  

## Design proposé

### Compilation multiplateforme
- Cibles supportées :  
  - `aarch64-linux-android`  
  - `x86_64-pc-windows-msvc`  
  - `aarch64-apple-ios`  
  - `wasm32-unknown-unknown` (web)  

```sh
vitpm target add aarch64-apple-ios
vitpm target add aarch64-linux-android
```

### API mobile unifiée
- Module `std::mobile` :  
  - `camera::capture()`  
  - `gps::location()`  
  - `sensor::accelerometer()`  

Exemple :  
```vitte
use std::mobile

fn main() {
    let pos = mobile::gps::location()
    print("Lat: {}, Lon: {}", pos.lat, pos.lon)
}
```

### Interface graphique cross-platform
- Binding `vitui` inspiré de Flutter/SwiftUI.  
- Définition déclarative d’interfaces :  

```vitte
use vitui::{App, Text, Button, Column}

fn main() {
    App::new("HelloApp", |ctx| {
        Column::new([
            Text::new("Bonjour depuis Vitte!"),
            Button::new("Clique-moi", || {
                ctx.alert("Tu as cliqué!")
            })
        ])
    }).run()
}
```

### Intégration native
- Possibilité d’appeler directement du code Swift/Java/Kotlin via FFI.  
- Packaging :  
```sh
vitpm build --target aarch64-apple-ios --bundle
vitpm build --target aarch64-linux-android --apk
```

### Gestion des permissions
- Déclarées dans `vitpm.toml`.  
```toml
[permissions]
camera = true
location = true
```

### Exemple complet : App mobile minimaliste
```vitte
use vitui::{App, Text}

fn main() {
    App::new("GPSApp", |_ctx| {
        Text::new("Position: " + std::mobile::gps::location().to_string())
    }).run()
}
```

### CI/CD mobile
- Build automatisé avec `vitpm ci android` et `vitpm ci ios`.  
- Intégration avec stores (`Play Store`, `App Store`).  

## Alternatives considérées
- **Ne pas supporter mobile** : rejeté, marché énorme manqué.  
- **Passer par frameworks externes uniquement** : rejeté, uniformité nécessaire.  
- **Ne cibler que Android** : rejeté, iOS indispensable.  

## Impact et compatibilité
- Impact fort : ouvre Vitte au mobile et aux apps grand public.  
- Compatible avec stores officiels et web.  
- Introduit `vitui` comme framework cross-platform.  

## Références
- [Flutter](https://flutter.dev/)  
- [React Native](https://reactnative.dev/)  
- [SwiftUI](https://developer.apple.com/xcode/swiftui/)  
- [Kotlin Multiplatform](https://kotlinlang.org/lp/mobile/)  
