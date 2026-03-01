# src/vitte/packages/process/compat/mod.vit

## usage
- use process/compat/mod.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
pick ProcState {
form ProcSpec {
form ProcHandle {
proc spec(path: string) -> ProcSpec {
proc with_args(s: ProcSpec, args: [string]) -> ProcSpec {
proc with_env_count(s: ProcSpec, env_count: int) -> ProcSpec {
proc with_dry_run(s: ProcSpec, enabled: bool) -> ProcSpec {
proc run_legacy(s: ProcSpec) -> ProcHandle {
proc is_success(h: ProcHandle) -> bool {
proc exit_code(h: ProcHandle) -> int {
proc run_shell(cmdline: string) -> int {
