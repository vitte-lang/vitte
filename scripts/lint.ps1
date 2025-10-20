[CmdletBinding()]
param(
    [switch]$Fix,
    [switch]$NoFmt,
    [switch]$NoClippy,
    [switch]$NoDeny,
    [switch]$AllowWarnings,
    [string[]]$Package,
    [switch]$Workspace,
    [switch]$AllFeatures,
    [string[]]$Features,
    [switch]$NoDefaultFeatures,
    [switch]$Locked,
    [int]$Verbosity = 0,
    [switch]$Quiet,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ExtraClippyArgs
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-Cargo {
    param([string[]]$Arguments)
    $cargo = $script:cargo
    Write-Host ("▶ cargo {0}" -f ($Arguments -join ' ')) -ForegroundColor Cyan
    & $cargo.Source @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "cargo exited with code $LASTEXITCODE."
    }
}

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $script:cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1

    if (-not $NoFmt) {
        if ($Fix) {
            try {
                Invoke-Cargo @('fmt', '--all')
            }
            catch {
                $hasRustup = (Get-Command rustup -ErrorAction SilentlyContinue)
                if ($hasRustup -and ((rustup toolchain list) -join "\n") -match 'nightly') {
                    Write-Warning 'rustfmt stable a échoué; tentative avec nightly'
                    & cargo +nightly fmt --all
                    if ($LASTEXITCODE -ne 0) { throw }
                }
                else {
                    Write-Warning "rustfmt a échoué et nightly introuvable. Installe: rustup toolchain install nightly"
                    throw
                }
            }
        }
        else {
            $ok = $true
            try {
                Invoke-Cargo @('fmt', '--all', '--', '--check')
            }
            catch {
                $ok = $false
            }
            if (-not $ok) {
                $hasRustup = (Get-Command rustup -ErrorAction SilentlyContinue)
                if ($hasRustup -and ((rustup toolchain list) -join "\n") -match 'nightly') {
                    Write-Warning 'rustfmt stable a échoué; tentative avec nightly'
                    & cargo +nightly fmt --all -- --check
                    if ($LASTEXITCODE -ne 0) { throw }
                }
                else {
                    Write-Warning "rustfmt a échoué et nightly introuvable. Installe: rustup toolchain install nightly"
                    throw
                }
            }
        }
    }
    else {
        Write-Host '▶ cargo fmt skipped (--NoFmt)' -ForegroundColor Yellow
    }

    if (-not $NoClippy) {
        $args = New-Object System.Collections.Generic.List[string]
        $args.Add('clippy')
        if ($Workspace -or (-not $Package)) { $args.Add('--workspace') }
        foreach ($pkg in ($Package | Where-Object { $_ })) {
            $args.Add('--package')
            $args.Add($pkg)
        }
        if ($AllFeatures) { $args.Add('--all-features') }
        elseif ($Features) {
            $args.Add('--features')
            $args.Add(($Features -join ','))
        }
        if ($NoDefaultFeatures) { $args.Add('--no-default-features') }
        if ($Locked) { $args.Add('--locked') }
        if ($Quiet) { $args.Add('--quiet') }
        for ($i = 0; $i -lt $Verbosity; $i++) { $args.Add('--verbose') }

        if (-not $AllowWarnings) {
            $args.Add('--')
            $args.Add('-D')
            $args.Add('warnings')
        }

        foreach ($extra in ($ExtraClippyArgs | Where-Object { $_ })) {
            $args.Add($extra)
        }

        Invoke-Cargo $args
    }
    else {
        Write-Host '▶ cargo clippy skipped (--NoClippy)' -ForegroundColor Yellow
    }

    if (-not $NoDeny) {
        $deny = Get-Command cargo-deny -ErrorAction SilentlyContinue
        if ($deny) {
            Write-Host '▶ cargo deny check' -ForegroundColor Cyan
            & $deny.Source 'check' 'advisories' 'bans' 'licenses' 'sources'
            if ($LASTEXITCODE -ne 0) {
                throw "cargo-deny exited with code $LASTEXITCODE."
            }
        }
        else {
            Write-Warning 'cargo-deny non installé — étape ignorée.'
        }
    }
    else {
        Write-Host '▶ cargo deny check skipped (--NoDeny)' -ForegroundColor Yellow
    }
}
finally {
    Pop-Location
}
