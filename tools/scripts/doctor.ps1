# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\doctor.ps1
<#
doctor.ps1 (full)

Workspace diagnostics for Vitte (CMake/C toolchain + repo layout sanity).

What it checks (best-effort):
  - host OS/arch + basic env
  - required tools presence + versions (cmake, make/ninja, compiler, debugger, git)
  - LLVM coverage tools (optional)
  - repo layout: key files + directories
  - build dirs existence and basic CMake cache sanity
  - Muffin files presence (muffin.muf, muffin.lock)
  - optional smoke: run vittec --version (if built)

Usage:
  pwsh tools/scripts/doctor.ps1
  pwsh tools/scripts/doctor.ps1 -Verbose
  pwsh tools/scripts/doctor.ps1 -BuildDir build/debug -Smoke
  pwsh tools/scripts/doctor.ps1 -FailOnWarn

Exit codes:
  0 ok (no errors, warnings allowed unless -FailOnWarn)
  1 warnings (only when -FailOnWarn)
  2 errors
#>

[CmdletBinding()]
param(
  [string]$Workspace = ".",
  [string]$BuildDir = "build/debug",
  [switch]$Smoke,
  [switch]$FailOnWarn
)

function Has([string]$Cmd) { [bool](Get-Command $Cmd -ErrorAction SilentlyContinue) }
function CmdVer([string]$Cmd, [string[]]$Args) {
  try { (& $Cmd @Args 2>$null | Select-Object -First 1) } catch { $null }
}
function Ensure-Abs([string]$p) { (Resolve-Path -LiteralPath $p -ErrorAction Stop).Path }

$errors = New-Object System.Collections.Generic.List[string]
$warns  = New-Object System.Collections.Generic.List[string]
$infos  = New-Object System.Collections.Generic.List[string]

function Info($m){ $infos.Add($m) | Out-Null; Write-Host ("[info] {0}" -f $m) }
function Warn($m){ $warns.Add($m) | Out-Null; Write-Warning $m }
function Err($m){ $errors.Add($m) | Out-Null; Write-Error $m }

function Header($t){
  Write-Host ""
  Write-Host ("=" * 78)
  Write-Host $t
  Write-Host ("=" * 78)
}

function Check-Tool([string]$name, [string]$cmd, [string[]]$verArgs = @("--version"), [switch]$Required) {
  if (-not (Has $cmd)) {
    if ($Required) { Err "Missing tool: $name ($cmd)" } else { Warn "Missing tool: $name ($cmd)" }
    return
  }
  $v = CmdVer $cmd $verArgs
  if ($v) { Info ("{0}: {1}" -f $name, $v) } else { Info ("{0}: present" -f $name) }
}

function Check-PathExists([string]$rel, [string]$kind = "file", [switch]$Required) {
  $p = Join-Path $ws $rel
  $ok = Test-Path $p
  if (-not $ok) {
    if ($Required) { Err "Missing $kind: $rel" } else { Warn "Missing $kind: $rel" }
  } else {
    Info ("Found {0}: {1}" -f $kind, $rel)
  }
}

function Try-ReadFirstLine([string]$path) {
  try { (Get-Content -Path $path -TotalCount 1 -ErrorAction Stop).Trim() } catch { $null }
}

$ws = Ensure-Abs $Workspace
Header "Vitte Doctor (full)"
Info ("workspace={0}" -f $ws)
Info ("pwsh={0}" -f $PSVersionTable.PSVersion.ToString())
Info ("os={0}" -f ($(if($IsWindows){"windows"}elseif($IsMacOS){"macos"}elseif($IsLinux){"linux"}else{"unknown"})))
Info ("arch={0}" -f ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()))

Header "Repo layout"
Check-PathExists ".editorconfig" "file" -Required
Check-PathExists "CMakeLists.txt" "file" -Required
Check-PathExists "Makefile" "file" -Required:$false
Check-PathExists "LICENSE" "file" -Required
Check-PathExists "VERSION" "file" -Required
Check-PathExists "muffin.muf" "file" -Required:$false
Check-PathExists "muffin.lock" "file" -Required:$false
Check-PathExists "tools/scripts" "dir" -Required
Check-PathExists "compiler" "dir" -Required:$false
Check-PathExists "runtime" "dir" -Required:$false
Check-PathExists "std" "dir" -Required:$false

if (Test-Path (Join-Path $ws "VERSION")) {
  $v = Try-ReadFirstLine (Join-Path $ws "VERSION")
  if ($v) { Info ("VERSION={0}" -f $v) } else { Warn "VERSION file exists but empty/unreadable" }
}

Header "Core tools"
Check-Tool "git"   "git"   @("--version") -Required
Check-Tool "cmake" "cmake" @("--version") -Required
Check-Tool "ctest" "ctest" @("--version") -Required:$false
Check-Tool "ninja" "ninja" @("--version") -Required:$false
Check-Tool "make"  "make"  @("--version") -Required:$false

Header "Compilers"
Check-Tool "clang" "clang" @("--version") -Required:$false
Check-Tool "gcc"   "gcc"   @("--version") -Required:$false
if ($IsWindows) {
  # cl.exe is usually in Developer Prompt; presence varies
  if (Has "cl") { Info ("msvc cl: {0}" -f (CmdVer "cl" @("/Bv"))) } else { Warn "MSVC cl.exe not in PATH (ok if using clang/MinGW)" }
}

Header "Debuggers"
Check-Tool "lldb" "lldb" @("--version") -Required:$false
Check-Tool "gdb"  "gdb"  @("--version") -Required:$false

Header "Coverage tooling (optional)"
Check-Tool "llvm-profdata" "llvm-profdata" @("--version") -Required:$false
Check-Tool "llvm-cov"      "llvm-cov"      @("--version") -Required:$false

Header "Build directory sanity"
$bd = Join-Path $ws $BuildDir
if (-not (Test-Path $bd)) {
  Warn "Build dir not found: $BuildDir (ok if not built yet)"
} else {
  Info ("build_dir={0}" -f $bd)

  $cache = Join-Path $bd "CMakeCache.txt"
  if (Test-Path $cache) {
    Info "Found CMakeCache.txt"
    $gen = Select-String -Path $cache -Pattern '^CMAKE_GENERATOR:INTERNAL=' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($gen) { Info ("CMake generator: {0}" -f ($gen.Line -replace 'CMAKE_GENERATOR:INTERNAL=','')) }
    $cc = Select-String -Path $cache -Pattern '^CMAKE_C_COMPILER:FILEPATH=' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cc) { Info ("C compiler: {0}" -f ($cc.Line -replace 'CMAKE_C_COMPILER:FILEPATH=','')) }
  } else {
    Warn "No CMakeCache.txt found in build dir (configure not done?)"
  }

  $bin1 = Join-Path $bd "vittec"
  $bin2 = Join-Path $bd "vittec.exe"
  if (Test-Path $bin1 -or Test-Path $bin2) {
    Info "Found vittec binary in build dir"
  } else {
    Warn "vittec not found in build dir (build step missing?)"
  }
}

Header "Scripts presence (recommended)"
$scriptList = @(
  "tools/scripts/build_all.ps1",
  "tools/scripts/build_once.ps1",
  "tools/scripts/checksums.ps1",
  "tools/scripts/ci_env.ps1",
  "tools/scripts/ci_cache_restore.ps1",
  "tools/scripts/ci_artifacts_collect.ps1",
  "tools/scripts/ci_fail_fast.ps1",
  "tools/scripts/coverage.ps1",
  "tools/scripts/debug_run.ps1",
  "tools/scripts/dist_tools.ps1",
  "tools/scripts/clean.ps1"
)
foreach ($s in $scriptList) {
  Check-PathExists $s "file" -Required:$false
}

Header "Smoke test (optional)"
if ($Smoke) {
  if (-not (Test-Path $bd)) {
    Err "Smoke requested but build dir missing: $BuildDir"
  } else {
    $bin = if (Test-Path (Join-Path $bd "vittec.exe")) { Join-Path $bd "vittec.exe" }
           elseif (Test-Path (Join-Path $bd "vittec")) { Join-Path $bd "vittec" }
           else { $null }

    if (-not $bin) {
      Err "Smoke requested but vittec binary missing in $BuildDir"
    } else {
      try {
        $out = & $bin --version 2>$null
        Info ("vittec --version: {0}" -f (($out | Select-Object -First 1)))
      } catch {
        Err "Failed to run vittec --version"
      }
    }
  }
}

Header "Summary"
Write-Host ("errors  = {0}" -f $errors.Count)
Write-Host ("warnings= {0}" -f $warns.Count)

if ($errors.Count -gt 0) { exit 2 }
if ($FailOnWarn -and $warns.Count -gt 0) { exit 1 }
exit 0
