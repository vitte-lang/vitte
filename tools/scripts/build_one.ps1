# tools/scripts/build_once.ps1
<#
One-shot build for a single configuration.

Usage:
  pwsh tools/scripts/build_once.ps1 -Mode Release
  pwsh tools/scripts/build_once.ps1 -Mode Debug -Tests On
  pwsh tools/scripts/build_once.ps1 -Mode Asan -Jobs 12
  pwsh tools/scripts/build_once.ps1 -Mode RelWithDebInfo -Bench On

Exit codes:
  0 ok
  2 configure/build failed
  3 tests failed
#>

[CmdletBinding()]
param(
  [ValidateSet("Release","Debug","Asan","RelWithDebInfo")]
  [string]$Mode = "Release",

  [ValidateSet("On","Off")]
  [string]$Tests = "Off",

  [ValidateSet("On","Off")]
  [string]$Bench = "Off",

  [string]$BuildRoot = "build",

  [ValidateRange(1,256)]
  [int]$Jobs = 0
)

function Fail([string]$Msg, [int]$Code) {
  Write-Error $Msg
  exit $Code
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  Fail "cmake not found in PATH." 2
}

$Gen = @()
if (Get-Command ninja -ErrorAction SilentlyContinue) {
  $Gen = @("-G","Ninja")
}

function ToOnOff([string]$v) { if ($v -eq "On") { "ON" } else { "OFF" } }

switch ($Mode) {
  "Release" {
    $Cfg = "Release"; $Bdir = Join-Path $BuildRoot "release"; $Asan = "OFF"
  }
  "Debug" {
    $Cfg = "Debug"; $Bdir = Join-Path $BuildRoot "debug"; $Asan = "OFF"
  }
  "Asan" {
    $Cfg = "Debug"; $Bdir = Join-Path $BuildRoot "asan"; $Asan = "ON"
  }
  "RelWithDebInfo" {
    $Cfg = "RelWithDebInfo"; $Bdir = Join-Path $BuildRoot "relwithdebinfo"; $Asan = "OFF"
  }
}

Write-Host ("[build_once] mode={0} dir={1} cfg={2} tests={3} bench={4}" -f $Mode, $Bdir, $Cfg, $Tests, $Bench)
New-Item -ItemType Directory -Force -Path $Bdir | Out-Null

$args = @("-S",".","-B",$Bdir) + $Gen + @(
  "-DCMAKE_BUILD_TYPE=$Cfg",
  "-DVITTE_ENABLE_SANITIZERS=$Asan",
  "-DVITTE_BUILD_TESTS=$(ToOnOff $Tests)",
  "-DVITTE_BUILD_BENCH=$(ToOnOff $Bench)"
)

& cmake @args | Out-Host
if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }

if ($Jobs -gt 0) {
  & cmake --build $Bdir --config $Cfg -- "-j" "$Jobs" | Out-Host
} else {
  & cmake --build $Bdir --config $Cfg | Out-Host
}
if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }

if ($Tests -eq "On") {
  Push-Location $Bdir
  try {
    & ctest --output-on-failure | Out-Host
    if ($LASTEXITCODE -ne 0) { Fail "Tests failed." 3 }
  } finally {
    Pop-Location
  }
}

Write-Host "[build_once] done"
exit 0
