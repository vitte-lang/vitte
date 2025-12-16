# Module System Example

Demonstrates organizing code with modules.

## Usage

```bash
vittec muffin.muf
```

## Structure

```
modules/
├── muffin.muf       # Package manifest
├── src/
│   ├── main.vit     # Entry point
│   ├── math/
│   │   ├── mod.muf  # Module declaration
│   │   └── ops.vit  # Module contents
│   └── utils/
│       ├── mod.muf
│       └── helpers.vit
```

## Description

Shows:
- Module organization
- Public/private visibility
- Module imports
- Re-exports
- Nested modules
