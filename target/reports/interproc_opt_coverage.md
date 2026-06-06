# Interproc Optimizations Coverage

- Devirtualization: PASS (16/15)
- Function specialization: PASS (13/12)
- Whole program optimization: PASS (16/15)
- Link-time optimization: PASS (14/12)

## Quantitative Metrics
- Fixtures analyzed: 5
- Devirtualized callsites (total): 16
- Specialized functions (total): 13
- Cross-module rewrites (total): 16
- LTO internalized symbols (total): 14

## Machine Exports
- target/interproc_opt/analysis.json
- target/interproc_opt/fixture_metrics.csv
