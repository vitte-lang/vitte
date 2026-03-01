# Alerts Patterns

## Purpose
Operational guide for `vitte/alerts` usage in on-call conditions.

## On-Call Runbook
- Define `AlertRule` with explicit threshold/window and level (`Warn/Error/Critical/Page`).
- Attach a versioned `NotificationTemplate` with non-empty `runbook_url` for `Critical/Page`.
- Route by tenant and severity using `RoutePolicy` and `notify_routed`.
- Capture dead letters and inspect `dead_letter_reason_name(...)` for remediation.

## Escalation Map
- `Info`: email digest channel only.
- `Warn`: email to service owner.
- `Error`: webhook to incident intake.
- `Critical`: webhook + escalation operator.
- `Page`: pager primary route with immediate human ack.

Recommended mapping in package:
- `NotificationChannel.Email`
- `NotificationChannel.Webhook`
- `NotificationChannel.Pager`

## Silence Policy
- Use `PlannedSilence` windows for maintenance with label scope.
- Validate with `silence_conflicts(...)` before applying changes.
- Keep silence reason explicit and time-bounded.
- Prefer narrow scope labels (`tenant:*`, `env:*`, service labels) over global silences.

## Tenant and Trace Contract
- Use `TenantId` and `TraceId` wrappers at module boundary.
- Extract tenant via labels and keep trace propagation end-to-end.
- Route explanation (`route_explain*`) must include tenant and channel context.

## Dead Letter Handling
- Treat dead letters as first-class signals.
- Current reasons:
  - `InvalidTemplateOrRunbook`
  - `ProviderCircuitOpen`
  - `NotifyRetryExhausted`
- Track counters: `notify_attempts_total`, `notify_dead_letters_total`, `notify_failures`.

## Governance
- Stable API listed by `stable_api_surface()`.
- Any public addition requires `api_version()` bump.
- Export removals require a major bump and are enforced by export policy lint.
