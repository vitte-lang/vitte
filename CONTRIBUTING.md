# Contributing to vitte-bench

## Adding a New Benchmark

### Micro Benchmarks

1. Create a new file `src/micro/bm_<name>.c`
2. Implement function: `void bm_<name>(void* ctx)`
3. Register in `src/bench/bench_registry.c`

Example:
```c
void bm_strlen(void* ctx) {
  (void)ctx;
  const char* str = "hello";
  volatile size_t len = 0;
  for (int i = 0; i < 1000000; i++) {
    len = strlen(str);
  }
}
```

### Macro Benchmarks

1. Create a new file `src/macro/bm_<name>.c`
2. Implement function: `void bm_<name>(void* ctx)`
3. Register in `src/bench/bench_registry.c`

Example:
```c
void bm_file_io(void* ctx) {
  (void)ctx;
  FILE* f = fopen("test.txt", "w");
  for (int i = 0; i < 1000; i++) {
    fprintf(f, "line %d\n", i);
  }
  fclose(f);
}
```

## Running Benchmarks

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
./benchc --all
```

## Tips

- Use `volatile` to prevent optimization of dead code
- Keep micro benchmarks small and self-contained
- Use `sink` variables to prevent results from being optimized away
- Consider using inline assembly for tight loops
