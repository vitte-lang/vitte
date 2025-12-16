# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\build_all.ps1
<#
Build everything (CMake builds: release/debug/asan, optional tests/bench).

Usage:
  pwsh tools/scripts/build_all.ps1
  pwsh tools/scripts/build_all.ps1 -Mode All
  pwsh tools/scripts/build_all.ps1 -Mode Release -Tests On -Bench Off
  pwsh tools/scripts/build_all.ps1 -Mode All -Jobs 12

Exit codes:
  0 ok
  2 configure/build failed
  3 tests failed
#>

[CmdletBinding()]
param(
  [ValidateSet("All","Release","Debug","Asan","RelWithDebInfo")]
  [string]$Mode = "All",

  [ValidateSet("On","Off")]
  [string]$Tests = "Off",

  [ValidateSet("On","Off")]
  [string]$Bench = "Off",

  [string]$BuildRoot = "build",

  [ValidateRange(1, 256)]
  [int]$Jobs = 0
)

function Fail([string]$Msg, [int]$Code) {
  Write-Error $Msg
  exit $Code
}

function Has([string]$Cmd) {
  return [bool](Get-Command $Cmd -ErrorAction SilentlyContinue)
}

if (-not (Has "cmake")) {
  Fail "cmake not found in PATH." 2
}

$Generator = $null
if (Has "ninja") {
  $Generator = @("-G","Ninja")
}

function Build-One {
  param(
    [string]$Name,
    [string]$BuildDir,
    [string]$Config,
    [string]$Sanitizers,   # ON|OFF
    [string]$EnableTests,  # ON|OFF
    [string]$EnableBench   # ON|OFF
  )

  Write-Host ("[build] {0}: dir={1} type={2} sanitizers={3} tests={4} bench={5}" -f $Name, $BuildDir, $Config, $Sanitizers, $EnableTests, $EnableBench)
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

  $args = @("-S",".","-B",$BuildDir) + $Generator + @(
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DVITTE_ENABLE_SANITIZERS=$Sanitizers",
    "-DVITTE_BUILD_TESTS=$EnableTests",
    "-DVITTE_BUILD_BENCH=$EnableBench"
  )

  & cmake @args | Out-Host
  if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed ($Name)." 2 }

  if ($Jobs -gt 0) {
    & cmake --build $BuildDir --config $Config -- "-j" "$Jobs" | Out-Host
  } else {
    & cmake --build $BuildDir --config $Config | Out-Host
  }

  if ($LASTEXITCODE -ne 0) { Fail "CMake build failed ($Name)." 2 }
}

function Run-CTest([string]$BuildDir) {
  Write-Host ("[test] ctest in {0}" -f $BuildDir)
  Push-Location $BuildDir
  try {
    & ctest --output-on-failure | Out-Host
    if ($LASTEXITCODE -ne 0) { Fail "Tests failed." 3 }
  } finally {
    Pop-Location
  }
}

$EnableTests = if ($Tests -eq "On") { "ON" } else { "OFF" }
$EnableBench = if ($Bench -eq "On") { "ON" } else { "OFF" }

switch ($Mode) {
  "All" {
    Build-One -Name "release" -BuildDir (Join-Path $BuildRoot "release") -Config "Release" -Sanitizers "OFF" -EnableTests $EnableTests -EnableBench $EnableBench
    Build-One -Name "debug"   -BuildDir (Join-Path $BuildRoot "debug")   -Config "Debug"   -Sanitizers "OFF" -EnableTests $EnableTests -EnableBench $EnableBench
  }
  "Release" {
    Build-One -Name "release" -BuildDir (Join-Path $BuildRoot "release") -Config "Release" -Sanitizers "OFF" -EnableTests $EnableTests -EnableBench $EnableBench
  }
  "Debug" {
    Build-One -Name "debug"   -BuildDir (Join-Path $BuildRoot "debug")   -Config "Debug"   -Sanitizers "OFF" -EnableTests $EnableTests -EnableBench $EnableBench
  }
  "Asan" {
    Build-One -Name "asan"    -BuildDir (Join-Path $BuildRoot "asan")    -Config "Debug"   -Sanitizers "ON"  -EnableTests $EnableTests -EnableBench $EnableBench
  }
  "RelWithDebInfo" {
    Build-One -Name "relwithdebinfo" -BuildDir (Join-Path $BuildRoot "relwithdebinfo") -Config "RelWithDebInfo" -Sanitizers "OFF" -EnableTests $EnableTests -EnableBench $EnableBench
  }
}

if ($Tests -eq "On") {
  # Prefer Debug for tests; fallback Release.
  $dbg = Join-Path $BuildRoot "debug"
  $rel = Join-Path $BuildRoot "release"
  if (Test-Path $dbg) { Run-CTest $dbg }
  elseif (Test-Path $rel) { Run-CTest $rel }
  else { Fail "No build directory found for tests." 3 }
}

Write-Host "[build] done"
exit 0
