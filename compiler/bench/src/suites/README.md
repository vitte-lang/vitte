# suites

Benchmark suites by subsystem.

Registration model:
- each suite implements `bench_suite_<suite>_init()` and registers cases via `bench_register()`
- `bench_register_all_suites()` calls all suite init functions (portable across toolchains)
