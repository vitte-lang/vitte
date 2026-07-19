param(
  [string]$Arch = $(if ($env:ARCH) { $env:ARCH } else { "all" }),
  [string]$Version = $env:VERSION,
  [string]$OutDir = $env:OUT_DIR,
  [string]$WindowsTargets = $(if ($env:WINDOWS_TARGETS) { $env:WINDOWS_TARGETS } else { "xp vista 7 8 8.1 10 11" }),
  [string]$WindowsVitteExe = $env:WINDOWS_VITTE_EXE
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$builder = Join-Path $scriptDir "build-windows-installer.sh"

if (-not (Test-Path $builder)) {
  throw "Windows installer builder not found: $builder"
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

function Quote-Sh {
  param([string]$Value)

  return "'" + ($Value -replace "'", "'\''") + "'"
}

$env:ARCH = $Arch
$env:WINDOWS_TARGETS = $WindowsTargets

if (-not [string]::IsNullOrWhiteSpace($Version)) {
  $env:VERSION = $Version
}

if (-not [string]::IsNullOrWhiteSpace($OutDir)) {
  $env:OUT_DIR = $OutDir
}

if (-not [string]::IsNullOrWhiteSpace($WindowsVitteExe)) {
  $env:WINDOWS_VITTE_EXE = $WindowsVitteExe
}

$posixRoot = ConvertTo-PosixPath $rootDir
$posixBuilder = ConvertTo-PosixPath $builder
$command = "cd $(Quote-Sh $posixRoot) && $(Quote-Sh $posixBuilder)"

& $sh -lc $command
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
