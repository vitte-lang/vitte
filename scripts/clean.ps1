$ErrorActionPreference = 'Stop'

function LogInfo([string]$msg) { Write-Host "[vitte][clean][INFO] $msg" }
function LogWarn([string]$msg) { Write-Warning "[vitte][clean] $msg" }
function Die([string]$msg) { throw "[vitte][clean][ERROR] $msg" }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$muffin = Join-Path $root 'muffin.muf'
$scriptsDir = Join-Path $root 'scripts'
if (-not (Test-Path $muffin)) { Die "muffin.muf not found at workspace root ($root); aborting clean." }
if (-not (Test-Path $scriptsDir)) { Die "scripts/ directory not found at workspace root ($root); aborting clean." }

$targetDir = Join-Path $root 'target'

if (Test-Path $targetDir) {
  LogInfo "Cleaning target directory: $targetDir"
  Get-ChildItem -Force -LiteralPath $targetDir | Remove-Item -Recurse -Force
  LogInfo "target/ cleaned. Leaving the directory itself in place."
} else {
  LogInfo "No target/ directory found; creating: $targetDir"
  New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
}

LogInfo "Cleaning miscellaneous temporary files (if any)"
$patterns = @('*.tmp', '*.swp', '*~', '.DS_Store')
$roots = @(
  (Join-Path $root 'src'),
  (Join-Path $root 'tests'),
  (Join-Path $root 'bootstrap')
)

foreach ($scanRoot in $roots) {
  if (-not (Test-Path $scanRoot)) { continue }
  foreach ($pattern in $patterns) {
    Get-ChildItem -Recurse -Force -Path $scanRoot -Filter $pattern -File -ErrorAction SilentlyContinue |
      ForEach-Object {
        LogInfo "Removing temp file: $($_.FullName)"
        Remove-Item -Force -LiteralPath $_.FullName
      }
  }
}

LogInfo "Clean completed."
LogInfo "  - Workspace root: $root"
LogInfo "  - target/ directory cleaned: $targetDir"
