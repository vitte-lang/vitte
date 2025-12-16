# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\coverage.ps1
<#
Coverage runner for Vitte (C/C99) using LLVM (clang/llvm-profdata/llvm-cov).

What it does:
  - configures a dedicated coverage build dir
  - builds with coverage instrumentation
  - runs ctest
  - merges profraw -> profdata
  - generates:
      dist/coverage/coverage.txt
      dist/coverage/coverage.json
      dist/coverage/index.html (HTML report dir)

Requirements:
  - clang
  - llvm-profdata
  - llvm-cov
  - cmake, ctest

Usage:
  pwsh tools/scripts/coverage.ps1
  pwsh tools/scripts/coverage.ps1 -BuildRoot build -OutRoot dist/coverage -Jobs 12
  pwsh tools/scripts/coverage.ps1 -BuildDir build/coverage -Clean

Exit codes:
  0 ok
  2 missing tooling / configure/build failure
  3 tests failed
  4 coverage generation failed
#>

[CmdletBinding()]
param(
  [string]$BuildDir = "build/coverage",
  [string]$OutRoot  = "dist/coverage",
  [ValidateRange(0,256)]
  [int]$Jobs = 0,
  [switch]$Clean
)

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }

function Ensure-Dir([string]$Path) {
  if (-not (Test-Path $Path)) { New-Item -ItemType Directory -Force -Path $Path | Out-Null }
}

function Has([string]$Cmd) { return [bool](Get-Command $Cmd -ErrorAction SilentlyContinue) }

# Tooling checks
foreach ($t in @("cmake","ctest","clang","llvm-profdata","llvm-cov")) {
  if (-not (Has $t)) { Fail "Missing tool in PATH: $t" 2 }
}

if ($Clean -and (Test-Path $BuildDir)) {
  Remove-Item -Recurse -Force -Path $BuildDir -ErrorAction SilentlyContinue
}

Ensure-Dir $BuildDir
Ensure-Dir $OutRoot

# Make paths safe
$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
$OutRoot  = (Resolve-Path -LiteralPath $OutRoot).Path

$env:LLVM_PROFILE_FILE = Join-Path $BuildDir "profraw/%p-%m.profraw"
Ensure-Dir (Join-Path $BuildDir "profraw")

Write-Host ("[cov] build_dir={0}" -f $BuildDir)
Write-Host ("[cov] out_root ={0}" -f $OutRoot)
Write-Host ("[cov] LLVM_PROFILE_FILE={0}" -f $env:LLVM_PROFILE_FILE)

# Configure with coverage flags.
# NOTE: use clang explicitly to ensure llvm-cov compatibility.
$covCFlags = "-O0 -g -fprofile-instr-generate -fcoverage-mapping"
$covLdFlags = "-fprofile-instr-generate -fcoverage-mapping"

$cfgArgs = @(
  "-S",".",
  "-B",$BuildDir,
  "-DCMAKE_BUILD_TYPE=Debug",
  "-DCMAKE_C_COMPILER=clang",
  "-DVITTE_BUILD_TESTS=ON",
  "-DVITTE_BUILD_BENCH=OFF",
  "-DVITTE_ENABLE_SANITIZERS=OFF",
  "-DCMAKE_C_FLAGS=$covCFlags",
  "-DCMAKE_EXE_LINKER_FLAGS=$covLdFlags",
  "-DCMAKE_SHARED_LINKER_FLAGS=$covLdFlags"
)

& cmake @cfgArgs | Out-Host
if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }

# Build
if ($Jobs -gt 0) {
  & cmake --build $BuildDir --config Debug -- "-j" "$Jobs" | Out-Host
} else {
  & cmake --build $BuildDir --config Debug | Out-Host
}
if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }

# Run tests
Push-Location $BuildDir
try {
  & ctest --output-on-failure | Out-Host
  if ($LASTEXITCODE -ne 0) { Fail "Tests failed." 3 }
} finally {
  Pop-Location
}

# Merge profraw -> profdata
$profdata = Join-Path $OutRoot "coverage.profdata"
$profrawDir = Join-Path $BuildDir "profraw"

$rawFiles = Get-ChildItem -Path $profrawDir -Recurse -Filter "*.profraw" -ErrorAction SilentlyContinue
if (-not $rawFiles -or $rawFiles.Count -eq 0) {
  Fail "No .profraw files found. Ensure tests executed instrumented binaries." 4
}

$rawPaths = $rawFiles | ForEach-Object { $_.FullName }
& llvm-profdata merge -sparse @rawPaths -o $profdata | Out-Host
if ($LASTEXITCODE -ne 0) { Fail "llvm-profdata merge failed." 4 }

# Try to locate the main binary and any test executables for better coverage.
# We include everything in the build dir that looks executable.
$executables = @()

# vittec (primary)
foreach ($cand in @("vittec.exe","vittec")) {
  $p = Join-Path $BuildDir $cand
  if (Test-Path $p) { $executables += $p }
}

# Any other executables in build dir (tests) - best effort
Get-ChildItem -Path $BuildDir -File -Recurse -ErrorAction SilentlyContinue |
  Where-Object {
    ($_.Extension -eq ".exe") -or ($IsLinux -or $IsMacOS) # no reliable extension on unix
  } | ForEach-Object {
    # Filter out obvious non-binaries
    if ($_.Name -match '\.(o|obj|a|lib|dll|so|dylib|pdb|profdata|profraw|txt|json|html|cmake)$') { return }
    # Avoid CMake internal tools
    if ($_.FullName -match '[\\/]CMakeFiles[\\/]') { return }
    # Avoid huge noise; keep typical test executables
    $executables += $_.FullName
  }

$executables = $executables | Sort-Object -Unique
if ($executables.Count -eq 0) {
  Fail "No executables found in build dir for llvm-cov." 4
}

# Reports
$txt = Join-Path $OutRoot "coverage.txt"
$json = Join-Path $OutRoot "coverage.json"
$htmlDir = Join-Path $OutRoot "html"

if (Test-Path $htmlDir) { Remove-Item -Recurse -Force -Path $htmlDir -ErrorAction SilentlyContinue }
Ensure-Dir $htmlDir

# llvm-cov show (HTML)
# Use --ignore-filename-regex to reduce noise (deps/build system).
$ignore = "(.*/_deps/.*|.*/CMakeFiles/.*|.*/tests/.*\.c)"  # keep adjustable
& llvm-cov show @executables `
  --instr-profile $profdata `
  --format html `
  --output-dir $htmlDir `
  --ignore-filename-regex $ignore `
  --show-line-counts-or-regions `
  --show-instantiations `
  | Out-Host
if ($LASTEXITCODE -ne 0) { Fail "llvm-cov show (html) failed." 4 }

# llvm-cov report (text)
& llvm-cov report @executables `
  --instr-profile $profdata `
  --ignore-filename-regex $ignore `
  | Tee-Object -FilePath $txt | Out-Host
if ($LASTEXITCODE -ne 0) { Fail "llvm-cov report failed." 4 }

# llvm-cov export (json)
& llvm-cov export @executables `
  --instr-profile $profdata `
  --ignore-filename-regex $ignore `
  | Out-File -FilePath $json -Encoding utf8
if ($LASTEXITCODE -ne 0) { Fail "llvm-cov export failed." 4 }

Write-Host "[cov] outputs:"
Write-Host ("  {0}" -f $txt)
Write-Host ("  {0}" -f $json)
Write-Host ("  {0}" -f $htmlDir)

exit 0
