# vitte-bench-c (standalone)

Bench harness C11 minimaliste, orienté **micro** et **macro** benchmarks.
- `benchc` : runner (warmup, iters, stats simples)
- `micro/*` : cases unitaires très rapides (ns/op)
- `macro/*` : workloads (ms/s, throughput)
- sortie texte + CSV optionnel

## Build
```sh
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

## Run
```sh
./benchc --list
./benchc micro:add --iters 2000000
./benchc micro:hash --iters 200000
./benchc macro:json_parse --seconds 2
./benchc --csv out.csv micro:add micro:hash
```
