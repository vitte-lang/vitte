$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$goldenDir = Join-Path $root 'tests\goldens\parse'
$parserWrapper = Join-Path $root 'tools\run_parser_with_diags.py'

$python = if ($env:PYTHON -and $env:PYTHON.Trim() -ne '') { $env:PYTHON } else { 'python' }
$env:PYTHONIOENCODING = if ($env:PYTHONIOENCODING) { $env:PYTHONIOENCODING } else { 'utf-8' }
$OutputEncoding = [Console]::OutputEncoding = [System.Text.Encoding]::UTF8

if (-not (Test-Path $parserWrapper)) {
  throw "[goldens] ERROR: wrapper parser manquant: $parserWrapper"
}

if (-not (Test-Path $goldenDir)) {
  Write-Host "[goldens] WARNING: pas de dossier $goldenDir, rien à faire."
  exit 0
}

$goldens = Get-ChildItem -LiteralPath $goldenDir -Filter '*.golden' -File -ErrorAction SilentlyContinue
if (-not $goldens -or $goldens.Count -eq 0) {
  Write-Host "[goldens] WARNING: aucun fichier .golden dans $goldenDir"
  exit 0
}

$status = 0
Push-Location $root
try {
  foreach ($golden in $goldens) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($golden.Name)
    $relInput = "tests/goldens/parse/$name.vitte"
    $input = Join-Path $root "tests\goldens\parse\$name.vitte"

    if (-not (Test-Path $input)) {
      Write-Host "[goldens][FAIL] ${name}: fichier d'entrée manquant ($input)"
      $status = 1
      continue
    }

    $expectedExit = $null
    $expectedStrings = New-Object System.Collections.Generic.List[string]
    foreach ($line in (Get-Content -LiteralPath $golden.FullName -Encoding UTF8)) {
      $trim = $line.TrimEnd()
      if ($trim -eq '' -or $trim.StartsWith('#')) { continue }
      if ($trim -match '^exit:([0-9]+)$') {
        $expectedExit = [int]$Matches[1]
        continue
      }
      [void]$expectedStrings.Add($trim)
    }

    $outLines = & $python $parserWrapper $relInput 2>&1
    $runRc = $LASTEXITCODE

    $fail = $false
    if ($null -ne $expectedExit -and $runRc -ne $expectedExit) {
      Write-Host "[goldens][FAIL] ${name}: exit $runRc (attendu $expectedExit)"
      $fail = $true
    }

    $outText = (($outLines | ForEach-Object { "$_" }) -join "`n") -replace '\\', '/'
    foreach ($expected in $expectedStrings) {
      if (-not $outText.Contains($expected)) {
        Write-Host "[goldens][FAIL] ${name}: manquant => $expected"
        $fail = $true
      }
    }

    if (-not $fail) {
      Write-Host "[goldens][OK] $name"
    } else {
      Write-Host "[goldens][OUTPUT] ${name}:"
      ($outLines | ForEach-Object { "    $_" }) | Write-Host
      $status = 1
    }
  }
} finally {
  Pop-Location
}

exit $status
