param(
  [ValidateSet('build', 'stub', 'skip')]
  [string]$EnsureStage1 = $(if ($env:VITTE_STAGE0_ENSURE_STAGE1) { $env:VITTE_STAGE0_ENSURE_STAGE1 } else { 'skip' }),
  [switch]$EnsureStage1Stub,
  [switch]$Help
)

$ErrorActionPreference = 'Stop'

if ($Help) {
  @"
Usage: scripts/bootstrap_stage0.ps1 [options]

Options:
  -EnsureStage1 <MODE>     MODE: build, stub, skip (default: skip). When build,
                           run scripts/hooks/build_vittec_stage1.ps1 if the stage1
                           compiler is missing. When stub, install a Windows-friendly
                           stage1 stub driver to unblock downstream steps.
  -EnsureStage1Stub        Shortcut for -EnsureStage1 stub.
  -Help                    Show this help text.

You can also set VITTE_STAGE0_ENSURE_STAGE1 to override the default mode.
"@ | Write-Host
  exit 0
}

if ($EnsureStage1Stub) { $EnsureStage1 = 'stub' }

function Timestamp() {
  try { return (Get-Date).ToString("yyyy-MM-ddTHH:mm:sszzz") } catch { return "unknown" }
}

function EnsureDir([string]$path) {
  New-Item -ItemType Directory -Force -Path $path | Out-Null
}

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$env:VITTE_ROOT = $root
$env:PYTHONPATH = $root

$python = if ($env:PYTHON -and $env:PYTHON.Trim() -ne '') { $env:PYTHON } else { 'python' }

$hostRoot = Join-Path $root 'target\bootstrap\host'
$hostLogDir = Join-Path $hostRoot 'logs'
$hostGrammarDir = Join-Path $hostRoot 'grammar'
$hostSamplesDir = Join-Path $hostRoot 'samples'
$grammarReport = Join-Path $hostGrammarDir 'report.txt'
$samplesReport = Join-Path $hostSamplesDir 'report.txt'
$hostReportLog = Join-Path $hostRoot 'reports.log'
$stage0Log = Join-Path $hostLogDir 'stage0.log'

$stage1Bin = Join-Path $root 'target\bootstrap\stage1\vittec-stage1'
$buildStage1Hook = Join-Path $root 'scripts\hooks\build_vittec_stage1.ps1'

EnsureDir $hostLogDir
EnsureDir $hostGrammarDir
EnsureDir $hostSamplesDir
EnsureDir (Join-Path $root 'bootstrap\bin')

function GenerateGrammarReport() {
  $lines = New-Object System.Collections.Generic.List[string]
  [void]$lines.Add("# Vitte bootstrap - grammar report")
  [void]$lines.Add("generated_at=$(Timestamp)")
  [void]$lines.Add("root=$root/grammar")
  $grammarRoot = Join-Path $root 'grammar'
  if (Test-Path $grammarRoot) {
    Get-ChildItem -Recurse -File -Path $grammarRoot -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -match '\.(pest|vitte|ebnf)$' } |
      Sort-Object FullName |
      ForEach-Object {
        $rel = $_.FullName.Substring($root.Length + 1).Replace('\', '/')
        [void]$lines.Add(" - $rel")
      }
  } else {
    [void]$lines.Add("status=missing (no grammar directory found)")
  }
  $lines -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $grammarReport
}

function GenerateSamplesReport() {
  $lines = New-Object System.Collections.Generic.List[string]
  [void]$lines.Add("# Vitte bootstrap - samples report")
  [void]$lines.Add("generated_at=$(Timestamp)")
  [void]$lines.Add("root=$root/tests/data")
  $samplesRoot = Join-Path $root 'tests\data'
  if (Test-Path $samplesRoot) {
    Get-ChildItem -Recurse -File -Path $samplesRoot -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -match '\.(vitte|muf|json)$' } |
      Sort-Object FullName |
      ForEach-Object {
        $rel = $_.FullName.Substring($root.Length + 1).Replace('\', '/')
        [void]$lines.Add(" - $rel")
      }
  } else {
    [void]$lines.Add("status=missing (no tests/data directory found)")
  }
  $lines -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $samplesReport
}

function InitHostReportLog() {
  @(
    "# Vitte bootstrap - host reports"
    "generated_at=$(Timestamp)"
    "workspace_root=$root"
    "grammar_report=$grammarReport"
    "samples_report=$samplesReport"
  ) -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $hostReportLog
}

function LogHostReport([string]$line) {
  Add-Content -Encoding UTF8 -LiteralPath $hostReportLog -Value $line
}

function GenerateHostReports() {
  InitHostReportLog
  LogHostReport "status=generating grammar_report"
  GenerateGrammarReport
  LogHostReport "status=ok grammar_report"
  LogHostReport "status=generating samples_report"
  GenerateSamplesReport
  LogHostReport "status=ok samples_report"
}

function EnsureStage1Available() {
  if (Test-Path $stage1Bin) { return $true }

  switch ($EnsureStage1.ToLowerInvariant()) {
    'build' {
      if (-not (Test-Path $buildStage1Hook)) { return $false }
      & powershell -NoProfile -ExecutionPolicy Bypass -File $buildStage1Hook
      return (Test-Path $stage1Bin)
    }
    'stub' {
      if (-not (Test-Path $buildStage1Hook)) { return $false }
      & powershell -NoProfile -ExecutionPolicy Bypass -File $buildStage1Hook -InstallStubOnly
      return (Test-Path $stage1Bin)
    }
    default { return $false }
  }
}

function RefreshStage0Binary() {
  $ok = EnsureStage1Available
  if (-not $ok) {
    @(
      "[vitte][stage0] stage1 compiler not available at $stage1Bin"
      "[vitte][stage0] ensure-stage1=$EnsureStage1 did not provide stage1; stage0 binary refresh skipped"
    ) -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $stage0Log
    return
  }

  Write-Host "[vitte][stage0] Refreshing bootstrap/bin/vittec-stage0 from $stage1Bin"
  & $python (Join-Path $root 'tools\vitte_build.py') stage0 `
    --source (Join-Path $root 'source.vitte') `
    --out-bin (Join-Path $root 'bootstrap\bin\vittec-stage0') `
    --log $stage0Log
}

GenerateHostReports
RefreshStage0Binary

Write-Host "[vitte][stage0] Host reports available:"
Write-Host "  - $grammarReport"
Write-Host "  - $samplesReport"
Write-Host "  - $hostReportLog"
Write-Host "  - $stage0Log"
