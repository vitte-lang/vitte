

# -----------------------------------------------------------------------------
# run_large.ps1 - Regenerate a large/stress dataset (PowerShell)
# -----------------------------------------------------------------------------
# This runner is intended for local perf/stress refreshes and long CI jobs.
# It runs the generator driver with a "large" preset and validates the result.
#
# Usage examples:
#   pwsh ./run_large.ps1
#   pwsh ./run_large.ps1 -Preset ./presets/large.json -Generated ../generated_large
#   pwsh ./run_large.ps1 -Python python3
#   pwsh ./run_large.ps1 -ExtraArgs "--seed 0 --count 500 --stress 6 --emit-md"
#   pwsh ./run_large.ps1 -RegenOnly
#   pwsh ./run_large.ps1 -ValidateOnly
#   pwsh ./run_large.ps1 -DryRun
# -----------------------------------------------------------------------------

[CmdletBinding(PositionalBinding=$false)]
param(
  [string] $Python = "python",
  [string] $Preset = "./presets/large.json",
  [string] $Generated = "../generated_large",

  [switch] $RegenOnly,
  [switch] $ValidateOnly,

  # Additional args forwarded to driver.py (single string; split on whitespace)
  [string] $ExtraArgs = "",

  # If set, print commands but do not execute.
  [switch] $DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Section([string] $Title) {
  Write-Host ""
  Write-Host "=== $Title ==="
}

function Resolve-PathSafe([string] $p) {
  $base = Split-Path -Parent $PSCommandPath
  $cand = Join-Path $base $p
  return [System.IO.Path]::GetFullPath($cand)
}

function Split-Args([string] $s) {
  if ([string]::IsNullOrWhiteSpace($s)) { return @() }
  return ($s -split "\s+") | Where-Object { $_ -ne "" }
}

function Invoke-Checked([string] $Exe, [string[]] $Args) {
  if ($DryRun) {
    $joined = ($Args | ForEach-Object { if ($_ -match "\s") { '"' + $_ + '"' } else { $_ } }) -join " "
    Write-Host "[dry-run] $Exe $joined"
    return
  }

  & $Exe @Args
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code $LASTEXITCODE: $Exe $($Args -join ' ')"
  }
}

# Move to script dir for stable relative paths
$Here = Split-Path -Parent $PSCommandPath
Set-Location $Here

# Resolve paths
$PresetPath = Resolve-PathSafe $Preset
$GeneratedPath = Resolve-PathSafe $Generated

# Tools
$Driver = Resolve-PathSafe "./driver.py"
$Validator = Resolve-PathSafe "./validate_dataset.py"

if (-not (Test-Path $Driver)) {
  throw "driver.py not found at: $Driver"
}
if (-not (Test-Path $Validator)) {
  throw "validate_dataset.py not found at: $Validator"
}

$extra = Split-Args $ExtraArgs

$doRegen = $true
$doValidate = $true
if ($RegenOnly) { $doValidate = $false }
if ($ValidateOnly) { $doRegen = $false }

if (-not $doRegen -and -not $doValidate) {
  throw "Nothing to do: both regen and validate are disabled"
}

try {
  if ($doRegen) {
    Write-Section "Regenerate large/stress fixtures"
    $args = @($Driver, "--preset", $PresetPath, "--generated", $GeneratedPath) + $extra
    Invoke-Checked $Python $args
  }

  if ($doValidate) {
    Write-Section "Validate dataset"
    $args = @($Validator, "--root", $GeneratedPath)
    Invoke-Checked $Python $args
  }

  Write-Section "OK"
  Write-Host "generated: $GeneratedPath"
}
catch {
  Write-Host ""
  Write-Host "[error] $($_.Exception.Message)" -ForegroundColor Red
  exit 1
}