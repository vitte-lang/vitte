# vitte_jack

Configuration, Abstraction and Knowledge systems for Vitte compiler.

## Features

- Configuration value types (String, Integer, Boolean, Array)
- Configuration management with key-value storage
- Compiler options builder pattern
- Knowledge base with fact storage and querying
- Metadata store for cross-module information
- Abstraction layer trait for semantic abstraction

## Usage

```vit
use vitte_jack::{Config, ConfigValue, CompilerOptions, KnowledgeBase, Fact};

fn main() {
    let mut config = Config::new();
    config.set("opt_level".to_string(), ConfigValue::Integer(2));
    
    let opts = CompilerOptions::new()
        .with_optimization()
        .with_target("x86_64".to_string());
    
    let mut kb = KnowledgeBase::new();
    kb.add_fact(Fact::new(
        "type".to_string(),
        "int".to_string(),
        "32-bit".to_string()
    ));
}
```
