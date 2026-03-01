# src/vitte/packages/process/internal/spawn.vit

## usage
- use process/internal/spawn.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc hash_text(seed: string) -> int {
proc policy_hash(program: string, profile: string, allow_shell: bool, allow_privileged: bool) -> int {
proc next_pid(program: string, args_count: int) -> int {
proc spawn_allowed_by_policy(allow_background: bool, detached: bool) -> bool {
