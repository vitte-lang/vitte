# Advanced Suggestions Architecture (Max)

## 1. Goal

Build a production-grade completion engine for the VS Code extension with:

- low-latency first paint
- high acceptance quality
- predictable fallback behavior
- explainable ranking decisions
- offline resilience

This document maps product ideas to implementation modules, TypeScript interfaces, rollout phases, and KPIs.

## 2. Scope (Backlog Mapping)

In scope:

- Adaptive Ranking
- Suggestion Budgeting
- Streaming Completion
- Intent Modes
- Semantic Blocks
- Inline Fix + Complete
- Cross-file Prediction
- Acceptance Feedback Loop
- Latency Guardrails
- Explainable Suggestions
- Conflict-aware Suggestions
- Refactor-aware Autocomplete
- Promptless Snippets
- Multi-cursor Smart Completion
- Offline Semantic Cache

Out of scope (for this doc):

- cloud training pipelines
- remote telemetry backend design
- language model inference hosting

## 3. High-Level Flow

1. User triggers completion.
2. `pipeline` starts with a strict latency budget.
3. Local providers return first batch quickly.
4. LSP/plugins/index providers stream additional batches.
5. `ranking` merges + scores.
6. `safety` filters invalid/risky items.
7. `budgeting` truncates dynamic top-N.
8. UI renders incremental updates with explain labels.
9. Accepted/rejected actions feed local learning store.

## 4. Proposed Module Layout (`src/`)

Add these folders:

- `src/suggestions/pipeline/`
- `src/suggestions/providers/`
- `src/suggestions/ranking/`
- `src/suggestions/modes/`
- `src/suggestions/safety/`
- `src/suggestions/cache/`
- `src/suggestions/feedback/`
- `src/suggestions/ui/`
- `src/suggestions/telemetry/`
- `src/suggestions/types/`

## 5. Core TypeScript Contracts

```ts
// src/suggestions/types/contracts.ts
export type SuggestionSource = "local" | "lsp" | "plugin" | "index" | "snippet" | "predictor";
export type IntentMode = "code" | "test" | "refactor" | "docs" | "terminal";

export interface SuggestionContext {
  uri: string;
  languageId: string;
  line: number;
  character: number;
  prefix: string;
  surroundingText: string;
  importedModules: string[];
  diagnostics: Array<{ code?: string; severity: number; message: string }>;
  intentMode: IntentMode;
  multiCursorCount: number;
  isRefactorSession: boolean;
  workspaceId: string;
}

export interface SuggestionItem {
  id: string;
  label: string;
  insertText: string;
  detail?: string;
  documentation?: string;
  kind?: number;
  source: SuggestionSource;
  typeTag?: "symbol" | "function" | "module" | "snippet" | "block" | "fix";
  score?: number;
  explain?: string[];
  conflicts?: string[];
  latencyMs?: number;
}

export interface SuggestionProvider {
  name: string;
  priority: number;
  stream(ctx: SuggestionContext, signal: AbortSignal): AsyncIterable<SuggestionItem[]>;
}

export interface SuggestionBudget {
  firstPaintTopN: number;
  steadyTopN: number;
  hardMaxN: number;
}
```

## 6. Pipeline Orchestrator

Responsibilities:

- launch providers concurrently
- enforce timeout/fallback policies
- stream partial updates to UI
- perform final merge/rank/filter/budget

```ts
// src/suggestions/pipeline/orchestrator.ts
export interface OrchestratorConfig {
  lspTimeoutMs: number;          // e.g. 180-250
  globalTimeoutMs: number;       // e.g. 800
  firstPaintDeadlineMs: number;  // e.g. 60-90
}
```

Behavior:

- By `firstPaintDeadlineMs`: return local/index/snippet batch.
- If LSP late: continue without blocking, mark `lsp_timeout=true`.
- If global deadline reached: finalize with what exists.

## 7. Adaptive Ranking

Features for scoring:

- prefix/fuzzy quality
- same file / same module
- import proximity
- recent usage frequency
- historical acceptance rate
- diagnostic relevance
- source reliability prior

Simple weighted score:

`score = w1*prefix + w2*fuzzy + w3*sameModule + w4*usage + w5*acceptRate + w6*diagMatch + w7*sourcePrior`

Keep weights in config:

```json
{
  "vitte.suggestions.ranking.weights": {
    "prefix": 3.0,
    "fuzzy": 1.2,
    "sameModule": 0.9,
    "usage": 0.7,
    "acceptRate": 1.1,
    "diagMatch": 0.8,
    "sourcePrior": 0.5
  }
}
```

## 8. Suggestion Budgeting

Dynamic top-N based on latency and load:

- low latency + low CPU: `top 50`
- medium: `top 30`
- degraded: `top 20`

Inputs:

- last p95 completion latency
- extension host event loop lag
- multi-cursor count

## 9. Streaming Completion

Provider order:

1. local AST + snippets
2. offline semantic cache
3. plugin providers
4. LSP
5. cross-file predictor

UI updates:

- first list paint quickly
- append/refresh as batches arrive
- preserve selection when stable IDs match

## 10. Intent Modes

Fast switching command:

- `Vitte: Set Suggestion Intent Mode`

Mode filters examples:

- `test`: prioritize test helpers/assertions
- `refactor`: prioritize rename-safe signatures/import rewrites
- `docs`: prioritize doc blocks and comment templates
- `terminal`: shell/task snippets and command actions

## 11. Semantic Blocks + Promptless Snippets

`block provider` returns compound proposals:

- function + imports + docstring
- test skeleton + fixture + assertions
- refactor pattern block

Promptless generation:

- learned style from repository patterns
- naming conventions and error handling style

## 12. Inline Fix + Complete

When diagnostics exist at cursor:

- propose combined edit (`fix + completion`) as one completion item
- include `conflicts` and `explain`

Example explain:

- `["diagnostic match: VITTE-L102", "same module", "accepted 73% recently"]`

## 13. Cross-file Prediction

Offline graph features:

- call adjacency matrix
- import co-occurrence
- symbol transition n-grams

Use lightweight predictor (non-ML first):

- top transitions from current symbol/module

## 14. Acceptance Feedback Loop

Local store (SQLite or JSONL):

- per-suggestion accept/reject counts
- context hash (file type/module)
- decay older signals

Expose reset command:

- `Vitte: Reset Suggestion Learning`

## 15. Latency Guardrails

Hard rules:

- never block first paint on LSP
- abort stale requests on cursor move
- timeout any provider exceeding budget

Telemetry fields:

- `first_paint_ms`
- `finalize_ms`
- `provider_lsp_ms`
- `provider_local_ms`
- `items_pre_filter`
- `items_post_filter`

## 16. Explainable Suggestions UI

Add optional detail line:

- source badge
- short why-labels

Settings:

- `vitte.suggestions.showExplainLabels` (default `true`)

## 17. Conflict-aware Suggestions

Pre-display checks:

- lint rule violations
- forbidden APIs
- security policy tags
- incompatible imports

Policy:

- drop hard-conflict items
- down-rank soft conflicts

## 18. Refactor-aware Autocomplete

During rename/move operations:

- track refactor session token
- boost migrated symbols/new import paths
- suggest updated signatures only

## 19. Multi-cursor Smart Completion

Strategy:

- generate baseline completion at primary cursor
- adapt placeholders per cursor context
- validate all edits are non-overlapping

## 20. Offline Semantic Cache

Storage:

- workspace local cache dir
- versioned schema + index checksum

Entries:

- symbol -> files/usages
- import suggestions
- recent accepted blocks

Warm-up:

- startup background indexing
- incremental updates on file save

## 21. VS Code Integration Points

- `CompletionItemProvider` wrapper delegates to orchestrator.
- `InlineCompletionItemProvider` for inline fix+complete.
- commands for intent mode and diagnostics-aware actions.
- status bar item for mode + latency health.
- webview/panel for perf diagnostics.

## 22. Settings Surface (`package.json`)

- `vitte.suggestions.enabled`
- `vitte.suggestions.intentMode`
- `vitte.suggestions.maxItems`
- `vitte.suggestions.lspTimeoutMs`
- `vitte.suggestions.streaming`
- `vitte.suggestions.showExplainLabels`
- `vitte.suggestions.conflictPolicy`
- `vitte.suggestions.learning.enabled`
- `vitte.suggestions.offlineCache.enabled`
- `vitte.suggestions.offlineCache.maxMb`

## 23. KPI Dashboard

Primary:

- acceptance rate
- first paint p50/p95
- finalize p50/p95
- timeout rate

Secondary:

- rejection after accept (edit distance signal)
- % items filtered by safety
- mode-specific acceptance

## 24. Rollout Plan

### Sprint 1-2 (Foundation)

- pipeline skeleton
- local + snippet + LSP providers
- latency guardrails
- telemetry base

### Sprint 3-4 (Quality)

- adaptive ranking v1
- budgeting
- explain labels
- acceptance loop store

### Sprint 5-6 (Experience)

- streaming UI updates
- intent modes
- conflict-aware filtering
- offline cache v1

### Sprint 7-8 (Innovation)

- semantic blocks
- inline fix+complete
- cross-file predictor
- refactor-aware + multi-cursor enhancements

## 25. Testing Strategy

Unit:

- score feature correctness
- provider timeout handling
- dedup/merge determinism

Integration:

- first paint under budgets
- fallback correctness on LSP failure
- safety filter enforcement

Replay tests:

- deterministic traces from real completion sessions

## 26. Risks and Mitigations

- ranking overfit to one project:
  - use per-workspace model + safe defaults
- noisy telemetry:
  - sample rate and schema versioning
- UI flicker with streaming:
  - stable IDs and selection pinning
- large cache growth:
  - size cap + LRU eviction

## 27. Immediate Next PRs

1. Add `src/suggestions/types/contracts.ts` + orchestrator skeleton.
2. Add settings for timeout/streaming/max items.
3. Wire first provider chain (`local -> snippets -> lsp`).
4. Add minimal perf telemetry and output channel.

