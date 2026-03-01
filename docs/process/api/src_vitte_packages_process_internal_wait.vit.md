# src/vitte/packages/process/internal/wait.vit

## usage
- use process/internal/wait.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc elapsed(started_ms: int, timeout_ms: int) -> int {
proc timed_out(timeout_ms: int, requested_timeout_ms: int) -> bool {
proc success_exit() -> int {
proc timeout_exit() -> int {
