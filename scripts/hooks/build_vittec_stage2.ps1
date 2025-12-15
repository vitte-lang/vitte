param(
  [switch]$StrictSmoke
)

$ErrorActionPreference = 'Stop'

function Log([string]$msg) { Write-Host "[vitte][stage2-hook][INFO] $msg" }
function LogWarn([string]$msg) { Write-Warning "[vitte][stage2-hook] $msg" }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$targetRoot = Join-Path $root 'target'
$stage2Dir = Join-Path $targetRoot 'bootstrap\stage2'
$logDir = Join-Path $stage2Dir 'logs'
$logFile = Join-Path $logDir 'stage2.log'
$statusFile = Join-Path $stage2Dir 'status.txt'
$releaseDir = Join-Path $targetRoot 'release'
$debugDir = Join-Path $targetRoot 'debug'

$python = if ($env:PYTHON -and $env:PYTHON.Trim() -ne '') { $env:PYTHON } else { 'python' }
$env:PYTHONIOENCODING = if ($env:PYTHONIOENCODING) { $env:PYTHONIOENCODING } else { 'utf-8' }
$env:PYTHONPATH = $root

$stage1Bin = Join-Path $targetRoot 'bootstrap\stage1\vittec-stage1'
$projectManifest = Join-Path $root 'vitte.project.muf'
$outBinStage2 = Join-Path $stage2Dir 'vittec-stage2'
$outBinRelease = Join-Path $releaseDir 'vittec'
$outBinDebug = Join-Path $debugDir 'vittec'
$smokeScript = Join-Path $root 'tools\vitte_tests.py'

New-Item -ItemType Directory -Force -Path $stage2Dir, $logDir, $releaseDir, $debugDir | Out-Null

if (-not (Test-Path $stage1Bin)) {
  throw "[vitte][stage2-hook][ERROR] vittec-stage1 introuvable ($stage1Bin). Lance d'abord: scripts/hooks/build_vittec_stage1.ps1"
}

$ts = (Get-Date).ToString("yyyy-MM-ddTHH:mm:sszzz")
@(
  "== vitte bootstrap stage2 build (powershell) =="
  "workspace_root=$root"
  "stage1_bin=$stage1Bin"
  "project_manifest=$projectManifest"
  "out_stage2=$outBinStage2"
  "out_release=$outBinRelease"
  "timestamp=$ts"
  ""
) -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $logFile

Log "Building vittec-stage2 via stage1 compiler"
& $python $stage1Bin build $projectManifest --out-bin $outBinStage2 --log-file $logFile
if ($LASTEXITCODE -ne 0) {
  LogWarn "vittec-stage2 build failed, see $logFile"
  throw "stage2 build failed"
}

Copy-Item -Force -LiteralPath $outBinStage2 -Destination $outBinRelease
Copy-Item -Force -LiteralPath $outBinRelease -Destination $outBinDebug

@(
  "# Vitte bootstrap - stage2 build status"
  "workspace_root=$root"
  "timestamp=$ts"
  "status=ok-vitte-stage2"
  "binary=$outBinStage2"
  "release_binary=$outBinRelease"
  "compiler_stage1=$stage1Bin"
  "project_manifest=$projectManifest"
  "log=$logFile"
) -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $statusFile

if (Test-Path $smokeScript) {
  Log "Running smoke tests via $python $smokeScript smoke"
  & $python $smokeScript smoke
  if ($LASTEXITCODE -ne 0) {
    if ($StrictSmoke) { throw "stage2 smoke tests failed" }
    LogWarn "Smoke tests failed (exit $LASTEXITCODE); continuing because -StrictSmoke was not set"
  }
} else {
  LogWarn "Script de tests introuvable: $smokeScript"
}

Log "vittec-stage2 ready at $outBinRelease"
