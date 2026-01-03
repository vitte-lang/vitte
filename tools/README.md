# Vitte Tools Suite

Collection d'outils et utilitaires pour le développement, le testing et le déploiement du langage Vitte.

## ��️ Outils Disponibles

### 1. **vitte-fmt** - Code Formatter
Formatteur de code automatique pour le langage Vitte.
```bash
vitte-fmt <file.vitte>          # Format un fichier
vitte-fmt --check <file.vitte>  # Vérifier sans modifier
vitte-fmt --recursive <dir>     # Format tous les fichiers
```

### 2. **vitte-lint** - Static Analyzer
Analyseur statique pour détecter les erreurs et patterns suspects.
```bash
vitte-lint <file.vitte>         # Analyser un fichier
vitte-lint --strict <file.vitte> # Mode strict
vitte-lint --fix <file.vitte>    # Auto-correction
```

### 3. **vitte-doc** - Documentation Generator
Générateur de documentation à partir des commentaires du code.
```bash
vitte-doc <file.vitte>          # Générer la doc
vitte-doc --html <file.vitte>   # Format HTML
vitte-doc --recursive <dir>     # Doc complète du projet
```

### 4. **vitte-debug** - Interactive Debugger
Débuggeur interactif pour le bytecode Vitte.
```bash
vitte-debug <bytecode.vit>      # Déboguer
vitte-debug --step              # Mode pas-à-pas
vitte-debug --break line:5      # Point d'arrêt
```

### 5. **vitte-profile** - Performance Profiler
Profiler pour analyser la performance et l'utilisation des ressources.
```bash
vitte-profile <bytecode.vit>    # Profiler
vitte-profile --memory          # Analyse mémoire
vitte-profile --flame           # Flamegraph
```

### 6. **vitte-test** - Test Runner
Exécuteur de tests avec rapports détaillés.
```bash
vitte-test <dir>                # Exécuter les tests
vitte-test --watch              # Mode watch
vitte-test --coverage           # Couverture de code
```

### 7. **vitte-cli** - Interactive REPL
Interface de ligne de commande interactive.
```bash
vitte-cli                       # Lancer le REPL
vitte-cli <file.vitte>          # Exécuter un fichier
vitte-cli --playground          # Mode bac à sable
```

## 📁 Structure

```
tools/
├── README.md                    (ce fichier)
├── vitte-fmt/                   (Code formatter)
│   ├── fmt.c
│   ├── fmt.h
│   └── main.c
├── vitte-lint/                  (Static analyzer)
│   ├── lint.c
│   ├── lint.h
│   └── main.c
├── vitte-doc/                   (Doc generator)
│   ├── docgen.c
│   ├── docgen.h
│   └── main.c
├── vitte-debug/                 (Interactive debugger)
│   ├── debugger.c
│   ├── debugger.h
│   └── main.c
├── vitte-profile/               (Performance profiler)
│   ├── profiler.c
│   ├── profiler.h
│   └── main.c
├── vitte-test/                  (Test runner)
│   ├── runner.c
│   ├── runner.h
│   └── main.c
├── vitte-cli/                   (Interactive REPL)
│   ├── repl.c
│   ├── repl.h
│   └── main.c
└── CMakeLists.txt               (Build configuration)
```

## 🚀 Build Tools

```bash
# Build all tools
cd tools && cmake -B build && cmake --build build

# Build specific tool
cmake --build build --target vitte-fmt

# Install tools
cmake --install build
```

## 🔧 Requirements

- C99 compiler (gcc, clang)
- CMake 3.16+
- Vitte runtime library
- Standard libraries (readline, curses for TUI tools)

## 📖 Documentation

Each tool has its own README:
- [vitte-fmt/README.md](vitte-fmt/README.md)
- [vitte-lint/README.md](vitte-lint/README.md)
- [vitte-doc/README.md](vitte-doc/README.md)
- [vitte-debug/README.md](vitte-debug/README.md)
- [vitte-profile/README.md](vitte-profile/README.md)
- [vitte-test/README.md](vitte-test/README.md)
- [vitte-cli/README.md](vitte-cli/README.md)

## 🎯 Common Tasks

### Format all project files
```bash
vitte-fmt --recursive ../
```

### Check code quality
```bash
vitte-lint --strict ../
```

### Generate project documentation
```bash
vitte-doc --recursive ../ --html
```

### Run all tests with coverage
```bash
vitte-test ../ --coverage
```

### Profile a program
```bash
vitte-profile ./program.vit --flame > profile.html
```

## 🤝 Contributing

Tools improvements welcome! See CONTRIBUTING.md in root directory.

## 📄 License

Same as Vitte project - see LICENSE file.
