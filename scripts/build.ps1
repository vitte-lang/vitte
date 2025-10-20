[CmdletBinding()]
param(
    [string[]]$Package,
    [switch]$Workspace,
    [switch]$Release,
    [string]$Profile,
    [string]$Target,
    [switch]$AllFeatures,
    [string[]]$Features,
    [switch]$NoDefaultFeatures,
    [switch]$Locked,
    [switch]$Frozen,
    [switch]$AnalysisOnly,
    [switch]$SkipAnalysis,
    [switch]$CheckOnly,
    [switch]$Quiet,
    [switch]$CleanFirst,
    [int]$Verbosity = 0,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraCargoArgs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$strictWarnings = $true
if ($env:VITTE_STRICT_WARNINGS) {
    switch ($env:VITTE_STRICT_WARNINGS.ToString().ToLowerInvariant()) {
        '0' { $strictWarnings = $false }
        'false' { $strictWarnings = $false }
        'no' { $strictWarnings = $false }
        'off' { $strictWarnings = $false }
    }
}

function Add-UniqueFlag {
    param(
        [string]$Name,
        [string]$Flag
    )
    $current = [System.Environment]::GetEnvironmentVariable($Name, 'Process')
    if ([string]::IsNullOrWhiteSpace($current)) {
        [System.Environment]::SetEnvironmentVariable($Name, $Flag, 'Process')
        return
    }

    $parts = $current -split '\s+'
    if ($parts -contains $Flag) {
        return
    }

    [System.Environment]::SetEnvironmentVariable($Name, "$current $Flag", 'Process')
}

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1

    $analysisEnabled = $true
    $analysisOnlyFlag = [bool]$AnalysisOnly
    $skipReason = $null

    if ($env:VITTE_BUILD_SKIP_ANALYSIS) {
        $skipValue = $env:VITTE_BUILD_SKIP_ANALYSIS.ToString().ToLowerInvariant()
        if ($skipValue -match '^(1|true|yes|on)$') {
            $analysisEnabled = $false
            $skipReason = 'VITTE_BUILD_SKIP_ANALYSIS'
        }
    }

    if ($SkipAnalysis.IsPresent) {
        $analysisEnabled = $false
        $skipReason = '--SkipAnalysis'
    }

    if ($analysisOnlyFlag) {
        $analysisEnabled = $true
        $skipReason = $null
    }

    if ($strictWarnings) {
        Add-UniqueFlag -Name 'RUSTFLAGS' -Flag '-Dwarnings'
        Add-UniqueFlag -Name 'RUSTDOCFLAGS' -Flag '-Dwarnings'
        [System.Environment]::SetEnvironmentVariable('VITTE_STRICT_WARNINGS_EMITTED', '1', 'Process')
        Write-Host "▶ mode strict (warnings → erreurs)" -ForegroundColor Cyan
    }
    else {
        [System.Environment]::SetEnvironmentVariable('VITTE_STRICT_WARNINGS_EMITTED', $null, 'Process')
        Write-Warning "mode strict désactivé (VITTE_STRICT_WARNINGS)"
    }

    if ($CleanFirst) {
        Write-Host "▶ cargo clean" -ForegroundColor Cyan
        & $cargo.Source clean
        if ($LASTEXITCODE -ne 0) {
            throw "cargo clean failed with exit code $LASTEXITCODE."
        }
    }

    if ($analysisEnabled) {
        Write-Host "▶ scripts/check.ps1 (strict analysis)" -ForegroundColor Cyan
        & "$root/scripts/check.ps1"
        if ($LASTEXITCODE -ne 0) {
            throw "scripts/check.ps1 exited with code $LASTEXITCODE."
        }
        if ($analysisOnlyFlag) {
            Write-Host "Strict analysis complete — build skipped (--AnalysisOnly)." -ForegroundColor Green
            return
        }
    }
    else {
        if ($skipReason) {
            Write-Warning ("Strict analysis skipped ({0})." -f $skipReason)
        }
        else {
            Write-Warning "Strict analysis skipped."
        }
    }

    $args = New-Object System.Collections.Generic.List[string]
    if ($CheckOnly) {
        $args.Add('check')
    }
    else {
        $args.Add('build')
    }

    if ($Workspace -or (-not $Package)) {
        $args.Add('--workspace')
    }
    foreach ($pkg in ($Package | Where-Object { $_ })) {
        $args.Add('--package')
        $args.Add($pkg)
    }
    if ($Release) { $args.Add('--release') }
    if ($Profile) { $args.Add('--profile'); $args.Add($Profile) }
    if ($Target) { $args.Add('--target'); $args.Add($Target) }
    if ($AllFeatures) { $args.Add('--all-features') }
    elseif ($Features) {
        $args.Add('--features')
        $args.Add(($Features -join ','))
    }
    if ($NoDefaultFeatures) { $args.Add('--no-default-features') }
    if ($Locked) { $args.Add('--locked') }
    if ($Frozen) { $args.Add('--frozen') }
    if ($Quiet) { $args.Add('--quiet') }
    for ($i = 0; $i -lt $Verbosity; $i++) { $args.Add('--verbose') }
    foreach ($extra in ($ExtraCargoArgs | Where-Object { $_ })) {
        $args.Add($extra)
    }

    Write-Host ("▶ cargo {0}" -f ($args -join ' ')) -ForegroundColor Cyan
    & $cargo.Source @args
    if ($LASTEXITCODE -ne 0) {
        throw "cargo exited with code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
