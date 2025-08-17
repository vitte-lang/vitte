# Vitte Desktop — complet (GTK + Qt)

Application bureau légère pour lancer/essayer des programmes **Vitte**,
avec deux frontends : **GTK** et **Qt Widgets**. Le binaire appelle `vitte` (CLI)
pour exécuter un script, capture la sortie et l’affiche dans une console intégrée.

## Fonctions
- Ouvrir un fichier `.vitte`
- Bouton **Run** → `vitte run <fichier>` (stdout/stderr visibles)
- Console scrollable, compteur de durée (ms)
- Thème clair/sombre (Qt suit l’OS, GTK via thème système)
- Stubs fournis si toolkits absents (l’app se compile quand même en mode console).

## Build rapide

### GTK (Linux / *BSD)
```bash
# Dépendances: gtk4 (ou gtk+3), pkg-config, gcc/clang
make gtk
./build/desktop-gtk
```

### Qt (Linux/macOS/Windows)
```bash
# Dépendances: Qt 6 (ou Qt 5) + CMake + Ninja
make qt
./build-qt/desktop-qt    # (ou .exe sur Windows)
```

## Packaging (Linux)
- Fichier `.desktop` prêt: `packaging/linux/vitte-studio.desktop`
- Icônes en `assets/icons/`
- Exemple AppImage script (à compléter si voulu).

## Dossiers
- `gtk/` sources C (GTK)
- `qt/` sources C++ + CMakeLists (Qt Widgets)
- `scripts/` build helpers
- `assets/` icônes/ressources
- `packaging/` Linux `.desktop`, mac `Info.plist`, Windows NSIS (stubs)
