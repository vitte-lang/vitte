# kernel

Path: `src/vitte/stdlib/kernel`

## Purpose

System-facing runtime helpers such as process, scheduler, threads, sync, users, signals, network, device, and memory.

## Architecture Role

Use `kernel` when the program explicitly models system services, scheduling, process behavior, or device-facing coordination.

## Main Responsibilities

- Own explicit system-service and runtime control surfaces.
- Keep platform-facing concerns separate from pure collections or data flows.
- Document the cost and coupling of these interfaces.

## Module Inventory

- `device.vitl`
- `fileio.vitl`
- `interrupt.vitl`
- `memory.vitl`
- `network.vitl`
- `process.vitl`
- `scheduler.vitl`
- `signals.vitl`
- `sync.vitl`
- `threads.vitl`
- `users.vitl`

## Complete Integration Story

- A service manager may use scheduler, process, and signals while keeping policy in separate code.
- A network-facing runtime should explain why it depends on kernel surfaces instead of lighter families.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
