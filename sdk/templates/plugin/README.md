# Vitte Plugin Template

This template provides a scaffold for building Vitte plugins and extensions.

## Quick Start

```bash
cd my-plugin
vitte-build --release
```

Output: `target/release/libmy_plugin.so` (or `.dylib`, `.dll`)

## Structure

- `src/lib.vit` - Plugin entry point and exports
- `muffin.muf` - Plugin manifest

## Plugin Lifecycle

### Loading
```vitte
pub extern "C" fn plugin_init(ctx: *const PluginContext) -> PluginStatus {
    // Initialize plugin
    PluginStatus::OK
}
```

### Execution
```vitte
pub extern "C" fn plugin_execute(name: *const i8, args: *const *const i8, argc: i32) -> i32 {
    // Execute plugin command
    0
}
```

### Unloading
```vitte
pub extern "C" fn plugin_deinit() {
    // Clean up resources
}
```

## Building

### Debug
```bash
vitte-build
```

### Release (Optimized)
```bash
vitte-build --release
```

### Specific Target
```bash
vitte-build --target x86_64-apple-darwin
```

## Testing

```bash
vitte-test
```

## Distribution

Plugins are packaged as:
- **Linux**: `.so` (shared object)
- **macOS**: `.dylib` (dynamic library)
- **Windows**: `.dll` (dynamic link library)

Deploy to: `~/.vitte/plugins/`

## Examples

See [Vitte examples directory](../../examples/) for more plugin patterns.

## Documentation

- [SDK Documentation](../../docs/SDK.md)
- [FFI Guide](../../docs/FFI.md)
- [Vitte Language Reference](../../docs/language-spec/reference.md)
