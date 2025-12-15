# Vitte SDK (max)

Le SDK fournit le **sysroot** utilisé par:
- le compilateur `vittec` (C)
- les binders/FFI (`rust/crates/vitte-sys`)
- les outils (fmt/lsp/doc) pour connaître ABI/targets/templates

## Layout (contract)
- `sysroot/include/vitte/**.h` : headers ABI stables
- `sysroot/lib/vitte/` : libs runtime + stubs (static/shared)
- `sysroot/lib/pkgconfig/*.pc` : (optionnel) pkg-config
- `sysroot/share/vitte/targets/*.json` : cibles supportées
- `sysroot/share/vitte/specs/` : copies (ou pointers) vers spec officielle
- `sysroot/share/vitte/templates/` : templates (projets, modules, plugins)

Env:
- `VITTE_SDK=/path/to/vitte/sdk`
