# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\ci_env.ps1
<#
CI environment bootstrap for Vitte.

This script:
  - detects OS/arch
  - exports a few standardized environment variables used by other CI scripts
  - prints a small environment summary (safe for logs)
  - optionally prepares directories (build/, dist/, .ci-cache/)

Usage:
  pwsh tools/scripts/ci_env.ps1
  pwsh tools/scripts/ci_env.ps1 -PrepareDirs
  pwsh tools/scripts/ci_env.ps1 -PrepareDirs -CacheDir ".ci-cache"

Notes:
  - This does NOT install dependencies; it only standardizes variables.
  - In GitHub Actions, environment exports go to $env:GITHUB_ENV automatically if present.

Exit codes:
  0 ok
#>

[CmdletBinding()]
param(
  [switch]$PrepareDirs,
  [string]$Workspace = ".",
  [string]$BuildRoot = "build",
  [string]$DistRoot = "dist",
  [string]$CacheDir = ".ci-cache"
)

function Ensure-Dir([string]$Path) {
  if (-not (Test-Path $Path)) { New-Item -ItemType Directory -Force -Path $Path | Out-Null }
}

function Arch-Normalize([string]$a) {
  switch -Regex ($a) {
    '^(amd64|x64|x86_64)$' { "x86_64" }
    '^(arm64|aarch64)$'    { "aarch64" }
    '^(x86|i386|i686)$'    { "x86" }
    default                { $a }
  }
}

function Os-Normalize {
  if ($IsWindows) { return "windows" }
  if ($IsMacOS)   { return "macos" }
  if ($IsLinux)   { return "linux" }
  return "unknown"
}

$ws = Resolve-Path -LiteralPath $Workspace
$ws = $ws.Path

$os = Os-Normalize
$arch = Arch-Normalize ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant())

# Try to infer a "target triple" (coarse)
$triple = switch ($os) {
  "windows" { if ($arch -eq "x86_64") { "x86_64-pc-windows-msvc" } elseif ($arch -eq "aarch64") { "aarch64-pc-windows-msvc" } else { "$arch-pc-windows-msvc" } }
  "macos"   { if ($arch -eq "x86_64") { "x86_64-apple-darwin" } elseif ($arch -eq "aarch64") { "aarch64-apple-darwin" } else { "$arch-apple-darwin" } }
  "linux"   { if ($arch -eq "x86_64") { "x86_64-unknown-linux-gnu" } elseif ($arch -eq "aarch64") { "aarch64-unknown-linux-gnu" } else { "$arch-unknown-linux-gnu" } }
  default   { "unknown" }
}

$buildRootAbs = Join-Path $ws $BuildRoot
$distRootAbs  = Join-Path $ws $DistRoot
$cacheAbs     = Join-Path $ws $CacheDir

# Standardized env vars (used by other scripts)
$env:VITTE_WORKSPACE = $ws
$env:VITTE_OS = $os
$env:VITTE_ARCH = $arch
$env:VITTE_TARGET = $triple
$env:VITTE_BUILD_ROOT = $buildRootAbs
$env:VITTE_DIST_ROOT = $distRootAbs
$env:VITTE_CACHE_DIR = $cacheAbs

if ($PrepareDirs) {
  Ensure-Dir $buildRootAbs
  Ensure-Dir $distRootAbs
  Ensure-Dir $cacheAbs
}

# Export into GitHub Actions environment if available
if ($env:GITHUB_ENV) {
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_WORKSPACE={0}" -f $env:VITTE_WORKSPACE)
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_OS={0}" -f $env:VITTE_OS)
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_ARCH={0}" -f $env:VITTE_ARCH)
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_TARGET={0}" -f $env:VITTE_TARGET)
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_BUILD_ROOT={0}" -f $env:VITTE_BUILD_ROOT)
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_DIST_ROOT={0}" -f $env:VITTE_DIST_ROOT)
  Add-Content -Path $env:GITHUB_ENV -Value ("VITTE_CACHE_DIR={0}" -f $env:VITTE_CACHE_DIR)
}

Write-Host "[ci_env] summary"
Write-Host ("  workspace   = {0}" -f $env:VITTE_WORKSPACE)
Write-Host ("  os          = {0}" -f $env:VITTE_OS)
Write-Host ("  arch        = {0}" -f $env:VITTE_ARCH)
Write-Host ("  target      = {0}" -f $env:VITTE_TARGET)
Write-Host ("  build_root  = {0}" -f $env:VITTE_BUILD_ROOT)
Write-Host ("  dist_root   = {0}" -f $env:VITTE_DIST_ROOT)
Write-Host ("  cache_dir   = {0}" -f $env:VITTE_CACHE_DIR)

# Optional tool versions (best-effort)
if (Get-Command cmake -ErrorAction SilentlyContinue) {
  $v = (& cmake --version | Select-Object -First 1)
  Write-Host ("  cmake       = {0}" -f $v)
}
if (Get-Command ninja -ErrorAction SilentlyContinue) {
  $v = (& ninja --version | Select-Object -First 1)
  Write-Host ("  ninja       = {0}" -f $v)
}
if (Get-Command clang -ErrorAction SilentlyContinue) {
  $v = (& clang --version | Select-Object -First 1)
  Write-Host ("  clang       = {0}" -f $v)
}
if (Get-Command gcc -ErrorAction SilentlyContinue) {
  $v = (& gcc --version | Select-Object -First 1)
  Write-Host ("  gcc         = {0}" -f $v)
}

exit 0
