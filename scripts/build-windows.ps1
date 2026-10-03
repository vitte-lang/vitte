param(
  [string]$Arch = $(if ($env:ARCH) { $env:ARCH } else { "all" }),
  [string]$Version = $env:VERSION,
  [string]$OutDir = $env:OUT_DIR,
  [Alias("Name")]
  [string]$PackageName = $(if ($env:PACKAGE_NAME) { $env:PACKAGE_NAME } else { "vitte" }),
  [string]$WindowsTargets = $(if ($env:WINDOWS_TARGETS) { $env:WINDOWS_TARGETS } else { "xp vista 7 8 8.1 10 11" }),
  [Alias("VitteBin", "WindowsVitte")]
  [string]$WindowsVitteExe = $env:WINDOWS_VITTE_EXE,
  [Alias("Signtool")]
  [string]$WindowsSigntool = $(if ($env:WINDOWS_SIGNTOOL) { $env:WINDOWS_SIGNTOOL } else { "signtool" }),
  [Alias("SignCertificate")]
  [string]$WindowsSignCert = $env:WINDOWS_SIGN_CERT,
  [switch]$Sign,
  [switch]$StrictNative,
  [switch]$DryRun,
  [switch]$ListTargets,
  [switch]$PrintEnv,
  [switch]$Help
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$builder = Join-Path $scriptDir "build-windows-installer.sh"

if (-not (Test-Path $builder)) {
  throw "Windows installer builder not found: $builder"
}

$validArchitectures = @("all", "amd64", "x86_64", "x64", "i386", "x86", "arm64", "aarch64", "armv7", "arm")
if ($validArchitectures -notcontains $Arch.ToLowerInvariant()) {
  throw "Unsupported Windows architecture '$Arch'. Valid values: $($validArchitectures -join ', ')"
}

if ([string]::IsNullOrWhiteSpace($WindowsTargets)) {
  throw "WindowsTargets must contain at least one target (for example: '10 11')."
}

if ([string]::IsNullOrWhiteSpace($PackageName)) {
  throw "PackageName must not be empty."
}

if ($Sign -and [string]::IsNullOrWhiteSpace($WindowsSignCert)) {
  throw "-Sign requires -WindowsSignCert or WINDOWS_SIGN_CERT."
}

$shellCandidates = @()

if ($env:VITTE_SH) {
  $shellCandidates += $env:VITTE_SH
}

$shellCandidates += @(
  "C:\Program Files\Git\bin\sh.exe",
  "C:\Program Files\Git\usr\bin\sh.exe",
  "C:\Program Files (x86)\Git\bin\sh.exe",
  "C:\Program Files (x86)\Git\usr\bin\sh.exe",
  "sh.exe"
)

$sh = $null

foreach ($candidate in $shellCandidates) {
  if ([string]::IsNullOrWhiteSpace($candidate)) {
    continue
  }

  $command = Get-Command $candidate -ErrorAction SilentlyContinue

  if ($command) {
    $sh = $command.Source
    break
  }

  if (Test-Path $candidate) {
    $sh = $candidate
    break
  }
}

if (-not $sh) {
  throw "A POSIX shell is required to build the NSIS kits. Install Git for Windows or set VITTE_SH to sh.exe."
}

function ConvertTo-PosixPath {
  param([string]$Path)

  $resolved = (Resolve-Path $Path).Path
  $cygpathCandidates = @(
    (Join-Path (Split-Path -Parent $sh) "cygpath.exe"),
    (Join-Path (Split-Path -Parent (Split-Path -Parent $sh)) "usr\bin\cygpath.exe"),
    "cygpath.exe"
  )

  foreach ($candidate in $cygpathCandidates) {
    $command = Get-Command $candidate -ErrorAction SilentlyContinue
    if ($command) {
      return (& $command.Source -u $resolved).Trim()
    }
  }

  if ($resolved -match "^([A-Za-z]):\\(.*)$") {
    $drive = $Matches[1].ToLowerInvariant()
    $tail = $Matches[2] -replace "\\", "/"
    return "/$drive/$tail"
  }

  return $resolved -replace "\\", "/"
}

function ConvertTo-PosixPathLoose {
  param([string]$Path)

  if ([string]::IsNullOrWhiteSpace($Path)) {
    return $Path
  }

  if (Test-Path -LiteralPath $Path) {
    return ConvertTo-PosixPath $Path
  }

  if ($Path -match "^([A-Za-z]):[\\/](.*)$") {
    return "/$($Matches[1].ToLowerInvariant())/$($Matches[2] -replace '\\', '/')"
  }

  return $Path -replace '\\', '/'
}

function Quote-Sh {
  param([string]$Value)

  return "'" + ($Value -replace "'", "'\''") + "'"
}

function Set-ForwardedEnvironment {
  param(
    [string]$Name,
    [AllowEmptyString()][string]$Value
  )

  if ($null -ne $Value -and $Value.Length -gt 0) {
    Set-Item -Path "Env:$Name" -Value $Value
  }
}

$env:ARCH = $Arch.ToLowerInvariant()
$env:WINDOWS_TARGETS = $WindowsTargets
$env:PACKAGE_NAME = $PackageName
$env:WINDOWS_SIGNTOOL = $WindowsSigntool

if ($Sign) { $env:SIGN = "1" }
if ($StrictNative) { $env:STRICT_NATIVE = "1" }

Set-ForwardedEnvironment -Name VERSION -Value $Version

if (-not [string]::IsNullOrWhiteSpace($OutDir)) {
  $env:OUT_DIR = ConvertTo-PosixPathLoose $OutDir
}

if (-not [string]::IsNullOrWhiteSpace($WindowsVitteExe)) {
  if (-not (Test-Path -LiteralPath $WindowsVitteExe -PathType Leaf)) {
    throw "Windows compiler payload not found: $WindowsVitteExe"
  }
  $env:WINDOWS_VITTE_EXE = ConvertTo-PosixPath $WindowsVitteExe
}

if (-not [string]::IsNullOrWhiteSpace($WindowsSignCert)) {
  if ($Sign -and -not (Test-Path -LiteralPath $WindowsSignCert -PathType Leaf)) {
    throw "Windows signing certificate not found: $WindowsSignCert"
  }
  $env:WINDOWS_SIGN_CERT = ConvertTo-PosixPathLoose $WindowsSignCert
}

$posixRoot = ConvertTo-PosixPath $rootDir
$posixBuilder = ConvertTo-PosixPath $builder
$builderOptions = @()
if ($Help) { $builderOptions += "--help" }
if ($DryRun) { $builderOptions += "--dry-run" }
if ($ListTargets) { $builderOptions += "--list-targets" }
if ($PrintEnv) { $builderOptions += "--print-env" }
$command = "cd $(Quote-Sh $posixRoot) && $(Quote-Sh $posixBuilder) $($builderOptions -join ' ')"

& $sh -lc $command
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
