$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$distDir = Join-Path $root 'dist'
New-Item -ItemType Directory -Force -Path $distDir | Out-Null

$version = '0.1.0'
$releaseBin = Join-Path $root 'target\release\vittec'
if (-not (Test-Path $releaseBin)) {
  throw "[vitte][dist] Missing release binary at $releaseBin (run stage2/self-host first)."
}

$zipName = "vittec-$version-windows.zip"
$zipPath = Join-Path $distDir $zipName
if (Test-Path $zipPath) { Remove-Item -Force -LiteralPath $zipPath }

Compress-Archive -Path $releaseBin -DestinationPath $zipPath

$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $zipPath).Hash.ToLowerInvariant()
$hashPath = "$zipPath.sha256"
"$hash  $zipName" | Set-Content -Encoding ASCII -LiteralPath $hashPath

Write-Host "Dist created: $zipPath"
