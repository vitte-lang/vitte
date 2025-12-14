$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$stage1Dir = Join-Path $root 'target\bootstrap\stage1'
$logDir = Join-Path $stage1Dir 'logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$python = if ($env:PYTHON -and $env:PYTHON.Trim() -ne '') { $env:PYTHON } else { 'python' }
$env:PYTHONIOENCODING = if ($env:PYTHONIOENCODING) { $env:PYTHONIOENCODING } else { 'utf-8' }
$env:PYTHONPATH = $root

$stage0Bin = Join-Path $root 'bootstrap\bin\vittec-stage0'
$project = Join-Path $root 'vitte.project.muf'
$outBin = Join-Path $stage1Dir 'vittec-stage1'
$logFile = Join-Path $logDir 'stage1.build.log'

if (-not (Test-Path $stage0Bin)) {
  throw "[vitte][stage1][ERROR] $stage0Bin introuvable. Lance d'abord: scripts/bootstrap_stage0.ps1"
}

Write-Host "[vitte][stage1] Building stage1 via:"
Write-Host "  $python $stage0Bin build $project --out-bin $outBin --log-file $logFile"

& $python $stage0Bin build $project --out-bin $outBin --log-file $logFile
if ($LASTEXITCODE -ne 0) {
  throw "[vitte][stage1][ERROR] stage0 build failed (exit $LASTEXITCODE); see $logFile"
}

"stage1 ok" | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $stage1Dir 'status.txt')
Write-Host "[vitte][stage1] stage1 OK -> $outBin"
