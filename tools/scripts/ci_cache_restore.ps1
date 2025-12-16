# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\ci_cache_restore.ps1
<#
CI cache restore helper for Vitte.

Goal:
  Restore build caches from a pre-populated cache directory into the workspace,
  to speed up CI builds. This script is intentionally generic: it can run on
  GitHub Actions, GitLab CI, Azure Pipelines, etc.

Typical caches:
  - build/ (CMake build trees)      [optional, can be large]
  - .cache/ (tools cache)           [optional]
  - dist/  (downloaded toolchains)  [optional]
  - muffin/ or .muffin/ cache       [optional]
  - rust/target (if rust workspace) [optional]

Usage:
  pwsh tools/scripts/ci_cache_restore.ps1
  pwsh tools/scripts/ci_cache_restore.ps1 -CacheDir "$env:RUNNER_TEMP/vitte-cache"
  pwsh tools/scripts/ci_cache_restore.ps1 -CacheDir ".ci-cache" -What Build,Tool -Verbose

Exit codes:
  0 ok
  2 cache dir missing / nothing restored
#>

[CmdletBinding()]
param(
  # Where the CI system placed the restored cache (input).
  [string]$CacheDir = ".ci-cache",

  # Workspace root (repo root). Defaults to current directory.
  [string]$Workspace = ".",

  # What to restore.
  [ValidateSet("Build","Tool","Dist","Muffin","Rust","All")]
  [string[]]$What = @("All"),

  # If set, also restore CMake build directories (can be large).
  [switch]$RestoreBuildDirs,

  # If set, fail if nothing could be restored.
  [switch]$FailIfEmpty
)

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }

function Ensure-Dir([string]$Path) {
  if (-not (Test-Path $Path)) { New-Item -ItemType Directory -Force -Path $Path | Out-Null }
}

function Copy-DirIfExists([string]$Src, [string]$Dst) {
  if (-not (Test-Path $Src)) { return $false }
  Ensure-Dir (Split-Path $Dst -Parent)
  # Mirror semantics: copy recursively; best-effort overwrite.
  Copy-Item -Recurse -Force -Path $Src -Destination $Dst
  return $true
}

function Normalize-What([string[]]$items) {
  if ($items -contains "All") {
    return @("Build","Tool","Dist","Muffin","Rust")
  }
  return $items
}

$What = Normalize-What $What

$cacheAbs = Resolve-Path -LiteralPath $CacheDir -ErrorAction SilentlyContinue
if (-not $cacheAbs) {
  $msg = "[cache] missing cache dir: $CacheDir"
  if ($FailIfEmpty) { Fail $msg 2 } else { Write-Host $msg; exit 0 }
}
$cacheAbs = $cacheAbs.Path

$wsAbs = Resolve-Path -LiteralPath $Workspace -ErrorAction SilentlyContinue
if (-not $wsAbs) { Fail "[cache] workspace not found: $Workspace" 2 }
$wsAbs = $wsAbs.Path

Write-Host ("[cache] restore from={0} to={1}" -f $cacheAbs, $wsAbs)

$restored = New-Object System.Collections.Generic.List[string]

# Layout inside CacheDir (recommended):
#   build/                (optional)
#   .cache/               (optional)
#   dist/                 (optional)
#   .muffin/ or muffin/   (optional)
#   rust/target/          (optional)
#
# We restore into the workspace root, preserving folder names.
foreach ($w in $What) {
  switch ($w) {
    "Build" {
      if (-not $RestoreBuildDirs) {
        Write-Host "[cache] skip build/ (RestoreBuildDirs not set)"
        break
      }
      $src = Join-Path $cacheAbs "build"
      $dst = Join-Path $wsAbs "build"
      if (Copy-DirIfExists $src $dst) { $restored.Add("build") | Out-Null }
      else { Write-Verbose "[cache] build/ not found in cache" }
    }

    "Tool" {
      $src = Join-Path $cacheAbs ".cache"
      $dst = Join-Path $wsAbs ".cache"
      if (Copy-DirIfExists $src $dst) { $restored.Add(".cache") | Out-Null }
      else { Write-Verbose "[cache] .cache/ not found in cache" }
    }

    "Dist" {
      $src = Join-Path $cacheAbs "dist"
      $dst = Join-Path $wsAbs "dist"
      if (Copy-DirIfExists $src $dst) { $restored.Add("dist") | Out-Null }
      else { Write-Verbose "[cache] dist/ not found in cache" }
    }

    "Muffin" {
      $src1 = Join-Path $cacheAbs ".muffin"
      $dst1 = Join-Path $wsAbs ".muffin"
      $src2 = Join-Path $cacheAbs "muffin"
      $dst2 = Join-Path $wsAbs "muffin"

      $ok = $false
      if (Copy-DirIfExists $src1 $dst1) { $restored.Add(".muffin") | Out-Null; $ok = $true }
      if (Copy-DirIfExists $src2 $dst2) { $restored.Add("muffin") | Out-Null; $ok = $true }

      if (-not $ok) { Write-Verbose "[cache] muffin cache not found in cache" }
    }

    "Rust" {
      # If you cache rust/target, recommended structure is rust/target/
      $src = Join-Path $cacheAbs "rust/target"
      $dst = Join-Path $wsAbs "rust/target"
      if (Copy-DirIfExists $src $dst) { $restored.Add("rust/target") | Out-Null }
      else { Write-Verbose "[cache] rust/target not found in cache" }
    }
  }
}

if ($restored.Count -eq 0) {
  $msg = "[cache] nothing restored"
  if ($FailIfEmpty) { Fail $msg 2 } else { Write-Host $msg; exit 0 }
}

Write-Host ("[cache] restored: {0}" -f ($restored -join ", "))
exit 0
