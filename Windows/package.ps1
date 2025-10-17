[CmdletBinding()]
param(
    [string]$Version,
    [string[]]$Target = @('x86_64-pc-windows-msvc', 'aarch64-pc-windows-msvc'),
    [string]$OutputDir = 'Windows/dist',
    [switch]$NoBuild,
    [switch]$KeepWorkDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1

    if (-not $Version) {
        $metadata = & $cargo.Source 'metadata' '--format-version' '1' '--no-deps' | ConvertFrom-Json
        $pkg = $metadata.packages | Where-Object { $_.name -eq 'vitte-cli' } | Select-Object -First 1
        if (-not $pkg) { $pkg = $metadata.packages | Where-Object { $_.name -eq 'vitte-bin' } | Select-Object -First 1 }
        if (-not $pkg) { throw 'Impossible de déterminer la version via cargo metadata.' }
        $Version = $pkg.version
    }

    $resolvedOutput = $null
    try {
        $resolvedOutput = (Resolve-Path -LiteralPath $OutputDir -ErrorAction Stop).Path
    }
    catch {
        $resolvedOutput = (New-Item -Path $OutputDir -ItemType Directory -Force).FullName
    }
    $OutputDir = $resolvedOutput

    $workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("vitte-win-" + [Guid]::NewGuid())
    New-Item -Path $workDir -ItemType Directory -Force | Out-Null

    function Get-ShortArch {
        param([string]$Triple)
        switch -Wildcard ($Triple) {
            'x86_64-*' { 'win-x64'; break }
            'aarch64-*' { 'win-arm64'; break }
            'i686-*' { 'win-x86'; break }
            default { $Triple.Replace('pc-', '').Replace('-msvc', '') }
        }
    }

    foreach ($triple in $Target) {
        if (-not $NoBuild) {
            Write-Host "▶ cargo build --release --target $triple --locked" -ForegroundColor Cyan
            & $cargo.Source 'build' '--release' '--target' $triple '--locked'
            if ($LASTEXITCODE -ne 0) {
                throw "cargo build failed for target $triple."
            }
        }

        $binaryPath = Join-Path $root "target/$triple/release/vitte.exe"
        if (-not (Test-Path $binaryPath)) {
            throw "Binaire introuvable pour $triple : $binaryPath"
        }

        $short = Get-ShortArch $triple
        $stageDir = Join-Path $workDir "vitte-$Version-$short"
        New-Item -Path $stageDir -ItemType Directory -Force | Out-Null

        Copy-Item $binaryPath (Join-Path $stageDir 'vitte.exe') -Force
        Copy-Item 'README.md' (Join-Path $stageDir 'README.md') -Force
        Copy-Item 'LICENSE' (Join-Path $stageDir 'LICENSE') -Force

        $archive = Join-Path $OutputDir "vitte-$Version-$short.zip"
        if (Test-Path $archive) { Remove-Item $archive -Force }
        Write-Host "▶ Compress-Archive $stageDir -> $archive" -ForegroundColor Cyan
        Compress-Archive -Path (Join-Path $stageDir '*') -DestinationPath $archive -Force
    }

    Write-Host "✅ Archives générées dans $OutputDir" -ForegroundColor Green

    if ($KeepWorkDir) {
        Write-Host "Dossier de travail conservé: $workDir" -ForegroundColor Yellow
    }
    else {
        Remove-Item -Recurse -Force $workDir
    }
}
finally {
    Pop-Location
}
