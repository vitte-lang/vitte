# 🎓 Getting Started with Vitte SDK

Welcome! This guide will help you get started with the Vitte Programming Language SDK.

## ⚡ Quick Start (5 minutes)

### 1. Setup Environment
```bash
# Set SDK path
export VITTE_SDK=/path/to/vitte/sdk
export PATH="$VITTE_SDK/bin:$PATH"

# Verify installation
vittec --version
vitte-new --version
vitte-build --version
```

### 2. Create First Project
```bash
# Create CLI application
vitte-new hello --template=cli
cd hello
```

### 3. Build & Run
```bash
# Build the project
vitte-build

# Run the binary
./target/hello

# Or on Windows
target\hello
```

That's it! You've successfully compiled and run a Vitte program.

---

## 📚 Learning Path

### Beginner (1-2 hours)
1. **Read**: [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md)
   - Common commands
   - Basic patterns
   - Troubleshooting

2. **Try**: [TEMPLATES.md](sdk/docs/TEMPLATES.md)
   - Create all 4 template types
   - Understand template structure
   - Customize examples

3. **Explore**: Example programs in `templates/`
   - Study project layouts
   - Review code patterns

### Intermediate (3-5 hours)
1. **Study**: [SDK.md](sdk/docs/SDK.md)
   - Complete feature overview
   - Build system details
   - Configuration options

2. **Learn**: [BUILDING.md](sdk/docs/BUILDING.md)
   - Build process
   - Optimization levels
   - Cross-compilation

3. **Practice**: Build multi-file projects
   - Module organization
   - Public APIs
   - Testing patterns

### Advanced (6+ hours)
1. **Understand**: [FFI.md](sdk/docs/FFI.md)
   - C interoperability
   - Type mapping
   - Unsafe blocks

2. **Master**: [TARGETS.md](sdk/docs/TARGETS.md)
   - Cross-compilation
   - Platform-specific code
   - Target configuration

3. **Deep Dive**: [ABI.md](sdk/docs/ABI.md) & [SPECIFICATION.md](sdk/docs/SPECIFICATION.md)
   - Binary interface details
   - Performance tuning
   - System integration

---

## 🎯 Common Tasks

### Create New Project
```bash
# CLI application
vitte-new my-cli --template=cli

# Library
vitte-new my-lib --template=lib

# WebAssembly
vitte-new my-wasm --template=wasm

# Plugin
vitte-new my-plugin --template=plugin
```

### Build Project
```bash
# Debug build
cd my-project
vitte-build

# Release (optimized)
vitte-build --release

# Clean rebuild
vitte-build --clean && vitte-build
```

### Test Code
```bash
# Run tests
vitte-test

# Run specific test
vitte-test my_test_name

# With coverage
vitte-test --coverage
```

### Code Quality
```bash
# Format code
vitte-fmt

# Lint code
vittec --lint main.vit

# Generate documentation
vitte-doc --output=target/doc
```

### Cross-Compile
```bash
# Target macOS from Linux
vitte-build --target x86_64-apple-darwin --cross

# Target WebAssembly
vitte-build --target wasm32-unknown-unknown

# Target ARM Linux
vitte-build --target aarch64-unknown-linux-gnu --cross
```

---

## 📖 Documentation Guide

### By Role

**🎨 Designer**
- [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md) - Overview
- [SPECIFICATION.md](sdk/docs/SPECIFICATION.md) - Architecture

**👨‍💻 Developer**
- [SDK.md](sdk/docs/SDK.md) - Full guide
- [BUILDING.md](sdk/docs/BUILDING.md) - Build system
- [TEMPLATES.md](sdk/docs/TEMPLATES.md) - Project templates
- [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md) - Command ref

**🔗 FFI Integration**
- [FFI.md](sdk/docs/FFI.md) - C interop
- [ABI.md](sdk/docs/ABI.md) - Binary interface
- [sysroot/include/vitte/](sdk/sysroot/include/vitte/) - C headers

**🌍 Cross-Platform**
- [TARGETS.md](sdk/docs/TARGETS.md) - Platform support
- [BUILDING.md](sdk/docs/BUILDING.md) - Cross-compilation
- [config/platforms.toml](sdk/config/platforms.toml) - Platform defs

**📦 Distribution**
- [PACKAGING.md](sdk/docs/PACKAGING.md) - Distribution guide
- [BUILDING.md](sdk/docs/BUILDING.md) - Optimization
- [toolchains/README.md](sdk/toolchains/README.md) - Toolchain config

**🏗️ Contributor**
- [CONTRIBUTING.md](sdk/CONTRIBUTING.md) - Contribution guide
- [INDEX.md](sdk/INDEX.md) - File reference
- [SPECIFICATION.md](sdk/docs/SPECIFICATION.md) - Technical spec

---

## 🛠️ Project Structure

After creating a project with `vitte-new`, you'll have:

```
my-project/
├── muffin.muf              # Project manifest
├── vitte.toml              # Build configuration
├── src/
│   ├── main.vit           # Entry point (CLI)
│   └── lib.vit            # Library code
├── tests/
│   └── test_*.vit         # Tests
├── examples/
│   └── example.vit        # Examples
├── target/
│   ├── debug/             # Debug artifacts
│   └── release/           # Release artifacts
└── README.md              # Project documentation
```

---

## 🔍 Finding Information

### Quick Lookup
→ [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md)

### Build-Related Questions
→ [BUILDING.md](sdk/docs/BUILDING.md)

### Platform/Target Questions
→ [TARGETS.md](sdk/docs/TARGETS.md) or [config/platforms.toml](sdk/config/platforms.toml)

### C Integration Questions
→ [FFI.md](sdk/docs/FFI.md)

### Performance/Optimization
→ [ABI.md](sdk/docs/ABI.md) or [BUILDING.md](sdk/docs/BUILDING.md)

### Template Questions
→ [TEMPLATES.md](sdk/docs/TEMPLATES.md)

### Distribution/Packaging
→ [PACKAGING.md](sdk/docs/PACKAGING.md)

### File Reference
→ [INDEX.md](sdk/INDEX.md)

### Contributing
→ [CONTRIBUTING.md](sdk/CONTRIBUTING.md)

---

## ⚙️ Configuration

### Project Configuration (vitte.toml)
```toml
[package]
name = "my-app"
version = "0.1.0"

[build]
optimization = "release"
lto = true

[features]
default = ["std", "alloc"]
```

### Platform Support
See [config/platforms.toml](sdk/config/platforms.toml) for 14+ platform definitions.

### Feature Flags
See [config/features.toml](sdk/config/features.toml) for available features.

---

## 🎓 Code Examples

### Hello World
```vitte
fn main() {
    println!("Hello, Vitte!")
}
```

### Function with Error Handling
```vitte
fn divide(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err("Division by zero".to_string())
    } else {
        Ok(a / b)
    }
}
```

### Pattern Matching
```vitte
match value {
    Some(x) => println!("Found: {}", x),
    None => println!("Not found"),
}
```

### C FFI
```vitte
extern "C" fn strlen(s: *const i8) -> usize

fn main() {
    let result = strlen("hello\0".as_ptr());
    println!("Length: {}", result);
}
```

More examples in:
- `templates/cli/src_main.vit` - CLI patterns
- `templates/lib/src_lib.vit` - Library patterns
- `templates/wasm/src_lib.vit` - WebAssembly patterns
- `templates/plugin/src_lib.vit` - Plugin patterns

---

## 🚀 Next Steps

1. **Setup**: [Environment Setup](#-quick-start-5-minutes)
2. **Create**: First project with `vitte-new`
3. **Learn**: Read [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md)
4. **Practice**: Complete [TEMPLATES.md](sdk/docs/TEMPLATES.md) examples
5. **Explore**: Try cross-compilation to different targets
6. **Master**: Study [FFI.md](sdk/docs/FFI.md) and [ABI.md](sdk/docs/ABI.md)
7. **Share**: Package and distribute your project

---

## ❓ FAQ

**Q: Where do I start?**
A: Read this guide, then [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md)

**Q: How do I create a new project?**
A: `vitte-new name --template=TYPE` (cli, lib, wasm, plugin)

**Q: How do I build my project?**
A: `cd project && vitte-build`

**Q: How do I cross-compile?**
A: `vitte-build --target TRIPLE --cross`

**Q: Where are the C headers?**
A: [sdk/sysroot/include/vitte/](sdk/sysroot/include/vitte/)

**Q: How do I use C libraries?**
A: See [FFI.md](sdk/docs/FFI.md)

**Q: Which platforms are supported?**
A: See [TARGETS.md](sdk/docs/TARGETS.md) (14 targets)

**Q: Can I contribute?**
A: Yes! See [CONTRIBUTING.md](sdk/CONTRIBUTING.md)

---

## 📞 Support

### Documentation
- 📖 Full docs: [sdk/docs/](sdk/docs/)
- 🔍 Quick ref: [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md)
- 📋 File index: [INDEX.md](sdk/INDEX.md)

### Issues & Questions
- 🐛 Report bugs: GitHub Issues
- 💬 Ask questions: GitHub Discussions
- 📧 Email: support@vitte.dev

### Community
- 🌐 Website: https://vitte.dev
- 💻 GitHub: https://github.com/vitte-lang/vitte
- 📰 Blog: https://blog.vitte.dev

---

## 🎉 Welcome!

You're now ready to build amazing applications with Vitte!

**Start with**: [QUICK_REFERENCE.md](sdk/docs/QUICK_REFERENCE.md)

Happy coding! 🚀

---

**Version**: 0.2.0
**Last Updated**: 2024
**Maintained By**: Vitte Project Team
