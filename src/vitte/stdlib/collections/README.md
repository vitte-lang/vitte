# collections

Path: `src/vitte/stdlib/collections`

## Purpose

Container and traversal surfaces such as vector, deque, queue, stack, linked list, hashmap, hashset, graph, and matrix.

## Architecture Role

Use `collections` when the shape of data matters more than the host system. This family owns grouping, ordering, indexing, and traversal concerns.

## Main Responsibilities

- Offer explicit data-structure choices for program state.
- Keep container behavior separated from business rules.
- Support larger flows where data shape and access cost both matter.

## Module Inventory

- `collections.vitl`
- `deque.vitl`
- `graph.vitl`
- `hashmap.vitl`
- `hashset.vitl`
- `linkedlist.vitl`
- `matrix.vitl`
- `queue.vitl`
- `stack.vitl`
- `vector.vitl`

## Complete Integration Story

- A build report groups diagnostics in a vector and indexes them in a hashmap.
- A scheduler stores pending work in a queue or deque.
- A graph or matrix page should explain why those shapes exist, not just list filenames.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
