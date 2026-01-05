# vitte_raym

Runtime Attributes and Memory management for Vitte.

## Features

- Memory pool management with allocation/deallocation
- Multiple memory strategies (Stack, Heap, Arena)
- Lifetime tracking for values
- Runtime attributes system
- Runtime managed trait for cleanup

## Usage

```vit
use vitte_raym::{MemoryAllocator, MemoryStrategy, Lifetime};

fn main() {
    let mut allocator = MemoryAllocator::new(MemoryStrategy::Heap, 2048);
    allocator.allocate(512);
    
    let lifetime = Lifetime::new(0, 1000);
    assert!(lifetime.is_active(500));
}
```
