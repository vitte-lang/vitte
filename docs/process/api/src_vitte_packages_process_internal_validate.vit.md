# src/vitte/packages/process/internal/validate.vit

## usage
- use process/internal/validate.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc contains(text: string, needle: string) -> bool {
proc has_control_chars(text: string) -> bool {
proc env_valid(env: [string]) -> bool {
proc args_valid(args: [string]) -> bool {
proc starts_with(text: string, prefix: string) -> bool {
proc cwd_valid(cwd: string) -> bool {
