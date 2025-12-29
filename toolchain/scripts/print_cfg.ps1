<#
toolchain/scripts/print_cfg.ps1

Vitte toolchain - print effective configuration inputs (PowerShell).

This script does NOT parse TOML; it prints the files/environment that typically
feed the config loader. It is safe and read-only.

Usage:
  pwsh ./toolchain/scripts/print_cfg.ps1
  pwsh ./toolchain/scripts/print_cfg.ps1 --target linux_x86_64 --profile hardening,lto
  pwsh ./toolchain/scripts/print_cfg.ps1 --env
#>

[CmdletBinding(PositionalBinding = $false)]
param(
  [string]$Target = "",
  [string]$Profile = "",
  [switch]$Env,
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
  $here = Split-Path -Parent $PSCommandPath
  return (Resolve-Path -LiteralPath (Join-Path $here "..\\..")).Path
}

function Print-File([string]$Path) {
  if (Test-Path -LiteralPath $Path) {
    Write-Host ""
    Write-Host "=== $Path ==="
    Get-Content -LiteralPath $Path | ForEach-Object { "  $_" }
  } else {
    Write-Host ""
    Write-Host "=== $Path (missing) ==="
  }
}

$root = Resolve-RootDir
$cfgDir = Join-Path $root "toolchain\\config"

Write-Host "Vitte toolchain configuration"
Write-Host "ROOT_DIR: $root"
Write-Host "CFG_DIR : $cfgDir"

Print-File (Join-Path $cfgDir "default.toml")
Print-File (Join-Path $cfgDir "c_std.toml")
Print-File (Join-Path $cfgDir "cpp_std.toml")
Print-File (Join-Path $cfgDir "warnings.toml")

if ($Target -and $Target.Trim().Length -gt 0) {
  Print-File (Join-Path $cfgDir ("targets\\{0}.toml" -f $Target))
} else {
  Write-Host ""
  Write-Host "(no target specified; default/host will be used)"
}

if ($Profile -and $Profile.Trim().Length -gt 0) {
  $profiles = $Profile.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
  foreach ($p in $profiles) {
    Print-File (Join-Path $cfgDir ("{0}.toml" -f $p))
  }
} else {
  Write-Host ""
  Write-Host "(no profiles specified)"
}

if ($Env) {
  Write-Host ""
  Write-Host "=== environment ==="
  $keys = @(
    "VITTE_LLVM_BIN",
    "VITTE_TOOLCHAIN_ROOT",
    "VITTE_CLANG",
    "VITTE_CLANGXX",
    "VITTE_LLD",
    "VITTE_LLVM_AR",
    "VITTE_LLVM_RANLIB",
    "CC","CXX","LD",
    "SDKROOT",
    "ANDROID_NDK_ROOT","NDK_HOME",
    "WindowsSdkDir","VCToolsInstallDir"
  )
  foreach ($k in $keys) {
    $v = [Environment]::GetEnvironmentVariable($k)
    if ($v -and $v.Trim().Length -gt 0) {
      Write-Host ("  {0}={1}" -f $k, $v)
    }
  }
}

Write-Log "print_cfg complete"

