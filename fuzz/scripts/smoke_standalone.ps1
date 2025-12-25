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

function Get-SmokeTargets {
  $raw = & python fuzz/scripts/target_map.py smoke-targets
  if ($LASTEXITCODE -eq 0 -and $raw) {
    return ($raw -split "`n" | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
  }

  return @(
    "fuzz_lexer",
    "fuzz_parser",
    "fuzz_parser_recovery",
    "fuzz_ast_invariants",
    "fuzz_lowering",
    "fuzz_ast_printer",
    "fuzz_vitte_parser",
    "fuzz_typecheck",
    "fuzz_lockfile",
    "fuzz_muf_parser",
    "fuzz_vm_exec",
    "fuzir_verify",
    "fuzz_diag_json",
    "fuze_vm_decode",
    "fuzz_asm_verify"
  )
}

function Get-StandaloneCorpusRoot([string]$defaultRoot) {
  $raw = & python fuzz/scripts/target_map.py standalone-corpus-root
  if ($LASTEXITCODE -eq 0 -and $raw) {
    return $raw.Trim()
  }
  return $defaultRoot
}

$CorpusDir = Get-StandaloneCorpusRoot $CorpusDir

Get-SmokeTargets | ForEach-Object { Run-TargetDir $_ }
