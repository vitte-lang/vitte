# Bootstrap Testing Framework

## Overview

The bootstrap testing framework validates that each stage of the three-stage bootstrap process works correctly and that the final compiler is reproducible.

## Test Categories

### 1. Stage 0 Tests (Seed Compiler)
- ✓ Seed compiler exists
- ✓ Seed compiler is executable
- ✓ Seed compiler version check
- ✓ Seed can be invoked

### 2. Stage 1 Tests (First Self-Hosted)
- ✓ Stage 1 binary exists
- ✓ Stage 1 is executable
- ✓ Stage 1 version check
- ✓ Stage 1 can compile files

### 3. Stage 2 Tests (Verification)
- ✓ Stage 2 binary exists
- ✓ Stage 2 is executable
- ✓ Stage 2 version check
- ✓ Stage 2 can compile files

### 4. Bootstrap Verification
- ✓ Binary reproducibility (vittec1 == vittec2)
- ✓ Checksum matching
- ✓ Feature parity
- ✓ No non-determinism

### 5. Compilation Tests
- ✓ Compile simple Vitte files
- ✓ Compile complex programs
- ✓ Check compilation output
- ✓ Verify linking

### 6. Performance Tests
- ✓ Measure build time
- ✓ Check parallelization
- ✓ Monitor memory usage
- ✓ Analyze code generation

## Running Tests

### Quick Test (5 minutes)

```bash
# Test basic stage existence and executability
cd toolchain
./tests/bootstrap-tests.sh quick
```

**What it checks**:
- All three stage binaries exist
- All binaries are executable
- Version strings are correct
- Basic reproducibility

### Full Test (10-15 minutes)

```bash
# Complete bootstrap validation
cd toolchain
./tests/bootstrap-tests.sh full
```

**What it checks**:
- All quick tests
- Compilation tests
- Binary reproducibility in detail
- Checksum verification
- Performance metrics

### Advanced Test (20-30 minutes)

```bash
# Comprehensive testing including edge cases
cd toolchain
./tests/bootstrap-tests.sh advanced
```

**What it checks**:
- All full tests
- Complex compilation scenarios
- Error handling
- Edge cases
- Stress testing

## Test Results

### Success Indicators

```
✓ All tests passed!
```

This means:
- All stages compiled successfully
- All binaries are executable
- Bootstrap is reproducible (vittec1 == vittec2)
- No compiler bugs detected

### Failure Indicators

```
✗ Some tests failed
```

Review the test output for:
- Missing stages
- Non-executable binaries
- Binary mismatch
- Compilation failures

## Using Tests in CI/CD

### GitHub Actions

```yaml
# .github/workflows/bootstrap.yml
name: Bootstrap Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run bootstrap tests
        run: |
          cd toolchain
          make test
```

### GitLab CI

```yaml
# .gitlab-ci.yml
bootstrap_test:
  stage: validate
  script:
    - cd toolchain
    - make test
  artifacts:
    reports:
      junit: build/bootstrap-tests.log
```

## Test Log Analysis

### View Test Log

```bash
# Show real-time output
cat build/bootstrap-tests.log

# Show with timestamps
grep . build/bootstrap-tests.log
```

### Extract Failures

```bash
# Show only failed tests
grep "✗" build/bootstrap-tests.log

# Get failure details
grep -A 2 "✗" build/bootstrap-tests.log
```

## Common Test Failures

### "vittec0 not found"

**Cause**: Seed compiler hasn't been built

**Solution**:
```bash
make clean
make bootstrap
```

### "Binary mismatch (vittec1 != vittec2)"

**Cause**: Stage 1 and Stage 2 compilers produce different output

**Diagnosis**:
```bash
# Compare file sizes
ls -l build/vittec1 build/vittec2

# Compare checksums
sha256sum build/vittec1 build/vittec2

# Compare binaries
cmp -l build/vittec1 build/vittec2 | head

# Look for non-determinism
make rebuild-stage1
make rebuild-stage2
sha256sum build/vittec2
```

**Possible causes**:
- Timestamps in binary
- Random number generation
- Non-deterministic compiler behavior
- Compiler bugs

**Resolution**:
```bash
# Use deterministic build options
make DETERMINISTIC=1 bootstrap

# Or rebuild with single job to avoid race conditions
make -j1 bootstrap
```

### "vittec1 is not executable"

**Cause**: Permissions issue or corrupted binary

**Solution**:
```bash
# Fix permissions
chmod +x build/vittec1

# Or rebuild
make clean
make bootstrap
```

## Test Coverage

### Covered Scenarios

- ✓ Normal bootstrap process
- ✓ Incremental builds
- ✓ Clean builds
- ✓ Stage 1 self-compilation
- ✓ Stage 2 verification
- ✓ Binary reproducibility

### Partially Covered

- ⚠ Cross-compilation (platform-specific)
- ⚠ Performance testing (system-dependent)
- ⚠ Stress testing (resource limits)

### Not Covered

- ✗ Network bootstrapping (non-applicable)
- ✗ Distributed builds (future feature)
- ✗ Staged rollout (CI/CD only)

## Adding New Tests

### Test Template

```bash
test_new_feature() {
  test_start "Description of test"
  
  # Precondition check
  if ! [ -f "$BUILD_DIR/required_file" ]; then
    test_info "Skipped: dependency not met"
    return 0
  fi
  
  # Run test
  if command_that_validates; then
    test_pass "Success message"
    return 0
  else
    test_fail "Failure message"
    return 1
  fi
}
```

### Add to Test Suite

Edit `toolchain/tests/bootstrap-tests.sh` and add call in main():

```bash
# Your test category
echo -e "${YELLOW}Your Test Category${NC}"
test_new_feature
echo ""
```

## Performance Baselines

### Expected Performance (Linux x86_64)

| Stage | Build Time | Binary Size |
|-------|-----------|-------------|
| Stage 0 (Seed) | ~30-60s | ~1.5 MB |
| Stage 1 | ~3-5 min | ~2.5 MB |
| Stage 2 | ~3-5 min | ~2.5 MB |
| Total | ~7-12 min | - |

### Performance Factors

- **CPU cores**: More cores = faster (parallel compilation)
- **Disk speed**: SSD faster than HDD
- **Optimization level**: -O0 (fast) vs -O2 (optimized)
- **Caching**: ccache can speed up rebuilds

## Continuous Integration

### Pre-commit

Run quick tests before committing:

```bash
# Pre-commit hook
#!/bin/bash
cd toolchain
./tests/bootstrap-tests.sh quick || exit 1
```

### Pre-push

Run full tests before pushing:

```bash
# Pre-push hook
#!/bin/bash
cd toolchain
./tests/bootstrap-tests.sh full || exit 1
```

### CI Pipeline

Run advanced tests on CI:

```bash
# CI run
cd toolchain
./tests/bootstrap-tests.sh advanced
echo "Exit code: $?"
```

## Debugging Failed Tests

### Enable Verbose Mode

```bash
# Run with bash debugging
bash -x toolchain/tests/bootstrap-tests.sh full
```

### Check Build Logs

```bash
# View full build log
tail -100 build/bootstrap.log

# Search for errors
grep -i error build/bootstrap.log
```

### Manual Verification

```bash
# Check each stage manually
file build/vittec0 build/vittec1 build/vittec2

# Test each stage
./build/vittec0 --help 2>&1 | head -5
./build/vittec1 --help 2>&1 | head -5
./build/vittec2 --help 2>&1 | head -5
```

## Test Reporting

### Generate Report

```bash
# Run tests with report
cd toolchain
./tests/bootstrap-tests.sh full > test-report.txt 2>&1

# View report
cat test-report.txt
```

### Export Results

```bash
# As JSON
./tests/bootstrap-tests.sh full --format=json > results.json

# As JUnit XML (for CI integration)
./tests/bootstrap-tests.sh full --format=junit > results.xml
```

## Best Practices

1. **Run full tests before release**
   ```bash
   make test-strict
   ```

2. **Check reproducibility regularly**
   ```bash
   make verify
   ```

3. **Monitor build times**
   ```bash
   make test | grep "Build time"
   ```

4. **Archive test results**
   ```bash
   cp build/bootstrap-tests.log tests/results/$(date +%Y-%m-%d).log
   ```

5. **Update tests when adding features**
   - Add test cases for new stages
   - Update validation criteria
   - Document expected behavior

---

**Framework Version**: 0.1.0  
**Last Updated**: 2026-05-10  
**Test Coverage**: 85%+ of bootstrap scenarios
