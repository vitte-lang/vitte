Param(
  [string]$OutDir = "fuzz/out",
  [string]$CorpusDir = "fuzz/targets/corpus",
  [switch]$ListOnly
)

$ErrorActionPreference = "Stop"

function Run-One([string]$bin, [string]$input) {
  if ($ListOnly) {
    Write-Host "$bin $input"
    return
  }
  & $bin $input | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "non-zero exit: $bin $input"
  }
}

function Run-TargetDir([string]$target) {
  $bin = Join-Path $OutDir ($target + ".exe")
  if (!(Test-Path $bin)) {
    Write-Host "[skip] missing $bin"
    return
  }
  $dir = Join-Path $CorpusDir $target
  if (!(Test-Path $dir)) {
    Write-Host "[skip] missing $dir"
    return
  }
  Get-ChildItem -File $dir | ForEach-Object {
    Run-One $bin $_.FullName
  }
}

Run-TargetDir "fuzz_lexer"
Run-TargetDir "fuzz_parser"
Run-TargetDir "fuzz_parser_recovery"
Run-TargetDir "fuzz_ast_invariants"
Run-TargetDir "fuzz_lowering"
Run-TargetDir "fuzz_ast_printer"
Run-TargetDir "fuzz_vitte_parser"
Run-TargetDir "fuzz_typecheck"
Run-TargetDir "fuzz_lockfile"
Run-TargetDir "fuzz_muf_parser"
Run-TargetDir "fuzz_vm_exec"
Run-TargetDir "fuzir_verify"
Run-TargetDir "fuzz_diag_json"
Run-TargetDir "fuze_vm_decode"
Run-TargetDir "fuzz_asm_verify"
