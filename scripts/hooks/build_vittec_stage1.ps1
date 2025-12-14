param(
  [switch]$InstallStubOnly
)

$ErrorActionPreference = 'Stop'

function Log([string]$msg) { Write-Host "[vitte][stage1-hook][INFO] $msg" }
function LogWarn([string]$msg) { Write-Warning "[vitte][stage1-hook] $msg" }

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$targetRoot = Join-Path $root 'target'
$stage1Root = Join-Path $targetRoot 'bootstrap\stage1'
$logDir = Join-Path $stage1Root 'logs'
$logFile = Join-Path $logDir 'stage1.build.log'
$statusFile = Join-Path $stage1Root 'status.txt'
$stage1Bin = Join-Path $stage1Root 'vittec-stage1'
$mainDebug = Join-Path $targetRoot 'debug\vittec'

$workspaceManifest = Join-Path $root 'muffin.muf'
$bootstrapManifest = Join-Path $root 'bootstrap\mod.muf'
$projectManifest = Join-Path $root 'vitte.project.muf'

$pythonStub = Join-Path $root 'bootstrap\stage1\vittec_stage1.py'

New-Item -ItemType Directory -Force -Path $logDir | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $mainDebug) | Out-Null

function WriteStatusFile() {
  $ts = (Get-Date).ToString("yyyy-MM-ddTHH:mm:sszzz")
  @(
    "# Vitte bootstrap - stage1 build status"
    "workspace_root=$root"
    "timestamp=$ts"
    "status=ok-vitte-stage1"
    "binary=$stage1Bin"
    "workspace_manifest=$workspaceManifest"
    "bootstrap_manifest=$bootstrapManifest"
    "project_manifest=$projectManifest"
    "log=$logFile"
  ) -join "`n" | Set-Content -Encoding UTF8 -LiteralPath $statusFile
}

function InstallStubStage1Driver() {
  if (-not (Test-Path $pythonStub)) {
    throw "[vitte][stage1-hook][ERROR] Missing bootstrap stub at $pythonStub"
  }

  # Create a Windows-friendly stage1 "binary": a Python entrypoint file with no extension.
  $wrapper = @"
#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
REPO_ROOT = None
for candidate in [ROOT, *ROOT.parents]:
    if (candidate / "muffin.muf").is_file():
        REPO_ROOT = candidate
        break
if REPO_ROOT is None:
    REPO_ROOT = ROOT.parents[-1]

STAGE1_DIR = REPO_ROOT / "bootstrap" / "stage1"
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))
if str(STAGE1_DIR) not in sys.path:
    sys.path.insert(0, str(STAGE1_DIR))

from vittec_stage1 import main

if __name__ == "__main__":
    raise SystemExit(main())
"@
  Set-Content -Encoding UTF8 -LiteralPath $stage1Bin -Value $wrapper

  LogWarn "Installed stub vittec-stage1 driver at $stage1Bin"
  WriteStatusFile
  Copy-Item -Force -LiteralPath $stage1Bin -Destination $mainDebug
  Log "Updated debug compiler copy -> $mainDebug"
}

Set-Content -Encoding UTF8 -LiteralPath $logFile -Value "== vitte bootstrap stage1 build (powershell) ==`nworkspace_root=$root`n"

if ($InstallStubOnly) {
  InstallStubStage1Driver
  exit 0
}

# Current Windows bootstrap uses the stub stage1 driver (Python).
InstallStubStage1Driver
Log "vittec-stage1 ready at $stage1Bin"
