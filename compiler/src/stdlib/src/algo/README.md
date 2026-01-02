# stdlib/algo

Algorithmic utilities for the Vitte standard library.

Design constraints:
- deterministic results
- explicit allocation strategy (no hidden global state)
- small modules with clear ownership
- portable, runtime-safe defaults

Modules:
- compat/: assertions + slice helpers
- ds/: min-heap, deque, ring buffer (stubs), queue (stub)
- graph/: graph container + BFS/DFS/Dijkstra + DSU + Bellman-Ford + Floyd-Warshall + SCC + bridges (mix of real + placeholder)
- search/: linear/binary + bounds + KMP + rotated/exponential/jump (some placeholders)
- sort/: insertion/quick + merge/heap (placeholders) + radix u32 + stable merge
- numeric/: gcd/lcm, sieve, modular arithmetic helpers
- string/: rolling hash, Z-algo, Manacher (odd), edit distance (placeholder)
- random/: xorshift64, pcg32

Testing:
- tests/ contains runnable unit tests modules.
Golden:
- golden/ fixtures for stable textual outputs (formatter/disasm pipelines).
Bench:
- bench/ placeholder microbench drivers.
