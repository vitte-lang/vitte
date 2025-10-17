[CmdletBinding()]
param(
    [switch]$KeepLogs,
    [switch]$SkipLint,
    [switch]$SkipTests,
    [switch]$SkipDocs,
    [switch]$SkipBuild,
    [string]$PipelineTarget
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    $logDir = Join-Path ([System.IO.Path]::GetTempPath()) ("vitte-pipeline-" + [Guid]::NewGuid())
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null

    function Write-Banner {
        param([string]$Text, [ConsoleColor]$Color = [ConsoleColor]::Cyan)
        $line = "========== $Text =========="
        $current = $Host.UI.RawUI.ForegroundColor
        $Host.UI.RawUI.ForegroundColor = $Color
        Write-Host ""
        Write-Host $line
        $Host.UI.RawUI.ForegroundColor = $current
    }

    function Show-Tail {
        param([string]$Content)
        $lines = $Content -split "`r?`n"
        if ($lines.Count -gt 40) {
            $lines = $lines[-40..-1]
        }
        foreach ($line in $lines) {
            if ($line -match '(?i)\berror:?') {
                Write-Host $line -ForegroundColor Red
            }
            elseif ($line -match '(?i)\bwarning:?') {
                Write-Host $line -ForegroundColor Yellow
            }
            else {
                Write-Host $line
            }
        }
    }

    function Invoke-Stage {
        param(
            [string]$Name,
            [scriptblock]$Command
        )
        $logFile = Join-Path $logDir ($Name.ToLower().Replace(' ', '_') + ".log")
        Write-Banner $Name
        try {
            $output = & $Command 2>&1
            $text = ($output | ForEach-Object { $_.ToString().TrimEnd() }) -join [Environment]::NewLine
            Set-Content -Path $logFile -Value $text -Encoding UTF8
            if ($LASTEXITCODE -ne 0) {
                throw "Exit code $LASTEXITCODE"
            }
            Write-Host "[OK] $Name" -ForegroundColor Green
        }
        catch {
            $text = if (Test-Path $logFile) { Get-Content $logFile -Raw } else { '' }
            Write-Host "[ÉCHEC] $Name" -ForegroundColor Red
            if (-not [string]::IsNullOrWhiteSpace($text)) {
                Show-Tail $text
            }
            else {
                Write-Host $_ -ForegroundColor Red
            }
            Write-Host ("Journal : {0}" -f $logFile) -ForegroundColor Yellow
            Write-Host "Pipeline interrompu." -ForegroundColor Red
            if (-not $KeepLogs) { Remove-Item -Recurse -Force $logDir }
            exit 1
        }
    }

    $scriptsRoot = $PSScriptRoot

    if (-not $SkipLint) {
        Invoke-Stage -Name 'Lint' -Command {
            & "$scriptsRoot/lint.ps1"
        }
    }
    else {
        Write-Host "Skipping Lint stage." -ForegroundColor Yellow
    }

    if (-not $SkipTests) {
        Invoke-Stage -Name 'Tests' -Command {
            & "$scriptsRoot/test.ps1" -Workspace -AllFeatures
        }
    }
    else {
        Write-Host "Skipping Tests stage." -ForegroundColor Yellow
    }

    if (-not $SkipDocs) {
        Invoke-Stage -Name 'Docs' -Command {
            & (Get-Command cargo).Source 'doc' '--workspace' '--all-features' '--no-deps'
        }
    }
    else {
        Write-Host "Skipping Docs stage." -ForegroundColor Yellow
    }

    if (-not $SkipBuild) {
        Invoke-Stage -Name 'Build' -Command {
            & "$scriptsRoot/build.ps1" -Workspace -AllFeatures -Release -Locked
        }
    }
    else {
        Write-Host "Skipping Build stage." -ForegroundColor Yellow
    }

    if ($PipelineTarget) {
        Invoke-Stage -Name 'Cross Build' -Command {
            & (Get-Command cargo).Source 'build' '--release' '--target' $PipelineTarget '--locked'
        }
    }

    Write-Banner "Pipeline terminée" -Color ([ConsoleColor]::Green)
    Write-Host "Tous les stages ont réussi." -ForegroundColor Green

    if ($KeepLogs) {
        Write-Host ("Logs conservés : {0}" -f $logDir) -ForegroundColor Cyan
    }
    else {
        Remove-Item -Recurse -Force $logDir
    }
}
finally {
    Pop-Location
}
