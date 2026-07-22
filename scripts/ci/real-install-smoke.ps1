param(
  [string]$VitteBin = $(if ($env:VITTE_BIN) { $env:VITTE_BIN } else { "vitte" }),
  [string]$WorkDir = $(if ($env:WORKDIR) { $env:WORKDIR } else { Join-Path $env:TEMP "vitte-real-install-smoke" })
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$source = @"
proc main() -> int {
  give 0
}
"@
$smoke = Join-Path $WorkDir "smoke.vit"
Set-Content -Path $smoke -Value $source -Encoding ASCII

Push-Location $WorkDir
try {
  # Required post-install contract:
  & $VitteBin --help | Out-Null
  & $VitteBin check smoke.vit
  & $VitteBin build smoke.vit -o smoke
  if (Test-Path ".\smoke.exe") {
    & ".\smoke.exe"
  }
  Write-Host "[real-install-smoke] OK bin=$VitteBin workdir=$WorkDir"
} finally {
  Pop-Location
}
