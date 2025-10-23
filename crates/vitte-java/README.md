

# vitte-java

Backend **Java/JVM** et interopérabilité pour le langage Vitte.  
`vitte-java` permet la génération de bytecode Java (`.class`) à partir de l’IR Vitte, ainsi que la liaison bidirectionnelle entre les modules Vitte et les bibliothèques JVM.

---

## Objectifs

- Fournir une **cible JVM complète** pour Vitte.  
- Générer des classes Java directement depuis l’IR du compilateur.  
- Supporter les appels de fonctions et structures entre Vitte et Java.  
- Offrir une compatibilité totale avec les outils JVM (JAR, Gradle, Maven).  
- Intégration dans le pipeline multi-backend de `vitte-compiler`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération du bytecode Java et `.class` files |
| `builder`     | Construction des signatures et symboles JVM |
| `interop`     | Conversion de types et appels FFI Java ↔ Vitte |
| `runtime`     | Liaison avec la JVM (`jni`, `classloader`) |
| `analyze`     | Validation des dépendances et imports externes |
| `tests`       | Tests de génération, exécution et compatibilité JVM |

---

## Exemple d’utilisation

```rust
use vitte_java::JavaBackend;
use vitte_codegen::Context;

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new_target("java");
    let backend = JavaBackend::new();

    let func = ctx.create_function("hello_world");
    backend.emit_function(&ctx, &func, "HelloWorld.class")?;

    println!("Bytecode Java généré avec succès.");
    Ok(())
}
```

---

## Exemple de code généré

```java
public class HelloWorld {
    public static void main(String[] args) {
        System.out.println("Bonjour depuis Vitte!");
    }
}
```

---

## Intégration

- `vitte-codegen` : génération de bytecode JVM.  
- `vitte-compiler` : pipeline multi-cible (x86, AArch64, JVM, WASM).  
- `vitte-runtime` : exécution et interaction avec la JVM.  
- `vitte-analyzer` : validation des types et symboles interlangages.  
- `vitte-docgen` : génération de documentation pour modules Java.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
byteorder = "1.5"
``` 

> Optionnel : `jni` pour liaison native JVM, `zip` pour génération d’archives JAR.

---

## Tests

```bash
cargo test -p vitte-java
```

- Tests de génération de bytecode et exécution JVM.  
- Tests de compatibilité entre IR et code Java généré.  
- Tests d’interopérabilité avec JNI et bibliothèques externes.  
- Benchmarks de performances JIT.

---

## Roadmap

- [ ] Génération complète de `.jar` signés.  
- [ ] Interopération dynamique via JNI et JNA.  
- [ ] Compilation croisée Java → Vitte (analyse bytecode).  
- [ ] Profilage JVM et instrumentation bytecode.  
- [ ] Support Kotlin/Scala dans le pipeline JVM.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau