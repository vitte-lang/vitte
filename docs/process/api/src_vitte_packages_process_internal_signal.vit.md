# src/vitte/packages/process/internal/signal.vit

## usage
- use process/internal/signal.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc terminate_ok(running: bool) -> bool {
proc kill_ok(running: bool) -> bool {
