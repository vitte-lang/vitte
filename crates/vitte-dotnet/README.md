

# vitte-dotnet

Intégration .NET pour l’écosystème **Vitte**.

Ce crate fournit les bindings, interopérabilités et générateurs nécessaires pour exposer le langage Vitte et ses modules au runtime .NET (C#, F#, VB). Il permet d’appeler du code Vitte depuis un projet .NET, ou d’utiliser les bibliothèques .NET au sein de Vitte via une couche d’interop FFI gérée.

---

## Objectifs

- Permettre la **communication bidirectionnelle** entre Vitte et .NET  
- Fournir un **générateur automatique de bindings C#** depuis les modules Vitte  
- Gérer les **types, exceptions et objets** entre les deux environnements  
- Offrir un **runtime embarqué** de Vitte dans les applications .NET  
- Intégration complète avec `vitte-cli`, `vitte-vm`, et `vitte-runtime`

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `bridge`       | Mécanismes d’interop (C FFI / C# PInvoke / C++/CLI) |
| `gen`          | Génération de wrappers C# et signatures d’API |
| `types`        | Conversion des types Vitte ↔ .NET |
| `runtime`      | Lancement et gestion d’une VM Vitte embarquée |
| `assembly`     | Introspection des assemblies .NET externes |
| `cli`          | Commandes de génération et d’exécution |
| `tests`        | Tests d’intégration cross-language |

---

## Exemple d’utilisation

### 1. Génération des bindings .NET

```bash
vitte dotnet gen-bindings --out bindings/
```

Produit des fichiers `.cs` pour tous les modules Vitte exportés.

### 2. Utilisation en C#

```csharp
using Vitte.Runtime;

class Program {
    static void Main() {
        var vm = new VitteVM();
        vm.Eval("print(42)");
        int result = vm.Call<int>("math.add", 2, 3);
        Console.WriteLine(result); // 5
    }
}
```

### 3. Appel de .NET depuis Vitte

```vitte
use dotnet::System::Math;

fn main() {
    let x = Math::Sqrt(9.0);
    print(x); // 3.0
}
```

---

## Commandes CLI

```
vitte dotnet gen-bindings [--out <dossier>]
vitte dotnet run <assembly.dll>
vitte dotnet link <module.vit>
vitte dotnet inspect <assembly.dll>
```

Exemples :
```
vitte dotnet gen-bindings --out src/bindings
vitte dotnet inspect System.dll
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
cbindgen = "0.26"
csbindgen = "1.5"
interoptopus = "0.16"
```

---

## Intégration

- `vitte-vm` : exécution native du bytecode Vitte depuis .NET  
- `vitte-cli` : génération des bindings et inspection  
- `vitte-runtime` : runtime intégré pour VM embarquée  
- `.NET 6+` : support via `DllImport` et `NativeLibrary`  

---

## Tests

```bash
cargo test -p vitte-dotnet
dotnet test bindings/InteropTests.csproj
```

- Tests unitaires Rust et C# combinés  
- Validation des appels interlangage  
- Tests de performance VM embarquée  

---

## Roadmap

- [ ] Génération automatique de NuGet packages  
- [ ] Support complet F# et VB.NET  
- [ ] Mapping async/await ↔ coroutines Vitte  
- [ ] Débogage interlangage (VSCode / Rider)  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau