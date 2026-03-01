# src/vitte/packages/process/internal/policy.vit

## usage
- use process/internal/policy.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc command_allowlisted(program: string, allowlist: [string]) -> bool {
proc command_denylisted(program: string) -> bool {
proc shell_injection_pattern(args: [string]) -> bool {
proc shell_exec_pattern(program: string, args: [string]) -> bool {
