# src/vitte/packages/process/internal/capture.vit

## usage
- use process/internal/capture.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc truncate_text(text: string, limit: int) -> string {
proc text_bytes(text: string) -> int {
