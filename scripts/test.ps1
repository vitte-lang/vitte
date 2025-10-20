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
    [switch]$NoFailFast,
    [switch]$UseNextest,
    [string]$Junit,
    [switch]$Doc,
    [switch]$NoDoc,
    [switch]$Bins,
    [switch]$Examples,
    [switch]$Benches,
    [switch]$Ignored,
    [switch]$NoCapture,
    [string]$Filter,
    [int]$Verbosity = 0,
    [switch]$Quiet,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$HarnessArgs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1
    $useNextest = $false
    if ($UseNextest) {
        $nextest = Get-Command cargo-nextest -ErrorAction SilentlyContinue
        if ($nextest) {
            $useNextest = $true
        }
        else {
            Write-Warning "cargo-nextest introuvable — fallback sur 'cargo test'."
            if ($Junit) {
                Write-Warning "--Junit ignoré (nécessite cargo-nextest)."
            }
        }
    }
    elseif ($Junit) {
        Write-Warning "--Junit ignoré (repose sur cargo-nextest)."
    }

    $args = New-Object System.Collections.Generic.List[string]
    if ($useNextest) {
        $args.Add('nextest')
        $args.Add('run')
        if ($Junit) {
            $dir = Split-Path -Parent $Junit
            if ($dir) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
            $args.Add('--junit')
            $args.Add($Junit)
        }
    }
    else {
        $args.Add('test')
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
    if ($NoFailFast) { $args.Add('--no-fail-fast') }
    if ($Quiet) { $args.Add('--quiet') }
    for ($i = 0; $i -lt $Verbosity; $i++) { $args.Add('--verbose') }

    if ($Doc) {
        $args.Add('--doc')
    }
    else {
        if ($NoDoc) { $args.Add('--tests') }
        if ($Bins) { $args.Add('--bins') }
        if ($Examples) { $args.Add('--examples') }
        if ($Benches) { $args.Add('--benches') }
    }

    $harness = @()
    if ($Ignored) { $harness += '--ignored' }
    if ($NoCapture) { $harness += '--nocapture' }
    if ($Filter) { $harness += $Filter }
    foreach ($extra in ($HarnessArgs | Where-Object { $_ })) {
        $harness += $extra
    }

    if ($harness.Count -gt 0) {
        $args.Add('--')
        $args.AddRange($harness)
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
