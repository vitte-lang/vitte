param()
$ErrorActionPreference = "Stop"

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

$strictWarnings = $true
if ($env:VITTE_STRICT_WARNINGS) {
    switch ($env:VITTE_STRICT_WARNINGS.ToString().ToLowerInvariant()) {
        '0' { $strictWarnings = $false }
        'false' { $strictWarnings = $false }
        'no' { $strictWarnings = $false }
        'off' { $strictWarnings = $false }
    }
}

if ($strictWarnings) {
    Add-UniqueFlag -Name 'RUSTFLAGS' -Flag '-Dwarnings'
    Add-UniqueFlag -Name 'RUSTDOCFLAGS' -Flag '-Dwarnings'
    if (-not $env:VITTE_STRICT_WARNINGS_EMITTED) {
        Write-Host "▶ mode strict (warnings → erreurs)"
    }
}
else {
    if (-not $env:VITTE_STRICT_WARNINGS_EMITTED) {
        Write-Warning "mode strict désactivé (VITTE_STRICT_WARNINGS)"
    }
}

Write-Host "▶ rustfmt --check"
$fmtSucceeded = $false
try {
    cargo fmt --all -- --check
    if ($LASTEXITCODE -eq 0) { $fmtSucceeded = $true }
}
catch {}
if (-not $fmtSucceeded) {
    if (Get-Command rustup -ErrorAction SilentlyContinue) {
        $toolchains = (rustup toolchain list) -join "\n"
        if ($toolchains -match 'nightly') {
            Write-Warning "rustfmt stable a échoué; tentative avec nightly"
            & cargo +nightly fmt --all -- --check
            if ($LASTEXITCODE -ne 0) { throw "rustfmt nightly a échoué." }
        }
        else {
            Write-Warning "rustfmt a échoué et nightly introuvable. Installe-le: rustup toolchain install nightly"
            throw "rustfmt stable a échoué."
        }
    }
    else {
        throw "rustfmt non disponible via rustup."
    }
}

Write-Host "▶ clippy (all features)"
cargo clippy --workspace --all-features -- -D warnings

Write-Host "▶ tests (all features)"
cargo test --workspace --all-features

if (Get-Command cargo-deny -ErrorAction SilentlyContinue) {
  Write-Host "▶ cargo-deny"
  cargo deny check advisories bans licenses sources
} else {
  Write-Warning "cargo-deny non installé — skip (install: cargo install cargo-deny)"
}

Write-Host "▶ docs (no-deps)"
cargo doc --workspace --all-features --no-deps

Write-Host "▶ architecture lint"
$archLint = Join-Path $PSScriptRoot 'pro/arch-lint.py'
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
  $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
  throw "python interpreter not found (required for arch-lint)."
}
& $python.Source $archLint
if ($LASTEXITCODE -ne 0) {
  throw "arch-lint exited with code $LASTEXITCODE."
}

Write-Host "✅ check OK"
