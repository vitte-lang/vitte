# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\dist_tools.ps1
<#
dist_tools.ps1 (max)

Distribution packager for Vitte tools.

Builds and packages:
  - vittec (compiler/tool)
  - optional extra tools (if present): vitte_fmt, vitte_lsp, etc. (best-effort)
  - metadata: VERSION, LICENSE, README*, muffin.muf, muffin.lock, checksums.sha256
  - optional: std/ (if requested)
  - optional: include symbols (PDB/.dSYM/.debug) best-effort
  - produces:
      dist/tools/<name>-<version>-<os>-<arch>/
      dist/tools/<name>-<version>-<os>-<arch>.zip

This script is CI-friendly, but usable locally.

Usage:
  pwsh tools/scripts/dist_tools.ps1
  pwsh tools/scripts/dist_tools.ps1 -Mode Release -IncludeStd -IncludeSymbols
  pwsh tools/scripts/dist_tools.ps1 -Mode Release -Tools vittec,vitte_fmt -OutRoot dist/tools -Jobs 12
  pwsh tools/scripts/dist_tools.ps1 -Mode Debug -NoBuild   # package existing build output

Exit codes:
  0 ok
  2 build/configure/tooling error
  3 packaging error (missing binaries, compress failure)
#>

[CmdletBinding()]
param(
  # Build mode mapped to build directory names (Release/Debug/Asan/RelWithDebInfo)
  [ValidateSet("Release","Debug","Asan","RelWithDebInfo")]
  [string]$Mode = "Release",

  # Root build folder (contains release/, debug/ etc. subdirs)
  [string]$BuildRoot = "build",

  # Output root for packages
  [string]$OutRoot = "dist/tools",

  # Package name prefix
  [string]$PackageName = "vitte-tools",

  # Comma-separated tool list to include; "auto" tries to include what exists.
  [string]$Tools = "auto",

  # If set, does not run cmake build; packages existing binaries only.
  [switch]$NoBuild,

  # If set, also include std/ in the package.
  [switch]$IncludeStd,

  # If set, include debug symbols when found/available.
  [switch]$IncludeSymbols,

  # If set, include selected build outputs like compile_commands.json, CMakeCache.txt.
  [switch]$IncludeBuildMeta,

  # Parallel jobs for cmake --build (0 = default)
  [ValidateRange(0,256)]
  [int]$Jobs = 0,

  # If set, overwrite existing package dir/zip.
  [switch]$Force
)

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }
function Has([string]$Cmd) { return [bool](Get-Command $Cmd -ErrorAction SilentlyContinue) }
function Ensure-Dir([string]$Path) { if (-not (Test-Path $Path)) { New-Item -ItemType Directory -Force -Path $Path | Out-Null } }

function Arch-Normalize([string]$a) {
  switch -Regex ($a) {
    '^(amd64|x64|x86_64)$' { "x86_64" }
    '^(arm64|aarch64)$'    { "aarch64" }
    '^(x86|i386|i686)$'    { "x86" }
    default                { $a }
  }
}
function Os-Normalize {
  if ($IsWindows) { return "windows" }
  if ($IsMacOS)   { return "macos" }
  if ($IsLinux)   { return "linux" }
  return "unknown"
}

function BuildDir-ForMode([string]$M) {
  switch ($M) {
    "Release" { return Join-Path $BuildRoot "release" }
    "Debug"   { return Join-Path $BuildRoot "debug" }
    "Asan"    { return Join-Path $BuildRoot "asan" }
    "RelWithDebInfo" { return Join-Path $BuildRoot "relwithdebinfo" }
  }
}

function ConfigureAndBuild([string]$BuildDir, [string]$Cfg, [string]$Sanitizers) {
  if (-not (Has "cmake")) { Fail "cmake not found in PATH." 2 }

  Ensure-Dir $BuildDir

  $gen = @()
  if (Has "ninja") { $gen = @("-G","Ninja") }

  $cache = Join-Path $BuildDir "CMakeCache.txt"
  if (-not (Test-Path $cache)) {
    & cmake -S . -B $BuildDir @gen `
      "-DCMAKE_BUILD_TYPE=$Cfg" `
      "-DVITTE_BUILD_TESTS=OFF" `
      "-DVITTE_BUILD_BENCH=OFF" `
      "-DVITTE_ENABLE_SANITIZERS=$Sanitizers" | Out-Host
    if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }
  }

  if ($Jobs -gt 0) {
    & cmake --build $BuildDir --config $Cfg -- "-j" "$Jobs" | Out-Host
  } else {
    & cmake --build $BuildDir --config $Cfg | Out-Host
  }
  if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }
}

function Resolve-Exe([string]$Dir, [string]$Name) {
  $p1 = Join-Path $Dir $Name
  $p2 = Join-Path $Dir ($Name + ".exe")
  if (Test-Path $p1) { return $p1 }
  if (Test-Path $p2) { return $p2 }
  return $null
}

function Copy-IfExists([string]$Src, [string]$Dst) {
  if (Test-Path $Src) {
    Ensure-Dir (Split-Path $Dst -Parent)
    Copy-Item -Force -Recurse -Path $Src -Destination $Dst
    return $true
  }
  return $false
}

function Get-Version {
  if (Test-Path "VERSION") {
    $v = (Get-Content -Path "VERSION" -Raw).Trim()
    if (-not [string]::IsNullOrWhiteSpace($v)) { return $v }
  }
  return "0.0.0"
}

function Parse-Tools([string]$s, [string]$BuildDir) {
  if ($s -eq "auto") {
    $candidates = @("vittec","vitte_fmt","vitte_lsp","vitte_muf","vitte_regex","vitte_unicode")
    $out = @()
    foreach ($t in $candidates) {
      if (Resolve-Exe $BuildDir $t) { $out += $t }
    }
    if ($out.Count -eq 0) { $out = @("vittec") }
    return $out
  }
  return ($s.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" })
}

function Collect-Symbols([string]$bin, [string]$symDir) {
  # Windows: .pdb next to exe
  $ok = $false
  $pdb = [System.IO.Path]::ChangeExtension($bin, ".pdb")
  if (Test-Path $pdb) {
    Copy-Item -Force -Path $pdb -Destination $symDir
    $ok = $true
  }

  # macOS: .dSYM (if present) or generate if dsymutil exists
  if ($IsMacOS) {
    $dsym = $bin + ".dSYM"
    if (Test-Path $dsym) {
      Copy-Item -Force -Recurse -Path $dsym -Destination $symDir
      $ok = $true
    } elseif (Has "dsymutil") {
      $out = Join-Path $symDir ((Split-Path $bin -Leaf) + ".dSYM")
      & dsymutil $bin -o $out | Out-Null
      if ($LASTEXITCODE -eq 0 -and (Test-Path $out)) { $ok = $true }
    }
  }

  # Linux: split debug (objcopy)
  if ($IsLinux -and (Has "objcopy")) {
    $dbg = Join-Path $symDir ((Split-Path $bin -Leaf) + ".debug")
    & objcopy --only-keep-debug $bin $dbg | Out-Null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $dbg)) { $ok = $true }
  }

  return $ok
}

# Preconditions
if (-not (Has "Compress-Archive")) { Fail "Compress-Archive not available." 2 }

$os = Os-Normalize
$arch = Arch-Normalize ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant())
$ver = Get-Version

$buildDir = BuildDir-ForMode $Mode

$cfg = switch ($Mode) {
  "Release" { "Release" }
  "Debug" { "Debug" }
  "RelWithDebInfo" { "RelWithDebInfo" }
  "Asan" { "Debug" }
}
$san = if ($Mode -eq "Asan") { "ON" } else { "OFF" }

if (-not $NoBuild) {
  ConfigureAndBuild -BuildDir $buildDir -Cfg $cfg -Sanitizers $san
}

$toolList = Parse-Tools -s $Tools -BuildDir $buildDir

$pkgDirName = "{0}-{1}-{2}-{3}" -f $PackageName, $ver, $os, $arch
$pkgDir = Join-Path $OutRoot $pkgDirName
$zipPath = Join-Path $OutRoot ($pkgDirName + ".zip")

if ($Force) {
  if (Test-Path $pkgDir) { Remove-Item -Recurse -Force -Path $pkgDir -ErrorAction SilentlyContinue }
  if (Test-Path $zipPath) { Remove-Item -Force -Path $zipPath -ErrorAction SilentlyContinue }
}

Ensure-Dir $OutRoot
Ensure-Dir $pkgDir
Ensure-Dir (Join-Path $pkgDir "bin")
Ensure-Dir (Join-Path $pkgDir "meta")

Write-Host ("[dist] mode={0} build_dir={1}" -f $Mode, $buildDir)
Write-Host ("[dist] package={0}" -f $pkgDirName)
Write-Host ("[dist] tools={0}" -f ($toolList -join ", "))

# Copy tools
foreach ($t in $toolList) {
  $exe = Resolve-Exe $buildDir $t
  if (-not $exe) { Fail "Missing tool binary in $buildDir: $t" 3 }
  Copy-Item -Force -Path $exe -Destination (Join-Path $pkgDir "bin")
}

# Metadata
Copy-IfExists "VERSION"      (Join-Path $pkgDir "meta/VERSION")      | Out-Null
Copy-IfExists "LICENSE"      (Join-Path $pkgDir "meta/LICENSE")      | Out-Null
Copy-IfExists "muffin.muf"   (Join-Path $pkgDir "meta/muffin.muf")   | Out-Null
Copy-IfExists "muffin.lock"  (Join-Path $pkgDir "meta/muffin.lock")  | Out-Null
Copy-IfExists "checksums.sha256" (Join-Path $pkgDir "meta/checksums.sha256") | Out-Null
Get-ChildItem -Path "." -File -Filter "README*" -ErrorAction SilentlyContinue |
  ForEach-Object { Copy-IfExists $_.FullName (Join-Path $pkgDir ("meta/" + $_.Name)) | Out-Null }

# Optional std/
if ($IncludeStd -and (Test-Path "std")) {
  Write-Host "[dist] include std/"
  Copy-Item -Recurse -Force -Path "std" -Destination (Join-Path $pkgDir "std")
}

# Optional build metadata
if ($IncludeBuildMeta) {
  Copy-IfExists (Join-Path $buildDir "compile_commands.json") (Join-Path $pkgDir "meta/build/compile_commands.json") | Out-Null
  Copy-IfExists (Join-Path $buildDir "CMakeCache.txt")        (Join-Path $pkgDir "meta/build/CMakeCache.txt")        | Out-Null
}

# Optional symbols
if ($IncludeSymbols) {
  $symDir = Join-Path $pkgDir "symbols"
  Ensure-Dir $symDir
  foreach ($t in $toolList) {
    $exe = Resolve-Exe $buildDir $t
    if ($exe) { [void](Collect-Symbols -bin $exe -symDir $symDir) }
  }
}

# Write an inventory manifest
$inv = Join-Path $pkgDir "manifest.txt"
$lines = @()
$lines += "package=$pkgDirName"
$lines += "version=$ver"
$lines += "os=$os"
$lines += "arch=$arch"
$lines += "mode=$Mode"
$lines += "tools=" + ($toolList -join ",")
$lines += "timestamp_utc=" + (Get-Date).ToUniversalTime().ToString("o")
$lines | Out-File -FilePath $inv -Encoding utf8

# Zip
if (Test-Path $zipPath) { Remove-Item -Force -Path $zipPath -ErrorAction SilentlyContinue }
try {
  Compress-Archive -Path $pkgDir -DestinationPath $zipPath -Force
} catch {
  Fail ("Compress-Archive failed: {0}" -f $_) 3
}

Write-Host "[dist] outputs:"
Write-Host ("  dir = {0}" -f $pkgDir)
Write-Host ("  zip = {0}" -f $zipPath)

exit 0
