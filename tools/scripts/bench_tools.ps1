# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\bench_tools.ps1
<#
Bench harness for Vitte tools (CMake builds, smoke runs, timing).

Usage:
  pwsh tools/scripts/bench_tools.ps1
  pwsh tools/scripts/bench_tools.ps1 -Iters 10 -Mode Release -CmdArgs "--help"
  pwsh tools/scripts/bench_tools.ps1 -Mode Debug -CmdArgs "compile examples/hello.vitte"

Exit codes:
  0 ok
  2 build failed
  3 run failed
#>

[CmdletBinding()]
param(
  [ValidateRange(1, 100000)]
  [int]$Iters = 5,

  [ValidateSet("Release","Debug","Asan")]
  [string]$Mode = "Release",

  [string]$CmdArgs = "--help",

  [string]$BuildRoot = "build",

  [string]$BinName = "vittec"
)

function Fail([string]$Msg, [int]$Code) {
  Write-Error $Msg
  exit $Code
}

function Get-BuildDir([string]$M) {
  switch ($M) {
    "Release" { return Join-Path $BuildRoot "release" }
    "Debug"   { return Join-Path $BuildRoot "debug" }
    "Asan"    { return Join-Path $BuildRoot "asan" }
  }
}

function Configure-Build([string]$M) {
  $bdir = Get-BuildDir $M
  New-Item -ItemType Directory -Force -Path $bdir | Out-Null

  if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Fail "cmake not found in PATH." 2
  }

  switch ($M) {
    "Release" {
      & cmake -S . -B $bdir -DCMAKE_BUILD_TYPE=Release -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=OFF | Out-Host
      if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }
      & cmake --build $bdir --config Release | Out-Host
      if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }
    }
    "Debug" {
      & cmake -S . -B $bdir -DCMAKE_BUILD_TYPE=Debug -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=OFF | Out-Host
      if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }
      & cmake --build $bdir --config Debug | Out-Host
      if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }
    }
    "Asan" {
      & cmake -S . -B $bdir -DCMAKE_BUILD_TYPE=Debug -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=OFF -DVITTE_ENABLE_SANITIZERS=ON | Out-Host
      if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }
      & cmake --build $bdir --config Debug | Out-Host
      if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }
    }
  }
}

function Resolve-Bin([string]$M) {
  $bdir = Get-BuildDir $M
  $exe1 = Join-Path $bdir $BinName
  $exe2 = Join-Path $bdir ($BinName + ".exe")
  if (Test-Path $exe1) { return $exe1 }
  if (Test-Path $exe2) { return $exe2 }
  Fail "Binary not found in $bdir ($BinName or $BinName.exe)." 3
  return $null
}

function Parse-CmdArgs([string]$s) {
  # Split like a shell would for simple cases; users can pass quoted args as a single string.
  # If you need strict parsing, pass an array and adjust this script.
  return [System.Management.Automation.PSParser]::Tokenize($s, [ref]$null) |
    Where-Object { $_.Type -eq 'CommandArgument' -or $_.Type -eq 'String' } |
    ForEach-Object { $_.Content }
}

Write-Host ("[bench] mode={0} iters={1} cmd={2}" -f $Mode, $Iters, $CmdArgs)

Configure-Build $Mode
$bin = Resolve-Bin $Mode
Write-Host ("[bench] binary={0}" -f $bin)

# Warmup
Write-Host "[bench] warmup..."
$warmArgs = Parse-CmdArgs $CmdArgs
& $bin @warmArgs *> $null
if ($LASTEXITCODE -ne 0) { Fail "Warmup failed (non-zero exit)." 3 }

Write-Host ("[bench] running {0} iterations..." -f $Iters)

$times = New-Object System.Collections.Generic.List[int]
for ($i = 1; $i -le $Iters; $i++) {
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  & $bin @warmArgs *> $null
  $sw.Stop()
  if ($LASTEXITCODE -ne 0) { Fail ("Iteration {0} failed." -f $i) 3 }
  $ms = [int]$sw.ElapsedMilliseconds
  $times.Add($ms) | Out-Null
  Write-Host ("  iter={0} dur_ms={1}" -f $i, $ms)
}

$sorted = $times | Sort-Object
$n = $sorted.Count
$min = $sorted[0]
$max = $sorted[$n-1]
$mean = [int](($sorted | Measure-Object -Average).Average)
$median = if ($n % 2 -eq 1) { [int]$sorted[($n-1)/2] } else { [int](($sorted[$n/2 - 1] + $sorted[$n/2]) / 2) }

function Percentile([int[]]$arr, [int]$p) {
  $n = $arr.Count
  if ($n -eq 0) { return 0 }
  $k = ($n - 1) * ($p / 100.0)
  $f = [math]::Floor($k)
  $c = [math]::Ceiling($k)
  if ($f -eq $c) { return [int]$arr[[int]$k] }
  return [int]($arr[$f] + ($arr[$c] - $arr[$f]) * ($k - $f))
}

$p95 = Percentile -arr $sorted -p 95

Write-Host "[bench] stats:"
Write-Host ("  n={0}" -f $n)
Write-Host ("  min_ms={0}" -f $min)
Write-Host ("  mean_ms={0}" -f $mean)
Write-Host ("  median_ms={0}" -f $median)
Write-Host ("  p95_ms={0}" -f $p95)
Write-Host ("  max_ms={0}" -f $max)

Write-Host "[bench] done"
exit 0
