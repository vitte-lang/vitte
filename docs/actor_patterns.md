# Actor Architecture Patterns (vitte/actor)

## Worker-Pool
- Build a router with bounded workers and sticky keys:
  - `router_new(name, min, max, scale_up_backlog, scale_down_backlog, semantics, retry_limit)`
  - `router_dispatch(...)` or `router_dispatch_secure(...)`
- Use `router_worker_count()` and `admin_list_actors()` for live operations.

## Saga (Compensation)
- Model each saga step as an actor message (`kind=step_x`).
- On failure (`fail` + supervision), emit compensation messages (`kind=compensate_x`).
- Keep idempotence via `message_id` and bounded retries via `send_with_delivery`.

## Event-Bus
- Use one router as bus ingress and shard by `(tenant_id, topic_or_entity)`.
- Enforce tenant isolation with:
  - `router_with_tenant_limit(...)`
  - `router_with_auth_rule(...)`
  - `router_dispatch_secure(...)`
- Inspect denied traffic with `tenant_denied_count(...)`.

## Request-Reply
- Open request lifecycle with `ask(actor, request_id, message, timeout_ticks)`.
- Drive runtime with `step(...)` and track state via `ask_state(...)`.
- Cancel late requests with `cancel_ask(...)`.

## Reliability Checklist
- Delivery semantics explicit: `AtMostOnce` / `AtLeastOnce`.
- Protocol ack/nack: `delivery_attempt(...)` + dead-letter fallback.
- Fault isolation checks: `family_fail_child_isolated(...)` and `family_sibling_unchanged(...)`.
- Runtime hardening: `fuzz_runtime(...)`, `property_invariants(...)`, `benchmark_suite(...)`.
