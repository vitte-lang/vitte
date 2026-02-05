# ============================================================
# vitte — target definition: windows-x86_64
# Location: toolchain/scripts/targets/windows-x86_64.ps1
# ============================================================

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ----------------------------
# Target identity
# ----------------------------
$env:VITTE_TARGET_NAME   = "windows-x86_64"
$env:VITTE_TARGET_OS     = "windows"
$env:VITTE_TARGET_ARCH   = "x86_64"
$env:VITTE_TARGET_TRIPLE = "x86_64-pc-windows-msvc"

# ----------------------------
# Toolchain selection
# ----------------------------
# Default to LLVM/Clang if present; fall back to MSVC
function Has-Cmd($n) { Get-Command $n -ErrorAction SilentlyContinue | Out-Null; return $? }

if (-not $env:CC) {
  if (Has-Cmd "clang") { $env:CC = "clang" }
  elseif (Has-Cmd "cl") { $env:CC = "cl" }
  else { throw "No suitable C compiler found (clang or cl)" }
}

if (-not $env:CXX) {
  if (Has-Cmd "clang++") { $env:CXX = "clang++" }
  elseif (Has-Cmd "cl") { $env:CXX = "cl" }
  else { throw "No suitable C++ compiler found (clang++ or cl)" }
}

if (-not $env:AR)  { $env:AR  = "llvm-ar" }
if (-not $env:LD)  { $env:LD  = "lld-link" }
if (-not $env:NM)  { $env:NM  = "llvm-nm" }
if (-not $env:STRIP) { $env:STRIP = "llvm-strip" }

# ----------------------------
# Windows SDK / MSVC env
# ----------------------------
# If using MSVC, ensure vcvars is loaded by the user beforehand.
# For Clang, Windows SDK is usually auto-detected.
if ($env:WINDOWS_SDK) {
  $env:SDKROOT = $env:WINDOWS_SDK
}

# ----------------------------
# Flags
# ----------------------------
$commonCFlags = @(
  "--target=$($env:VITTE_TARGET_TRIPLE)",
  "-fno-omit-frame-pointer",
  "-Wall", "-Wextra"
)

$commonLDFlags = @(
  "/machine:x64"
)

# Hardened defaults (best-effort)
$hardenCFlags = @(
  "-fstack-protector-strong"
)

# Append flags (do not overwrite)
$env:CFLAGS   = "$($env:CFLAGS) $($commonCFlags -join ' ') $($hardenCFlags -join ' ')".Trim()
$env:CXXFLAGS = "$($env:CXXFLAGS) $($commonCFlags -join ' ') $($hardenCFlags -join ' ')".Trim()
$env:LDFLAGS  = "$($env:LDFLAGS) $($commonLDFlags -join ' ')".Trim()

# ----------------------------
# Libraries / paths
# ----------------------------
# Optional sysroot/include/lib injection
if ($env:SDKROOT) {
  $env:CPATH        = "$($env:CPATH);$env:SDKROOT\Include"
  $env:LIBRARY_PATH = "$($env:LIBRARY_PATH);$env:SDKROOT\Lib"
}

# ----------------------------
# Vitte-specific knobs
# ----------------------------
$env:VITTE_ABI       = "msvc"
$env:VITTE_ENDIAN   = "little"
$env:VITTE_WORD_SIZE= "64"
$env:VITTE_PTR_SIZE = "8"

# ----------------------------
# Hooks (optional)
# ----------------------------
function vitte_target_pre_build { }
function vitte_target_post_build { }

# ----------------------------
# Summary
# ----------------------------
Write-Host "[target] $($env:VITTE_TARGET_NAME)"
Write-Host "  triple=$($env:VITTE_TARGET_TRIPLE)"
Write-Host "  cc=$($env:CC) cxx=$($env:CXX) ld=$($env:LD)"
if ($env:SDKROOT) { Write-Host "  sdk=$($env:SDKROOT)" }