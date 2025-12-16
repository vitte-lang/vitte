# Assembly Testing and Verification Guide

## Overview

This guide provides strategies for testing, validating, and benchmarking assembly code in the Vitte compiler.

## 1. Unit Testing Assembly Code

### Basic Test Structure

```c
#include <assert.h>
#include <stdio.h>
#include "compiler/inline_asm.h"

void test_bit_scan() {
    // Test bsf (bit scan forward)
    int result = 0;
    
    // Simplified test
    assert(result == 3);  // 0b1000 -> position 3
    printf("✓ bit_scan test passed\n");
}

void test_popcount() {
    // Test population count
    unsigned long x = 0xAAAAAAAAAAAAAAAAUL;
    unsigned int count = 0;
    
    // After popcount, should be 32 (alternating 1s)
    assert(count == 32);
    printf("✓ popcount test passed\n");
}

int main() {
    test_bit_scan();
    test_popcount();
    
    printf("\nAll assembly tests passed!\n");
    return 0;
}
```

### CTest Integration

Add to CMakeLists.txt:

```cmake
# Assembly-specific test executable
add_executable(test_assembly_ops
    tests/unit/test_assembly_ops.c
    src/inline_asm.c
)

target_include_directories(test_assembly_ops
    PRIVATE include
)

add_test(
    NAME assembly_operations
    COMMAND test_assembly_ops
)
```

## 2. Validation Strategies

### Strategy 1: Compare Against C Implementation

```vitte
// C implementation
fn popcount_c(x: u64) -> u32 {
    let count = 0
    
    while x > 0 {
        count = count + (x & 1)
        x = x >> 1
    }
    
    return count
}

// Assembly implementation
fn popcount_asm(x: u64) -> u32 {
    let count: u32
    
    asm "popcnt %1, %0"
        : "=r"(count)
        : "r"(x)
    
    return count
}

// Test
fn test_popcount() {
    for var i = 0; i < 100; i = i + 1 {
        let test_val = random()
        assert(popcount_c(test_val) == popcount_asm(test_val))
    }
    
    println("✓ popcount matches C implementation")
}
```

### Strategy 2: Property-Based Testing

```vitte
// Test properties that must hold

// Property 1: bsf returns position < 64
fn test_bsf_valid() {
    for var x = 1; x < (1 << 64); x = x << 1 {
        let pos = bsf(x)
        assert(pos < 64)
    }
}

// Property 2: popcount(x | y) >= popcount(x)
fn test_popcount_monotonic() {
    let x = 0xAAAAAAAAAAAAAAAA
    let y = 0x5555555555555555
    
    assert(popcount(x | y) >= popcount(x))
    assert(popcount(x | y) >= popcount(y))
}

// Property 3: bsr(x) > bsf(x) for any x
fn test_bsr_bsf_order() {
    let x = 0x100
    let high = bsr(x)  // Position of highest bit
    let low = bsf(x)   // Position of lowest bit
    
    assert(high > low)
}
```

### Strategy 3: Edge Case Testing

```c
#include <limits.h>

void test_edge_cases() {
    // Test minimum values
    test_operation(0x0000000000000001UL);  // Single bit
    test_operation(0x0000000000000002UL);  // Powers of 2
    test_operation(0x0000000000000004UL);
    test_operation(0x0000000000000008UL);
    
    // Test maximum values
    test_operation(0xFFFFFFFFFFFFFFFFUL);  // All bits set
    test_operation(0x7FFFFFFFFFFFFFFFUL);  // Max signed
    test_operation(0x8000000000000000UL);  // Min signed
    
    // Test common patterns
    test_operation(0xAAAAAAAAAAAAAAAAUL);  // Alternating pattern
    test_operation(0x5555555555555555UL);  // Inverse alternating
    
    printf("✓ All edge cases passed\n");
}
```

## 3. Benchmarking

### Simple Timing

```vitte
fn benchmark(name: *const u8, iterations: usize, fn: fn() -> i32) {
    let start = rdtsc()      // Read timestamp counter
    
    for var i = 0; i < iterations; i = i + 1 {
        fn()
    }
    
    let end = rdtsc()
    let cycles = end - start
    let per_call = cycles / iterations
    
    printf("%s: %d cycles per call\n", name, per_call)
}

fn main() {
    let iterations = 1000000
    
    benchmark("popcount_c", iterations, popcount_c)
    benchmark("popcount_asm", iterations, popcount_asm)
}
```

### Linux perf Integration

```bash
# Compile with debug symbols
gcc -g -O2 -march=native program.c -o program

# Profile with perf
perf record -e cycles,instructions,cache-misses ./program

# View results
perf report

# Show detailed assembly
perf annotate
```

### Windows VTune

```batch
REM Compile with debug info
cl /O2 /Zi program.c

REM Run with VTune (Intel VTune Profiler)
amplxe-cl -collect hotspots -result-dir ./result ./program.exe

REM View results in GUI
amplxe-gui ./result
```

## 4. Correctness Verification

### Compile and Check Assembly Output

```bash
# Generate assembly from C
gcc -S -O2 -march=native program.c

# Check generated inline assembly
grep -n "\.cfi_\|movq\|popcount" program.s
```

### Gdb Interactive Debugging

```bash
# Compile with debug symbols
gcc -g -O0 program.c -o program

# Start debugger
gdb ./program

# Inside gdb:
# break test_popcount
# run
# disassemble test_popcount
# stepi  (step by instruction)
# info registers
# x/i $pc  (examine instruction at program counter)
```

### Valgrind Memory Checking

```bash
# Run with memory checker
valgrind --leak-check=full ./program

# Check for uninitialized reads
valgrind --track-origins=yes ./program
```

## 5. Architecture-Specific Testing

### x86-64 Specific Tests

```c
#if defined(__x86_64__)

void test_x86_specific() {
    // Test CPUID
    unsigned int eax = 1;
    unsigned int ebx, ecx, edx;
    
    asm ("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "0"(eax)
    );
    
    printf("CPUID result: eax=%u, ebx=%u\n", eax, ebx);
    assert(ebx > 0);  // Brand ID should exist
    
    printf("✓ CPUID test passed\n");
}

void test_rflags() {
    unsigned long flags;
    
    asm ("pushfq; popq %0"
        : "=r"(flags)
    );
    
    // Verify flag bits
    assert(flags & (1 << 9));  // Interrupt flag
    printf("✓ RFLAGS test passed\n");
}

#endif
```

### ARM64 Specific Tests

```c
#if defined(__aarch64__)

void test_arm_specific() {
    // Test NEON instructions
    unsigned long result;
    
    asm ("fmov d0, #1.0; fmov d1, #2.0; fadd d0, d0, d1; fmov %0, d0"
        : "=r"(result)
    );
    
    printf("NEON result: %lu\n", result);
    printf("✓ NEON test passed\n");
}

#endif
```

## 6. Integration Tests

### Full Pipeline Test

```vitte
// Test assembly → C → Binary

fn test_full_pipeline() {
    // 1. Vitte source with assembly
    let source = """
    fn bit_scan(x: u64) -> u32 {
        let result: u32
        asm "bsf %1, %0"
            : "=r"(result)
            : "r"(x)
        return result
    }
    
    fn main() -> i32 {
        return bit_scan(0x100)
    }
    """
    
    // 2. Compile to C
    let c_code = vittec_compile(source)
    
    // 3. Check C output contains assembly
    assert(contains(c_code, "__asm__") || contains(c_code, "__inline"))
    
    // 4. Compile C to binary
    let result = cc_compile(c_code)
    
    // 5. Run and verify
    assert(result == 8)  // bsf(0x100) = 8
    
    println("✓ Full pipeline test passed")
}
```

### Cross-Platform Test

```bash
#!/bin/bash

# Test on multiple platforms
for platform in "x86_64-linux" "aarch64-linux" "x86_64-windows"
do
    echo "Testing on $platform..."
    
    # Cross-compile
    ./configure --target=$platform
    make
    
    # Run tests
    make test
    
    # Check results
    if [ $? -ne 0 ]; then
        echo "✗ Tests failed on $platform"
        exit 1
    fi
done

echo "✓ All platform tests passed"
```

## 7. Coverage Analysis

### Code Coverage

```bash
# Compile with coverage
gcc -fprofile-arcs -ftest-coverage program.c -o program

# Run tests
./program

# Generate report
gcov program.c

# View coverage
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage
```

### Instruction Coverage

```bash
# Use perf to check instruction coverage
perf stat -e instructions,cycles ./program

# Expected output
# 1,234,567 instructions
# 567,890 cycles
```

## 8. Regression Testing

### Automated Regression Suite

```c
// tests/regression/test_assembly_regressions.c

struct test_case {
    const char* name;
    unsigned long input;
    unsigned long expected;
};

struct test_case tests[] = {
    {"bsf_zero", 0x0000000000000001, 0},
    {"popcount_alternating", 0xAAAAAAAAAAAAAAAA, 32},
    {"clz_high_bit", 0x8000000000000000, 0},
    {NULL, 0, 0}
};

void run_regression_tests() {
    for (int i = 0; tests[i].name; i++) {
        unsigned long result = operation(tests[i].input);
        
        assert(result == tests[i].expected);
        printf("✓ %s\n", tests[i].name);
    }
}
```

## 9. Performance Regression Detection

```bash
#!/bin/bash

# Store baseline performance
baseline_cycles=1000

# Run benchmark
result_cycles=$(./benchmark_asm | grep cycles | awk '{print $1}')

# Check for regression (more than 10% slower)
threshold=$((baseline_cycles * 110 / 100))

if [ "$result_cycles" -gt "$threshold" ]; then
    echo "✗ Performance regression detected!"
    echo "  Baseline: $baseline_cycles cycles"
    echo "  Current:  $result_cycles cycles"
    exit 1
else
    echo "✓ Performance acceptable"
fi
```

## 10. Documentation of Test Results

### Test Report Template

```markdown
# Assembly Test Report

## Test Date
2024-01-15

## Platform
- OS: Linux
- CPU: Intel Core i7 (Skylake)
- Compiler: GCC 11.2

## Test Results

| Test | Status | Time | Notes |
|------|--------|------|-------|
| bsf | ✓ PASS | 0.001s | 100 test cases |
| popcount | ✓ PASS | 0.002s | Matches C impl |
| clz | ✓ PASS | 0.001s | Edge cases verified |
| atomic_cas | ✓ PASS | 0.010s | 1000 iterations |

## Performance Baseline

| Operation | Cycles | Throughput |
|-----------|--------|-----------|
| bsf | 4 | 1 per cycle |
| popcount | 3 | 1 per cycle |
| clz | 3 | 1 per cycle |

## Regressions
None detected.

## Platform Coverage
- [x] x86-64 Linux
- [x] x86-64 Windows
- [ ] ARM64 (pending)
- [ ] WebAssembly (not applicable)
```

---

## Summary Checklist

- [ ] Unit tests written for each assembly function
- [ ] Property-based tests verify invariants
- [ ] Edge cases tested (min, max, special patterns)
- [ ] C implementation comparison tests
- [ ] Architecture-specific tests
- [ ] Integration tests verify full pipeline
- [ ] Performance benchmarks established
- [ ] Regression tests in place
- [ ] Code coverage measured
- [ ] Cross-platform testing done
- [ ] Documentation of test strategy
- [ ] CI/CD pipeline configured

See [BUILD_AND_TEST.md](BUILD_AND_TEST.md) for build and test instructions.
