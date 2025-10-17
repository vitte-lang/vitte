param(
    [string]$LogDirectory = 'logs'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $root $LogDirectory
New-Item -Path $logDir -ItemType Directory -Force | Out-Null
$logFile = Join-Path $logDir ("vitte-lsp-" + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')

$cargo = Get-Command cargo -ErrorAction Stop | Select-Object -First 1
Write-Host "▶ Lancement du LSP Vitte (logs: $logFile)" -ForegroundColor Cyan

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $cargo.Source
$psi.Arguments = 'run -p vitte-lsp --features stdio --bin vitte-lsp'
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true

$proc = New-Object System.Diagnostics.Process
$proc.StartInfo = $psi
$null = $proc.Start()

$writer = New-Object System.IO.StreamWriter($logFile, $false, [System.Text.Encoding]::UTF8)

while (-not $proc.HasExited)
{
    if (-not $proc.StandardOutput.EndOfStream)
    {
        $line = $proc.StandardOutput.ReadLine()
        $writer.WriteLine($line)
        Write-Colored $line
    }
    if (-not $proc.StandardError.EndOfStream)
    {
        $line = $proc.StandardError.ReadLine()
        $writer.WriteLine($line)
        Write-Colored $line
    }
    Start-Sleep -Milliseconds 50
}

while (-not $proc.StandardOutput.EndOfStream)
{
    $line = $proc.StandardOutput.ReadLine()
    $writer.WriteLine($line)
    Write-Colored $line
}
while (-not $proc.StandardError.EndOfStream)
{
    $line = $proc.StandardError.ReadLine()
    $writer.WriteLine($line)
    Write-Colored $line
}

$writer.Flush()
$writer.Dispose()
exit $proc.ExitCode

function Write-Colored($text)
{
    if ([string]::IsNullOrEmpty($text)) { return }
    if ($text -match '(?i)error:') {
        Write-Host $text -ForegroundColor Red
    } elseif ($text -match '(?i)warning:') {
        Write-Host $text -ForegroundColor Yellow
    } elseif ($text -match '(?i)note:') {
        Write-Host $text -ForegroundColor Cyan
    } else {
        Write-Host $text
    }
}
