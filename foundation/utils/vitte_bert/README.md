# vitte_bert

Basic Data Structures and Encoding for Vitte compiler utilities.

## Features

- Stack and Queue implementations
- BitVector for efficient bit operations
- IndexMap for key-value storage
- Symbol management
- Serializable trait for encoding/decoding

## Usage

```vit
use vitte_bert::{Stack, Queue, BitVector, Symbol};

fn main() {
    let mut stack = Stack::new();
    stack.push(42);
    
    let mut bv = BitVector::new(32);
    bv.set(5, true);
    
    let sym = Symbol::new(1, "identifier".to_string());
}
```
