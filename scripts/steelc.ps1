param(
  [Parameter(Mandatory = $true)]
  [ValidateSet('build-debug', 'build-release')]
  [string]$Command
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$vittec = Join-Path $root 'target\release\vittec'
if (-not (Test-Path $vittec)) {
  throw "[vitte][steelc][ERROR] $vittec introuvable. Lance d'abord : scripts/hooks/build_vittec_stage2.ps1"
}

switch ($Command) {
  'build-debug' {
    $outBin = Join-Path $root 'target\debug\vittec'
    New-Item -ItemType Directory -Force -Path (Split-Path $outBin) | Out-Null
    Copy-Item -Force -LiteralPath $vittec -Destination $outBin
    Write-Host "[vitte][steelc] build-debug -> $outBin"
  }
  'build-release' {
    $outBin = Join-Path $root 'target\release\vittec'
    New-Item -ItemType Directory -Force -Path (Split-Path $outBin) | Out-Null
    if ((Resolve-Path $outBin).Path -ne (Resolve-Path $vittec).Path) {
      Copy-Item -Force -LiteralPath $vittec -Destination $outBin
    }
    Write-Host "[vitte][steelc] build-release -> $outBin"
  }
}
