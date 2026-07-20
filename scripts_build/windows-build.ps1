param(
  [string]$Arch = $(if ($env:ARCH) { $env:ARCH } else { "all" }),
  [string]$Version = $(if ($env:VERSION) { $env:VERSION } else { "" }),
  [string]$OutDir = $(if ($env:OUT_DIR) { $env:OUT_DIR } else { "" }),
  [string]$WindowsVersion = $(if ($env:WINDOWS_VERSION) { $env:WINDOWS_VERSION } else { "all" }),
  [string]$WindowsTargets = $(if ($env:WINDOWS_TARGETS) { $env:WINDOWS_TARGETS } else { "xp vista 7 8 8.1 10 11" }),
  [string]$WindowsVitteExe = $(if ($env:WINDOWS_VITTE_EXE) { $env:WINDOWS_VITTE_EXE } else { "" }),
  [string]$SourceDateEpoch = $(if ($env:SOURCE_DATE_EPOCH) { $env:SOURCE_DATE_EPOCH } else { "" }),
  [switch]$ListTargets,
  [switch]$PrintEnv,
  [switch]$DryRun,
  [switch]$VerifyOnly,
  [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Resolve-Path (Join-Path $ScriptDir "..")
$Builder = Join-Path $ScriptDir "build-windows.ps1"
$PackageVersionFile = Join-Path $RootDir "toolchain/scripts/package/PACKAGE_VERSION"

if (-not (Test-Path $Builder)) {
  throw "Required professional Windows builder is missing: $Builder"
}

if ([string]::IsNullOrWhiteSpace($Version)) {
  if (Test-Path $PackageVersionFile) {
    $Version = (Get-Content -Raw $PackageVersionFile).Trim()
  } else {
    $Version = "0.0.0"
  }
}

if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $RootDir "pkgout"
}

$KnownArchitectures = @("i386", "amd64", "arm64", "armv7")
$KnownWindowsVersions = @("xp", "vista", "7", "8", "8.1", "10", "11")

function Resolve-Architectures {
  param([string]$Value)

  switch ($Value.ToLowerInvariant()) {
    "all" { return $KnownArchitectures }
    "x86" { return @("i386") }
    "i386" { return @("i386") }
    "i486" { return @("i386") }
    "i586" { return @("i386") }
    "i686" { return @("i386") }
    "x64" { return @("amd64") }
    "x86_64" { return @("amd64") }
    "amd64" { return @("amd64") }
    "aarch64" { return @("arm64") }
    "arm64" { return @("arm64") }
    "arm" { return @("armv7") }
    "armv7" { return @("armv7") }
    default { throw "Unsupported Windows architecture: $Value" }
  }
}

function Resolve-WindowsVersions {
  param([string]$Value)

  if ($Value.ToLowerInvariant() -eq "all") {
    return $KnownWindowsVersions
  }

  $parts = $Value -split "[, ]+" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
  foreach ($part in $parts) {
    if ($KnownWindowsVersions -notcontains $part.ToLowerInvariant()) {
      throw "Unsupported Windows version target: $part"
    }
  }
  return @($parts | ForEach-Object { $_.ToLowerInvariant() })
}

function Write-JsonFile {
  param(
    [string]$Path,
    [object]$Value
  )

  $json = $Value | ConvertTo-Json -Depth 8
  Set-Content -Path $Path -Value ($json + "`n") -Encoding UTF8
}

$Architectures = Resolve-Architectures $Arch
$WindowsVersions = Resolve-WindowsVersions $WindowsVersion
$TargetRows = foreach ($os in $WindowsVersions) {
  foreach ($targetArch in $Architectures) {
    [pscustomobject]@{
      family = "windows"
      windows_version = $os
      arch = $targetArch
      installer = "professional"
      artifact = "vitte-$Version-windows-$os-$targetArch-professional.exe"
    }
  }
}

if ($ListTargets) {
  $TargetRows | ForEach-Object {
    "{0}/{1}/{2}/professional" -f $_.family, $_.windows_version, $_.arch
  }
  exit 0
}

if ($PrintEnv) {
  $envReport = [pscustomobject]@{
    version = $Version
    out_dir = $OutDir
    arch = $Arch
    resolved_architectures = $Architectures
    windows_version = $WindowsVersion
    resolved_windows_versions = $WindowsVersions
    windows_targets = $WindowsTargets
    source_date_epoch = $SourceDateEpoch
    professional_builder = $Builder
  }
  $envReport | ConvertTo-Json -Depth 8
  exit 0
}

if ($VerifyOnly) {
  if (-not (Test-Path $OutDir)) {
    throw "Output directory does not exist: $OutDir"
  }
  $missing = @()
  foreach ($row in $TargetRows) {
    $artifact = Join-Path $OutDir $row.artifact
    if (-not (Test-Path $artifact)) {
      $missing += $row.artifact
    }
  }
  if ($missing.Count -gt 0) {
    throw "Missing Windows installer artifacts: $($missing -join ', ')"
  }
  Write-Host "[windows-build] verify-only passed targets=$($TargetRows.Count) out=$OutDir"
  exit 0
}

if ($DryRun) {
  Write-Host "[windows-build][dry-run] version=$Version out=$OutDir targets=$($TargetRows.Count)"
  $TargetRows | ForEach-Object {
    Write-Host "[windows-build][dry-run] would build $($_.artifact)"
  }
  exit 0
}

if ($Clean) {
  $cleanRoot = Join-Path $RootDir "target"
  Get-ChildItem -Path $cleanRoot -Filter "installer-windows-*" -Directory -ErrorAction SilentlyContinue |
    Remove-Item -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$env:VERSION = $Version
$env:OUT_DIR = $OutDir
$env:WINDOWS_TARGETS = $WindowsTargets
if (-not [string]::IsNullOrWhiteSpace($WindowsVitteExe)) {
  $env:WINDOWS_VITTE_EXE = $WindowsVitteExe
}
if (-not [string]::IsNullOrWhiteSpace($SourceDateEpoch)) {
  $env:SOURCE_DATE_EPOCH = $SourceDateEpoch
}

$BuiltArtifacts = @()
foreach ($targetArch in $Architectures) {
  $env:ARCH = $targetArch
  $PowerShellExe = (Get-Process -Id $PID).Path
  & $PowerShellExe -NoProfile -ExecutionPolicy Bypass -File $Builder -Arch $targetArch -Version $Version -OutDir $OutDir -WindowsTargets $WindowsTargets -WindowsVitteExe $WindowsVitteExe
  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }

  $genericExe = Join-Path $OutDir "vitte-$Version-windows-$targetArch-installer.exe"
  $genericKit = Join-Path $OutDir "vitte-$Version-windows-$targetArch-nsis.tar.gz"
  foreach ($os in $WindowsVersions) {
    $professionalExe = Join-Path $OutDir "vitte-$Version-windows-$os-$targetArch-professional.exe"
    $professionalKit = Join-Path $OutDir "vitte-$Version-windows-$os-$targetArch-professional-nsis.tar.gz"

    if (Test-Path $genericExe) {
      Copy-Item -Force $genericExe $professionalExe
      $BuiltArtifacts += $professionalExe
    } elseif (Test-Path $genericKit) {
      Copy-Item -Force $genericKit $professionalKit
      $BuiltArtifacts += $professionalKit
    } else {
      throw "Professional Windows installer was not generated for $targetArch"
    }

    $manifestPath = Join-Path $OutDir "vitte-$Version-windows-$os-$targetArch-professional.MANIFEST.json"
    $artifactPath = $BuiltArtifacts[-1]
    $artifactInfo = Get-Item $artifactPath
    $sha256 = (Get-FileHash -Algorithm SHA256 $artifactPath).Hash.ToLowerInvariant()
    Write-JsonFile -Path $manifestPath -Value ([pscustomobject]@{
      schema = "org.vitte.windows-professional-installer.v1"
      name = Split-Path -Leaf $artifactPath
      platform = "windows"
      windows_version = $os
      arch = $targetArch
      version = $Version
      installer = "professional"
      size = $artifactInfo.Length
      sha256 = $sha256
    })
  }
}

$installersJson = Join-Path $OutDir "WINDOWS_INSTALLERS.json"
Write-JsonFile -Path $installersJson -Value ([pscustomobject]@{
  schema = "org.vitte.windows-installer-matrix.v1"
  version = $Version
  generated_by = "scripts_build/windows-build.ps1"
  windows_versions = $WindowsVersions
  architectures = $Architectures
  installer = "professional"
  artifacts = $BuiltArtifacts | ForEach-Object { Split-Path -Leaf $_ }
})

Write-Host "[windows-build] complete version=$Version targets=$($TargetRows.Count) artifacts=$($BuiltArtifacts.Count) out=$OutDir"
