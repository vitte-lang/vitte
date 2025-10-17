[CmdletBinding()]
param(
    [switch]$SkipCargo
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    function Write-Colored {
        param([string]$Text, [ConsoleColor]$Color = [ConsoleColor]::Gray)
        $current = $Host.UI.RawUI.ForegroundColor
        $Host.UI.RawUI.ForegroundColor = $Color
        Write-Host $Text
        $Host.UI.RawUI.ForegroundColor = $current
    }

    function Get-VitteCommand {
        $cmd = Get-Command vitte -ErrorAction SilentlyContinue
        if ($cmd) { return @($cmd.Source) }
        if (Test-Path "$root/bin/bin/vitte.exe") { return @("$root/bin/bin/vitte.exe") }
        $cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1
        return @($cargo.Source, 'run', '--quiet', '--locked', '--package', 'vitte-cli', '--')
    }

    $vitteCmd = Get-VitteCommand

    $files = Get-ChildItem -Recurse -File -Include *.vit, *.vitte, *.vt |
        Where-Object { $_.FullName -notmatch '\\target\\' -and $_.FullName -notmatch '\\.git\\' } |
        Sort-Object FullName

    if (-not $files) {
        Write-Colored 'Aucun fichier .vit/.vitte trouvé — saut du contrôle syntaxe.' ([ConsoleColor]::Yellow)
    }
    else {
        Write-Colored ("▶ Analyse syntaxique des sources Vitte ({0} fichiers)" -f $files.Count) ([ConsoleColor]::Cyan)
    }

    $failed = @()
    foreach ($file in $files) {
        $args = @($file.FullName)
        $processArgs = @()
        $processArgs += $vitteCmd
        $processArgs += 'fmt'
        $processArgs += '--check'
        $processArgs += $file.FullName

        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $processArgs[0]
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.UseShellExecute = $false
        $psi.Arguments = ($processArgs[1..($processArgs.Count - 1)] -join ' ')

        $proc = New-Object System.Diagnostics.Process
        $proc.StartInfo = $psi
        $null = $proc.Start()
        $stdout = $proc.StandardOutput.ReadToEnd()
        $stderr = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()

        if ($proc.ExitCode -ne 0) {
            $failed += $file.FullName
            Write-Colored ("✖ {0}" -f $file.FullName) ([ConsoleColor]::Red)
            foreach ($line in ($stdout + $stderr -split "`r?`n" | Where-Object { $_ })) {
                if ($line -match '(?i)error:') {
                    Write-Colored "  $line" ([ConsoleColor]::Red)
                }
                elseif ($line -match '(?i)warning:') {
                    Write-Colored "  $line" ([ConsoleColor]::Yellow)
                }
                else {
                    Write-Colored "  $line" ([ConsoleColor]::Gray)
                }
            }
        }
        else {
            Write-Colored ("✔ {0}" -f $file.FullName) ([ConsoleColor]::Green)
        }
    }

    if ($failed.Count -gt 0) {
        Write-Colored ("Échec : {0} fichier(s) non conformes." -f $failed.Count) ([ConsoleColor]::Red)
        exit 1
    }

    if (-not $files) {
        # Already reported skip
    }
    else {
        Write-Colored 'Syntaxe Vitte valide.' ([ConsoleColor]::Green)
    }

    if (-not $SkipCargo) {
        $cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1
        Write-Colored '▶ cargo check --workspace --all-features --locked --message-format=short' ([ConsoleColor]::Cyan)

        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $cargo.Source
        $psi.Arguments = 'check --workspace --all-features --locked --message-format=short'
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.UseShellExecute = $false

        $proc = New-Object System.Diagnostics.Process
        $proc.StartInfo = $psi
        $null = $proc.Start()
        $stdout = $proc.StandardOutput.ReadToEnd()
        $stderr = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()

        if ($proc.ExitCode -ne 0) {
            Write-Colored 'Échec compilation Rust.' ([ConsoleColor]::Red)
            foreach ($line in ($stdout + $stderr -split "`r?`n" | Where-Object { $_ })) {
                if ($line -match '(?i)error:') {
                    Write-Colored $line ([ConsoleColor]::Red)
                }
                elseif ($line -match '(?i)warning:') {
                    Write-Colored $line ([ConsoleColor]::Yellow)
                }
                else {
                    Write-Host $line
                }
            }
            exit $proc.ExitCode
        }
        else {
            Write-Colored 'Compilation Rust OK.' ([ConsoleColor]::Green)
        }
    }
}
finally {
    Pop-Location
}
