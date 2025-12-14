$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$env:VITTE_ROOT = $root
$env:VITTE_TARGET = Join-Path $root 'target'
$env:VITTE_BOOTSTRAP = Join-Path $env:VITTE_TARGET 'bootstrap'

# Add target/release to PATH so `vittec` is discoverable in the session.
$releaseDir = Join-Path $env:VITTE_TARGET 'release'
if (Test-Path $releaseDir) {
  $pathParts = $env:PATH -split ';'
  if ($pathParts -notcontains $releaseDir) {
    $env:PATH = ($releaseDir, $env:PATH) -join ';'
  }
}

Write-Host "[vitte][env] ROOT=$env:VITTE_ROOT"
Write-Host "[vitte][env] TARGET=$env:VITTE_TARGET"
Write-Host "[vitte][env] BOOTSTRAP=$env:VITTE_BOOTSTRAP"
