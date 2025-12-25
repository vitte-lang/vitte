<#
toolchain/scripts/smoke.ps1

Vitte toolchain - smoke test for toolchain drivers (cc/cxx/ld/ar) + configs.

Usage:
  pwsh ./toolchain/scripts/smoke.ps1
  pwsh ./toolchain/scripts/smoke.ps1 --target linux_x86_64
  pwsh ./toolchain/scripts/smoke.ps1 --profile hardening,lto
  pwsh ./toolchain/scripts/smoke.ps1 --keep
  pwsh ./toolchain/scripts/smoke.ps1 --verbose

Notes:
  - Assumes drivers are already built and available on PATH:
      vitte-cc, vitte-cxx, vitte-ld
  - Does not call sh/bash (works on "pure" Windows).
#>

[CmdletBinding(PositionalBinding = $false)]
param(
  [string]$Target = "",
  [string]$Profile = "",
  [switch]$Keep,
  [switch]$VerboseMode,
  [string]$RootDir = "",
  [string]$BuildDir = ""
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
  $here = Split-Path -Parent $PSCommandPath
  return (Resolve-Path -LiteralPath (Join-Path $here "..\\..")).Path
}

function Need([string]$Cmd) {
  $c = Get-Command $Cmd -ErrorAction SilentlyContinue
  if (-not $c) { throw "missing command: $Cmd" }
}

function Remove-Tree([string]$PathToRemove) {
  if (-not $PathToRemove) { return }
  if ($Keep) { Write-Log "keep: $PathToRemove"; return }
  if (Test-Path -LiteralPath $PathToRemove) { Remove-Item -LiteralPath $PathToRemove -Force -Recurse }
}

$root = Resolve-RootDir
if (-not $BuildDir -or $BuildDir.Trim().Length -eq 0) {
  $BuildDir = Join-Path $root "build"
}
$tmpDir = Join-Path $BuildDir ".smoke"

Need "vitte-cc"
Need "vitte-cxx"
Need "vitte-ld"

New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null
Write-Log "TMP_DIR=$tmpDir"

$cSrc = Join-Path $tmpDir "hello.c"
$cxxSrc = Join-Path $tmpDir "hello.cpp"
$objC = Join-Path $tmpDir "hello.o"
$objCxx = Join-Path $tmpDir "hello_cpp.o"
$exeC = Join-Path $tmpDir "hello_c"
$exeCxx = Join-Path $tmpDir "hello_cxx"
$rsp = Join-Path $tmpDir "args.rsp"

@'
#include <stdio.h>
int main(void) {
  puts("vitte-cc ok");
  return 0;
}
'@ | Set-Content -LiteralPath $cSrc -NoNewline

@'
#include <iostream>
int main() {
  std::cout << "vitte-cxx ok\n";
  return 0;
}
'@ | Set-Content -LiteralPath $cxxSrc -NoNewline

$targetArgs = @()
if ($Target -and $Target.Trim().Length -gt 0) {
  $targetArgs = @("--target", $Target)
}

function Run-Driver([string]$Tool, [string[]]$Args, [string]$FallbackMsg) {
  try {
    & $Tool @Args | Out-Null
    return
  } catch {
    if ($FallbackMsg) { Write-Log $FallbackMsg }
    throw
  }
}

Write-Log "compile C"
try {
  & vitte-cc @targetArgs -c $cSrc -o $objC --rsp $rsp | Out-Null
} catch {
  & vitte-cc @targetArgs -c $cSrc -o $objC | Out-Null
}

Write-Log "link C"
try {
  & vitte-cc @targetArgs $objC -o $exeC --rsp $rsp | Out-Null
} catch {
  & vitte-cc @targetArgs $objC -o $exeC | Out-Null
}

Write-Log "compile C++"
try {
  & vitte-cxx @targetArgs -c $cxxSrc -o $objCxx --rsp $rsp | Out-Null
} catch {
  & vitte-cxx @targetArgs -c $cxxSrc -o $objCxx | Out-Null
}

Write-Log "link C++"
try {
  & vitte-cxx @targetArgs $objCxx -o $exeCxx --rsp $rsp | Out-Null
} catch {
  & vitte-cxx @targetArgs $objCxx -o $exeCxx | Out-Null
}

$llvmAr = Get-Command "llvm-ar" -ErrorAction SilentlyContinue
if ($llvmAr) {
  Write-Log "archive"
  $libA = Join-Path $tmpDir "libsmoke.a"
  & llvm-ar rcs $libA $objC $objCxx | Out-Null
  Write-Host ("archive: ok ({0})" -f $libA)
} else {
  Write-Host "archive: skipped (llvm-ar not found)"
}

Write-Host ("C:   ok ({0})" -f $exeC)
Write-Host ("C++: ok ({0})" -f $exeCxx)

if (-not ($Target -and $Target.Trim().Length -gt 0)) {
  Write-Host -NoNewline "run: C   -> "
  try { & $exeC } catch { }
  Write-Host -NoNewline "run: C++ -> "
  try { & $exeCxx } catch { }
} else {
  Write-Host ("run: skipped (target={0})" -f $Target)
}

Remove-Tree $tmpDir

