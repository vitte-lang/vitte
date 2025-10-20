[CmdletBinding()]
param(
    [switch]$Install,
    [switch]$SkipNode,
    [switch]$SkipWasm
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-EnvSwitch {
    param([string]$Value)
    if (-not $Value) { return $false }
    $normalized = $Value.ToString().ToLowerInvariant()
    return $normalized -match '^(1|true|yes|on)$'
}

if (-not $Install -and (Test-EnvSwitch $env:INSTALL)) { $Install = $true }
if (-not $SkipNode -and (Test-EnvSwitch $env:SKIP_NODE)) { $SkipNode = $true }
if (-not $SkipWasm -and (Test-EnvSwitch $env:SKIP_WASM)) { $SkipWasm = $true }

function Write-Info {
    param([string]$Message)
    Write-Host "▶ $Message" -ForegroundColor Cyan
}

function Write-Ok {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

$script:criticalMissing = $false
$script:warnings = $false

function Test-Cmd {
    param([string]$Name)
    return (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-Cmd {
    param(
        [string]$Name,
        [string]$Label = $Name,
        [switch]$Critical
    )
    if (Test-Cmd $Name) {
        Write-Ok "$Label présent"
        return $true
    }
    Write-Warning "$Label manquant"
    $script:warnings = $true
    if ($Critical) {
        $script:criticalMissing = $true
    }
    return $false
}

function Get-TomlList {
    param(
        [string]$FilePath,
        [string]$Key
    )

    if (-not (Test-Path $FilePath)) {
        return @()
    }

    $capture = $false
    $items = @()
    foreach ($line in Get-Content $FilePath) {
        if (-not $capture -and $line -match "^\s*$Key\s*=") {
            $capture = $true
            continue
        }
        if ($capture) {
            if ($line -match '\]') {
                break
            }
            foreach ($match in [regex]::Matches($line, '"([^"]+)"')) {
                $items += $match.Groups[1].Value
            }
        }
    }
    return $items
}

function Ensure-OptionalTool {
    param(
        [string]$Command,
        [string]$Label,
        [string]$InstallCommand,
        [switch]$Critical
    )
    if (Test-Cmd $Command) {
        Write-Ok "$Label présent"
        return
    }
    Write-Warning "$Label manquant"
    $script:warnings = $true
    if ($Install -and $InstallCommand) {
        Write-Info "Installation de $Label via: $InstallCommand"
        try {
            Invoke-Expression $InstallCommand
            if (Test-Cmd $Command) {
                Write-Ok "$Label installé"
                return
            }
        }
        catch {
            Write-Warning "Échec de l'installation pour $Label : $($_.Exception.Message)"
        }
    }
    if ($Critical) {
        $script:criticalMissing = $true
    }
}

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    if (Ensure-Cmd -Name 'rustup' -Critical) {
        $toolchainOutput = & rustup show active-toolchain
        $toolchainName = if ($toolchainOutput) { ($toolchainOutput -split '\s+')[0] } else { 'stable' }
        if (-not $toolchainName) {
            $toolchainName = 'stable'
        }
        Write-Info ("Toolchain active: {0}" -f $toolchainName)
        if ($Install) {
            $listed = (rustup toolchain list) -join "\n"
            if ($listed -notmatch '^nightly') {
                Write-Info 'Installation de la toolchain nightly (pour rustfmt avancé)'
                try {
                    & rustup toolchain install nightly -c rustfmt | Out-Null
                    Write-Ok 'nightly installé (avec rustfmt)'
                }
                catch {
                    Write-Warning "Impossible d'installer nightly : $($_.Exception.Message)"
                    $script:warnings = $true
                }
            }
        }
    }
    else {
        $toolchainName = 'stable'
    }

    if (-not (Ensure-Cmd -Name 'cargo' -Critical)) {
        # nothing else to do if cargo missing
    }
    if (-not (Ensure-Cmd -Name 'rustc' -Critical)) {
        # nothing else to do if rustc missing
    }

    $toolchainFile = Join-Path $root 'rust-toolchain.toml'
    if (Test-Path $toolchainFile -and (Test-Cmd rustup)) {
        $components = Get-TomlList -FilePath $toolchainFile -Key 'components'
        if ($components.Count -gt 0) {
            Write-Info "Vérification des composants rustup requis…"
            foreach ($component in $components) {
                $installed = & rustup component list --toolchain $toolchainName --installed | Select-String -Pattern $component
                if ($installed) {
                    Write-Ok ("component {0}" -f $component)
                }
                else {
                    Write-Warning ("component {0} manquant" -f $component)
                    $script:criticalMissing = $true
                    if ($Install) {
                        Write-Info ("Installation du composant {0}" -f $component)
                        try {
                            & rustup component add --toolchain $toolchainName $component | Out-Null
                            Write-Ok ("component {0} installé" -f $component)
                        }
                        catch {
                            Write-Warning ("Échec d'installation pour {0}: {1}" -f $component, $_.Exception.Message)
                            $script:warnings = $true
                        }
                    }
                }
            }
        }

        $targets = Get-TomlList -FilePath $toolchainFile -Key 'targets'
        if ($targets.Count -gt 0) {
            Write-Info "Vérification des cibles rustup déclarées…"
            $installedTargets = & rustup target list --toolchain $toolchainName --installed | ForEach-Object {
                ($_ -split '\s+')[0]
            }
            $missingTargets = @()
            foreach ($target in $targets) {
                if ($installedTargets -contains $target) {
                    Write-Ok ("target {0}" -f $target)
                }
                else {
                    Write-Warning ("target {0} manquante" -f $target)
                    $missingTargets += $target
                }
            }
            if ($missingTargets.Count -gt 0) {
                $script:warnings = $true
                if ($Install) {
                    Write-Info ("Installation des cibles manquantes: {0}" -f ($missingTargets -join ', '))
                    try {
                        & rustup target add --toolchain $toolchainName @missingTargets | Out-Null
                        Write-Ok "Cibles installées"
                    }
                    catch {
                        Write-Warning ("Impossible d'installer toutes les cibles: {0}" -f $_.Exception.Message)
                    }
                }
                else {
                    Write-Warning ("Ajoute les cibles avec: rustup target add {0}" -f ($missingTargets -join ' '))
                }
            }
        }
    }

    Write-Info "Vérification des outils cargo optionnels…"
    Ensure-OptionalTool -Command 'cargo-deny' -Label 'cargo-deny' -InstallCommand 'cargo install cargo-deny --locked'
    Ensure-OptionalTool -Command 'cargo-nextest' -Label 'cargo-nextest' -InstallCommand 'cargo install cargo-nextest --locked'
    if (-not $SkipWasm) {
        Ensure-OptionalTool -Command 'wasm-pack' -Label 'wasm-pack' -InstallCommand 'cargo install wasm-pack'
    }
    else {
        Write-Info "wasm-pack ignoré (--skip-wasm)"
    }
    Ensure-OptionalTool -Command 'just' -Label 'just' -InstallCommand 'cargo install just --locked'

    if (-not $SkipNode) {
        Write-Info "Vérification Node.js / npm…"
        if (-not (Ensure-Cmd -Name 'node' -Label 'node')) {
            Write-Warning "Node.js est requis pour les outils web (installe via https://nodejs.org/)."
        }
        if (-not (Ensure-Cmd -Name 'npm' -Label 'npm')) {
            Write-Warning "npm est requis pour les outils web."
        }
    }
    else {
        Write-Info "Node.js / npm ignorés (--skip-node)"
    }
}
finally {
    Pop-Location
}

if ($script:criticalMissing) {
    Write-Warning "Bootstrap terminé avec erreurs critiques. Corrige les éléments listés ci-dessus."
    exit 1
}

if ($script:warnings) {
    Write-Warning "Bootstrap terminé avec avertissements (voir ci-dessus)."
}
else {
    Write-Ok "Environnement prêt."
}
