Param(
  [string]$CC = "",
  [string]$CFlags = "",
  [string]$LDFlags = "",
  [string]$OutDir = "fuzz/out",
  [string]$Only = "",
  [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Die([string]$msg) {
  throw "error: $msg"
}

if ([string]::IsNullOrWhiteSpace($env:ASAN_OPTIONS)) {
  $env:ASAN_OPTIONS = "abort_on_error=1:detect_leaks=0:allocator_may_return_null=1:handle_segv=0:handle_sigbus=0:handle_abort=0:symbolize=1:fast_unwind_on_malloc=0"
}
if ([string]::IsNullOrWhiteSpace($env:UBSAN_OPTIONS)) {
  $env:UBSAN_OPTIONS = "halt_on_error=1:abort_on_error=1:print_stacktrace=1:symbolize=1"
}

function Quote([string]$s) {
  if ($s -match '[\s"`]') { return '"' + ($s -replace '"', '""') + '"' }
  return $s
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = (Resolve-Path (Join-Path $ScriptDir "../..")).Path
$FuzzDir = Join-Path $RootDir "fuzz"
$TargetDir = Join-Path $FuzzDir "targets"
$OutDirAbs = (Join-Path $RootDir $OutDir)

if (!(Test-Path $TargetDir)) { Die "target dir missing: $TargetDir" }
New-Item -ItemType Directory -Force -Path $OutDirAbs | Out-Null
$OutDirAbs = (Resolve-Path $OutDirAbs).Path

$CCCmd = $env:FUZZ_CC
if ([string]::IsNullOrWhiteSpace($CCCmd)) { $CCCmd = $env:CC }
if ([string]::IsNullOrWhiteSpace($CCCmd)) { $CCCmd = $CC }

if ([string]::IsNullOrWhiteSpace($CCCmd)) {
  if (Get-Command clang -ErrorAction SilentlyContinue) { $CCCmd = "clang" }
  elseif (Get-Command clang-cl -ErrorAction SilentlyContinue) { $CCCmd = "clang-cl" }
  elseif (Get-Command cl -ErrorAction SilentlyContinue) { $CCCmd = "cl" }
  else { Die "no compiler found (set --cc or FUZZ_CC/CC)" }
}

$CFlagsEnv = $env:FUZZ_CFLAGS
if ([string]::IsNullOrWhiteSpace($CFlagsEnv)) { $CFlagsEnv = $env:VITTE_FUZZ_CFLAGS }
$LDFlagsEnv = $env:FUZZ_LDFLAGS
if ([string]::IsNullOrWhiteSpace($LDFlagsEnv)) { $LDFlagsEnv = $env:VITTE_FUZZ_LDFLAGS }

$sanitize = $env:VITTE_FUZZ_SANITIZE
if ([string]::IsNullOrWhiteSpace($sanitize)) { $sanitize = "1" }
$sanKind = $env:VITTE_FUZZ_SANITIZER
if ([string]::IsNullOrWhiteSpace($sanKind)) { $sanKind = "asan_ubsan" } # asan_ubsan|asan|ubsan|msan|none

$sanCFlags = ""
$sanLDFlags = ""
if ($sanitize -eq "1") {
  switch ($sanKind) {
    "none" { $sanCFlags = ""; $sanLDFlags = "" }
    "asan_ubsan" { $sanCFlags = "-fsanitize=address,undefined"; $sanLDFlags = "-fsanitize=address,undefined" }
    "asan" { $sanCFlags = "-fsanitize=address"; $sanLDFlags = "-fsanitize=address" }
    "ubsan" { $sanCFlags = "-fsanitize=undefined"; $sanLDFlags = "-fsanitize=undefined" }
    "msan" { $sanCFlags = "-fsanitize=memory"; $sanLDFlags = "-fsanitize=memory" }
    default { Die "unknown VITTE_FUZZ_SANITIZER=$sanKind (expected asan_ubsan|asan|ubsan|msan|none)" }
  }
}

$CFlagsDefault = "-std=c17 -g -O1 -fno-omit-frame-pointer -Wall -Wextra -Wpedantic $sanCFlags"
$CFlagsUse = $CFlags
if ([string]::IsNullOrWhiteSpace($CFlagsUse)) { $CFlagsUse = $CFlagsEnv }
if ([string]::IsNullOrWhiteSpace($CFlagsUse)) { $CFlagsUse = $CFlagsDefault }

$LDFlagsUse = $LDFlags
if ([string]::IsNullOrWhiteSpace($LDFlagsUse)) { $LDFlagsUse = $LDFlagsEnv }
if ([string]::IsNullOrWhiteSpace($LDFlagsUse)) { $LDFlagsUse = $sanLDFlags }

function Add-Inc([System.Collections.Generic.List[string]]$incs, [string]$dir) {
  if (Test-Path $dir) { [void]$incs.Add("-I" + (Quote $dir)) }
}

$incs = New-Object 'System.Collections.Generic.List[string]'
Add-Inc $incs (Join-Path $FuzzDir "include")
Add-Inc $incs (Join-Path $RootDir "include")
Add-Inc $incs (Join-Path $RootDir "src")
Add-Inc $incs (Join-Path $RootDir "compiler/include")

$linkVitte = $env:VITTE_FUZZ_LINK_VITTE
if ([string]::IsNullOrWhiteSpace($linkVitte)) { $linkVitte = "1" }
$vitteSources = @()
if ($linkVitte -eq "1" -and (Test-Path (Join-Path $RootDir "src/vitte"))) {
  $vitteSources = Get-ChildItem -File (Join-Path $RootDir "src/vitte") -Filter *.c | ForEach-Object { $_.FullName }
}

$linkVittecFront = $env:VITTE_FUZZ_LINK_VITTEC_FRONT
if ([string]::IsNullOrWhiteSpace($linkVittecFront)) { $linkVittecFront = "1" }
$vittecFrontSources = @()
if ($linkVittecFront -eq "1") {
  $cand = @(
    (Join-Path $RootDir "compiler/src/front/vittec_lexer.c"),
    (Join-Path $RootDir "compiler/src/front/parser.c")
  )
  foreach ($c in $cand) {
    if (Test-Path $c) { $vittecFrontSources += (Resolve-Path $c).Path }
  }
}

$linkAsm = $env:VITTE_FUZZ_LINK_ASM
if ([string]::IsNullOrWhiteSpace($linkAsm)) { $linkAsm = "1" }
$asmLib = ""
if ($linkAsm -eq "1") {
  $cmakeBuildDir = $env:VITTE_FUZZ_CMAKE_BUILD_DIR
  if ([string]::IsNullOrWhiteSpace($cmakeBuildDir)) { $cmakeBuildDir = (Join-Path $RootDir "build-fuzz") }
  $cmakeGen = $env:VITTE_FUZZ_CMAKE_GENERATOR
  $cmakeConfig = $env:VITTE_FUZZ_CMAKE_CONFIG
  if ([string]::IsNullOrWhiteSpace($cmakeConfig)) { $cmakeConfig = "" }

  $cfgArgs = @("-S", $RootDir, "-B", $cmakeBuildDir, "-DVITTE_ENABLE_ASM_RUNTIME=ON", "-DVITTE_ENABLE_RUNTIME_TESTS=OFF")
  if (-not [string]::IsNullOrWhiteSpace($cmakeGen)) { $cfgArgs = @("-G", $cmakeGen) + $cfgArgs }

  if ($DryRun) {
    Write-Host ("  cmake " + ($cfgArgs | ForEach-Object { Quote $_ } | Join-String " "))
    if ($cmakeConfig -ne "") {
      Write-Host ("  cmake --build " + (Quote $cmakeBuildDir) + " --config " + (Quote $cmakeConfig) + " --target vitte_asm_runtime")
    } else {
      Write-Host ("  cmake --build " + (Quote $cmakeBuildDir) + " --target vitte_asm_runtime")
    }
  } else {
    & cmake @cfgArgs | Out-Null
    if ($LASTEXITCODE -ne 0) { Die "cmake configure failed" }
    if ($cmakeConfig -ne "") { & cmake --build $cmakeBuildDir --config $cmakeConfig --target vitte_asm_runtime | Out-Null }
    else { & cmake --build $cmakeBuildDir --target vitte_asm_runtime | Out-Null }
    if ($LASTEXITCODE -ne 0) { Die "cmake build failed (vitte_asm_runtime)" }
  }

  $candidates = @()
  if (Test-Path $cmakeBuildDir) {
    $candidates += Get-ChildItem -Recurse -File $cmakeBuildDir -Filter "vitte_asm_runtime.lib" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
    $candidates += Get-ChildItem -Recurse -File $cmakeBuildDir -Filter "libvitte_asm_runtime.a" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
  }
  if ($candidates.Count -gt 0) {
    if ($cmakeConfig -ne "") {
      $preferred = $candidates | Where-Object { $_ -match ([regex]::Escape([IO.Path]::DirectorySeparatorChar + $cmakeConfig + [IO.Path]::DirectorySeparatorChar)) }
      if ($preferred.Count -gt 0) { $asmLib = $preferred[0] } else { $asmLib = $candidates[0] }
    } else {
      $asmLib = $candidates[0]
    }
  }
  else { Die "could not locate built vitte_asm_runtime library under $cmakeBuildDir" }
}

$targetsFound = 0
$targetsBuilt = 0

Get-ChildItem -File $TargetDir -Filter *.c | ForEach-Object {
  $src = $_.FullName
  $base = $_.BaseName

  if ($Only -ne "" -and $base -ne $Only) { return }

  if (!(Select-String -Path $src -Pattern "FUZZ_DRIVER_TARGET" -Quiet)) { return }

  $targetsFound++

  $outBin = Join-Path $OutDirAbs ($base + ".exe")

  $allSources = @($src) + $vitteSources + $vittecFrontSources

  $cmd = @()
  $cmd += $CCCmd
  $cmd += $CFlagsUse
  $cmd += $incs.ToArray()
  $cmd += "-DFUZZ_DRIVER_STANDALONE_MAIN=1"
  $cmd += "-DFUZZ_DISABLE_SANITIZER_TRACE=1"
  $cmd += ($allSources | ForEach-Object { Quote $_ })
  if ($asmLib -ne "") { $cmd += (Quote $asmLib) }
  if ($LDFlagsUse -ne "") { $cmd += $LDFlagsUse }
  $cmd += "-o"
  $cmd += (Quote $outBin)

  Write-Host "[fuzz-build] $base -> $outBin"
  if ($DryRun) {
    Write-Host ("  " + ($cmd -join " "))
    $targetsBuilt++
    return
  }

  & $CCCmd @($cmd[1..($cmd.Length - 1)]) | Out-Null
  if ($LASTEXITCODE -ne 0) { Die "build failed for $base" }
  $targetsBuilt++
}

if ($Only -ne "" -and $targetsFound -eq 0) { Die "no matching targets built for --only=$Only" }

if ($targetsBuilt -eq 0 -and -not $DryRun) {
  Write-Host "[fuzz-build] no harness sources define FUZZ_DRIVER_TARGET yet (placeholders?)"
} else {
  Write-Host "[fuzz-build] built $targetsBuilt harness(es)"
}
