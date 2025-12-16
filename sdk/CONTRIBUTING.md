# Contributing to Vitte SDK

Thank you for your interest in improving the Vitte SDK!

## Guidelines

### Adding New Targets

1. **Create target configuration JSON** in `sysroot/share/vitte/targets/TRIPLE.json`
   - Follow existing examples (e.g., x86_64-linux-gnu.json)
   - Include all required fields: name, architecture, os, pointer_width, etc.
   - Set appropriate tier level (1=production, 2=stable, 3=experimental)

2. **Update documentation**:
   - Add to `docs/TARGETS.md` with details
   - Update `config/platforms.toml` with platform definition
   - List in `README.md` Supported Platforms section

3. **Test the target**:
   ```bash
   vittec test.vit --target TRIPLE
   ```

4. **Submit PR** with:
   - Target JSON file
   - Documentation updates
   - Test results showing compilation succeeds

### Adding New Toolchain

1. **Create toolchain directory** and config:
   ```
   toolchains/NAME/
   └── config.toml
   ```

2. **Define compiler flags**:
   - base, debug, release, lto flags
   - Linker configuration
   - Runtime library settings
   - Target-specific overrides

3. **Update global config**:
   - Edit `toolchains/config.toml`
   - Add to appropriate fallback chain
   - Set environment variable names

4. **Document**:
   - Update `toolchains/README.md`
   - Add usage examples
   - List supported targets

5. **Test**:
   ```bash
   vitte-build --toolchain NAME
   ```

### Adding New Template

1. **Create template directory**:
   ```
   templates/TYPE/
   ├── README.md
   ├── muffin.muf
   └── src/[lib|main].vit
   ```

2. **Include standard files**:
   - Clear, concise README
   - Working muffin.muf manifest
   - Runnable example code
   - Comments explaining patterns

3. **Document in `docs/TEMPLATES.md`**:
   - Purpose and use case
   - Quick start instructions
   - Key patterns shown
   - Customization examples

4. **Test**:
   ```bash
   vitte-new test-proj --template=TYPE
   cd test-proj
   vitte-build
   ```

### Updating Documentation

1. **Identify document**:
   - SDK.md - Overview
   - BUILDING.md - Build system
   - FFI.md - C interop
   - TARGETS.md - Platforms
   - ABI.md - Binary interface
   - PACKAGING.md - Distribution
   - QUICK_REFERENCE.md - Command reference
   - SPECIFICATION.md - Formal spec

2. **Make clear edits**:
   - Use clear examples
   - Update all cross-references
   - Keep consistent formatting

3. **Verify links**:
   - Check internal links work
   - Update INDEX.md if structure changes
   - Test code examples compile

### Improving Headers

1. **Update ABI-stable headers** in `sysroot/include/vitte/`:
   - core.h - Core types
   - runtime.h - Runtime support
   - platform.h - OS abstraction
   - alloc.h - Memory management

2. **Maintain ABI stability**:
   - Cannot remove functions
   - Cannot change signatures
   - Cannot change struct layouts
   - Adding fields requires careful versioning

3. **Update documentation**:
   - Add comments explaining new functions
   - Document behavior and contracts
   - List parameter semantics
   - Describe return values

4. **Test compatibility**:
   - Existing code must compile
   - Binary compatibility maintained
   - Version should remain stable

## Code Style

### TOML Configuration
```toml
# Use clear section names
[section]
# Use underscores for keys
option_name = value

# Use arrays for lists
items = ["item1", "item2"]

# Comment complex settings
# This controls optimization level (debug|release|lto)
optimization = "release"
```

### JSON Target Configs
```json
{
  "name": "triple-format",
  "architecture": "cpu_arch",
  "os": "operating_system",
  "pointer_width": 64,
  "tier": 1,
  "features": "+feature1,+feature2"
}
```

### Markdown Documentation
- Clear headings (1-4 levels)
- Code blocks with language: ` ```bash `
- Links to related docs: `[link text](file.md)`
- Tables for reference material
- Examples for complex topics

### Vitte Code (Templates)
```vitte
// Use idiomatic Vitte patterns
pub fn public_function(arg: Type) -> Result<Output, Error> {
    // Implementation
    Ok(output)
}

#[test]
fn test_function() {
    assert_eq!(public_function(input), expected);
}
```

## Testing Changes

### For target additions:
```bash
# Verify target exists
vittec --list-targets | grep new-target

# Build test program
vittec test.vit --target new-target -o test.out

# Cross-compile if needed
vitte-build --target new-target --cross
```

### For templates:
```bash
# Create project from template
vitte-new test-proj --template=new

# Verify it builds
cd test-proj
vitte-build

# Run tests if available
vitte-test
```

### For documentation:
- Check spelling and grammar
- Verify all code examples compile
- Test all links
- Review formatting

## Submission Process

1. **Fork and branch**:
   ```bash
   git checkout -b feature/add-new-target
   ```

2. **Make changes**:
   - One logical change per PR
   - Update related documentation
   - Add tests/examples where applicable

3. **Commit with clear messages**:
   ```bash
   git commit -m "Add armv6-unknown-linux-gnueabihf target"
   ```

4. **Open pull request**:
   - Title: concise description
   - Description: what, why, how
   - Link related issues
   - Include test results

5. **Address review feedback**:
   - Make requested changes
   - Respond to questions
   - Update if new issues found

## Review Criteria

PRs are reviewed for:

✅ **Correctness**: Code/config is correct and complete
✅ **Documentation**: Changes are clearly documented
✅ **Testing**: Changes include appropriate tests/examples
✅ **Compatibility**: ABI stability maintained where needed
✅ **Style**: Consistent with project conventions
✅ **Scope**: Single, focused change

## Questions?

- Check [SDK.md](docs/SDK.md) for overview
- See [SPECIFICATION.md](docs/SPECIFICATION.md) for details
- Browse existing configs for examples
- Ask in GitHub discussions

## Code of Conduct

See [../../CODE_OF_CONDUCT.md](../../CODE_OF_CONDUCT.md)

---

Thank you for contributing to making Vitte better!
