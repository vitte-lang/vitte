<#
toolchain/scripts/clean.ps1

Vitte toolchain - clean build artifacts (PowerShell, Windows-friendly).

Usage:
  pwsh ./toolchain/scripts/clean.ps1
  pwsh ./toolchain/scripts/clean.ps1 --all
  pwsh ./toolchain/scripts/clean.ps1 --dist
  pwsh ./toolchain/scripts/clean.ps1 --cache
  pwsh ./toolchain/scripts/clean.ps1 --dry-run

Notes:
  - Designed to be safe: only removes known build/artifact directories/files.
  - Does not call sh/bash (works on "pure" Windows).
#>

[CmdletBinding(PositionalBinding = $false)]
param(
  [switch]$All,
  [switch]$Dist,
  [switch]$Cache,
  [switch]$DryRun,
  [switch]$VerboseMode,
  [string]$RootDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Log([string]$Message) {
  if ($VerboseMode) { Write-Host $Message }
}

function Resolve-RootDir {
  if ($RootDir -and $RootDir.Trim().Length -gt 0) {
    return (Resolve-Path -LiteralPath $RootDir).Path
  }
  # script dir -> ../..
  $here = Split-Path -Parent $PSCommandPath
  return (Resolve-Path -LiteralPath (Join-Path $here "..\\..")).Path
}

function Assert-SafePath([string]$PathToRemove) {
  if (-not $PathToRemove) { return }
  $full = $PathToRemove
  try { $full = (Resolve-Path -LiteralPath $PathToRemove -ErrorAction Stop).Path } catch { }

  $p = $full.TrimEnd("\\", "/")
  if ($p -eq "" -or $p -eq "\\" -or $p -match "^[A-Za-z]:$") {
    throw "refuse to remove dangerous path: $PathToRemove"
  }
  $home = [Environment]::GetFolderPath("UserProfile").TrimEnd("\\", "/")
  if ($home -and ($p -ieq $home -or $p.StartsWith($home + [IO.Path]::DirectorySeparatorChar))) {
    throw "refuse to remove path under HOME: $PathToRemove"
  }
}

function Remove-Path([string]$PathToRemove) {
  if (-not $PathToRemove) { return }
  Assert-SafePath $PathToRemove
  if ($DryRun) {
    Write-Host "[dry-run] rm -rf $PathToRemove"
    return
  }
  if (Test-Path -LiteralPath $PathToRemove) {
    Write-Log "rm -rf $PathToRemove"
    Remove-Item -LiteralPath $PathToRemove -Force -Recurse -ErrorAction Stop
  }
}

function Remove-FileGlob([string]$Dir, [string]$Pattern) {
  if (-not $Dir -or -not $Pattern) { return }
  if (-not (Test-Path -LiteralPath $Dir)) { return }
  $items = Get-ChildItem -LiteralPath $Dir -Filter $Pattern -File -ErrorAction SilentlyContinue
  foreach ($item in $items) {
    if ($DryRun) { Write-Host "[dry-run] rm -f $($item.FullName)" }
    else {
      Write-Log "rm -f $($item.FullName)"
      Remove-Item -LiteralPath $item.FullName -Force -ErrorAction Stop
    }
  }
}

$root = Resolve-RootDir
Write-Log "ROOT_DIR=$root"

$defaultMode = (-not $All) -and (-not $Dist) -and (-not $Cache)

if ($All -or $defaultMode) {
  Remove-Path (Join-Path $root "build")
  Remove-Path (Join-Path $root "out")
  Remove-Path (Join-Path $root ".cache")
  Remove-Path (Join-Path $root ".tmp")
  Remove-Path (Join-Path $root ".work")

  Remove-Path (Join-Path $root "toolchain\\build")
  Remove-Path (Join-Path $root "toolchain\\out")
  Remove-Path (Join-Path $root "toolchain\\.cache")
  Remove-Path (Join-Path $root "toolchain\\.tmp")

  Remove-Path (Join-Path $root "CMakeFiles")
  Remove-FileGlob $root "CMakeCache.txt"
  Remove-FileGlob $root "cmake_install.cmake"
  Remove-FileGlob $root "compile_commands.json"

  Remove-FileGlob $root "build.ninja"
  Remove-FileGlob $root ".ninja_log"
  Remove-FileGlob $root ".ninja_deps"
  Remove-FileGlob $root "Makefile"
}

if ($All -or $Cache) {
  Remove-Path (Join-Path $root "build\\.cache")
  Remove-Path (Join-Path $root "build\\.rsp")
  Remove-Path (Join-Path $root "toolchain\\build\\.cache")
  Remove-Path (Join-Path $root "toolchain\\build\\.rsp")
}

if ($All -or $Dist) {
  Remove-Path (Join-Path $root "dist")
  Remove-Path (Join-Path $root "release")
  Remove-Path (Join-Path $root "artifacts")
}

if ($All -or $defaultMode) {
  Remove-FileGlob $root "*.o"
  Remove-FileGlob $root "*.obj"
  Remove-FileGlob $root "*.a"
  Remove-FileGlob $root "*.lib"
  Remove-FileGlob $root "*.so"
  Remove-FileGlob $root "*.dylib"
  Remove-FileGlob $root "*.dll"
  Remove-FileGlob $root "*.d"
  Remove-FileGlob $root "*.pdb"
  Remove-FileGlob $root "*.ilk"
}

Write-Log "clean complete"

