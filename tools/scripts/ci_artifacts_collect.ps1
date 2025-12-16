# tools/scripts/ci_artifacts_collect.ps1
<#
CI artifact collector for Vitte.

Collects:
  - binaries (vittec)
  - debug symbols (best-effort)
  - logs (if present)
  - checksums.sha256 (if present)
  - selected metadata files (VERSION, LICENSE, muffin.muf, muffin.lock)

Produces:
  dist/artifacts/<stamp>/...
  dist/artifacts/<stamp>.zip

Usage:
  pwsh tools/scripts/ci_artifacts_collect.ps1
  pwsh tools/scripts/ci_artifacts_collect.ps1 -Mode Release -OutRoot dist/artifacts
  pwsh tools/scripts/ci_artifacts_collect.ps1 -Mode Debug -IncludeBuildDir

Exit codes:
  0 ok
  2 failed to collect (missing expected)
  3 packaging failed
#>

[CmdletBinding()]
param(
  [ValidateSet("Release","Debug","Asan","RelWithDebInfo")]
  [string]$Mode = "Release",

  [string]$BuildRoot = "build",

  [string]$OutRoot = "dist/artifacts",

  [string]$BinName = "vittec",

  [string]$Stamp = "",

  [switch]$IncludeBuildDir
)

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }

function Utc-Stamp {
  (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
}

function BuildDir-ForMode([string]$M) {
  switch ($M) {
    "Release" { return Join-Path $BuildRoot "release" }
    "Debug"   { return Join-Path $BuildRoot "debug" }
    "Asan"    { return Join-Path $BuildRoot "asan" }
    "RelWithDebInfo" { return Join-Path $BuildRoot "relwithdebinfo" }
  }
}

function Copy-IfExists([string]$Src, [string]$Dst) {
  if (Test-Path $Src) {
    New-Item -ItemType Directory -Force -Path (Split-Path $Dst -Parent) | Out-Null
    Copy-Item -Force -Recurse -Path $Src -Destination $Dst
    return $true
  }
  return $false
}

function Resolve-Bin([string]$Dir, [string]$Name) {
  $p1 = Join-Path $Dir $Name
  $p2 = Join-Path $Dir ($Name + ".exe")
  if (Test-Path $p1) { return $p1 }
  if (Test-Path $p2) { return $p2 }
  return $null
}

if (-not (Get-Command Compress-Archive -ErrorAction SilentlyContinue)) {
  Fail "Compress-Archive not found (needs PowerShell 5+ / pwsh)." 2
}

if ([string]::IsNullOrWhiteSpace($Stamp)) { $Stamp = Utc-Stamp }

$BuildDir = BuildDir-ForMode $Mode
$Dest = Join-Path $OutRoot $Stamp

New-Item -ItemType Directory -Force -Path $Dest | Out-Null
Write-Host ("[collect] mode={0} build_dir={1} out={2}" -f $Mode, $BuildDir, $Dest)

# Metadata
Copy-IfExists "VERSION"        (Join-Path $Dest "meta/VERSION")        | Out-Null
Copy-IfExists "LICENSE"        (Join-Path $Dest "meta/LICENSE")        | Out-Null
Copy-IfExists "muffin.muf"     (Join-Path $Dest "meta/muffin.muf")     | Out-Null
Copy-IfExists "muffin.lock"    (Join-Path $Dest "meta/muffin.lock")    | Out-Null
Copy-IfExists ".editorconfig"  (Join-Path $Dest "meta/.editorconfig")  | Out-Null
Copy-IfExists "CMakeLists.txt" (Join-Path $Dest "meta/CMakeLists.txt") | Out-Null
Copy-IfExists "Makefile"       (Join-Path $Dest "meta/Makefile")       | Out-Null
Copy-IfExists "checksums.sha256" (Join-Path $Dest "meta/checksums.sha256") | Out-Null

# Binary
$bin = Resolve-Bin -Dir $BuildDir -Name $BinName
if (-not $bin) {
  Fail "Binary not found in $BuildDir ($BinName). Build first." 2
}
New-Item -ItemType Directory -Force -Path (Join-Path $Dest "bin") | Out-Null
Copy-Item -Force -Path $bin -Destination (Join-Path $Dest "bin") | Out-Null

# Symbols (best-effort)
New-Item -ItemType Directory -Force -Path (Join-Path $Dest "symbols") | Out-Null

# PDB next to exe (MSVC) or other symbol files (best-effort)
$pdb = [System.IO.Path]::ChangeExtension($bin, ".pdb")
if (Test-Path $pdb) {
  Copy-Item -Force -Path $pdb -Destination (Join-Path $Dest "symbols") | Out-Null
}

# Logs (best-effort)
Copy-IfExists (Join-Path $BuildDir "Testing/Temporary/LastTest.log") (Join-Path $Dest "logs/LastTest.log") | Out-Null
Copy-IfExists (Join-Path $BuildDir "Testing/Temporary/LastTestsFailed.log") (Join-Path $Dest "logs/LastTestsFailed.log") | Out-Null

# Optional build outputs
if ($IncludeBuildDir) {
  Copy-IfExists (Join-Path $BuildDir "compile_commands.json") (Join-Path $Dest "build/compile_commands.json") | Out-Null
  Copy-IfExists (Join-Path $BuildDir "CMakeCache.txt") (Join-Path $Dest "build/CMakeCache.txt") | Out-Null
}

# Archive as zip
$zip = Join-Path $OutRoot ($Stamp + ".zip")
if (Test-Path $zip) { Remove-Item -Force $zip }

try {
  Compress-Archive -Path $Dest -DestinationPath $zip -Force
  Write-Host ("[collect] archive={0}" -f $zip)
} catch {
  Fail ("Packaging failed: {0}" -f $_) 3
}

Write-Host "[collect] done"
exit 0
