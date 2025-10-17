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
    [switch]$CheckOnly,
    [switch]$Quiet,
    [switch]$CleanFirst,
    [int]$Verbosity = 0,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraCargoArgs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1

    if ($CleanFirst) {
        Write-Host "▶ cargo clean" -ForegroundColor Cyan
        & $cargo.Source clean
        if ($LASTEXITCODE -ne 0) {
            throw "cargo clean failed with exit code $LASTEXITCODE."
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
