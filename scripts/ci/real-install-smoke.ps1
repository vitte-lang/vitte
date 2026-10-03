param(
  [string]$VitteBin = $(if ($env:VITTE_BIN) { $env:VITTE_BIN } else { "vitte" }),
  [string]$WorkDir = $env:WORKDIR
)

$ErrorActionPreference = "Stop"
$vitteCommand = Get-Command $VitteBin -ErrorAction Stop
$VitteBin = $vitteCommand.Source
$temporaryWorkDir = [string]::IsNullOrWhiteSpace($WorkDir)
if ($temporaryWorkDir) {
  $WorkDir = Join-Path ([System.IO.Path]::GetTempPath()) ("vitte-real-install-smoke-" + [guid]::NewGuid().ToString("N"))
}
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

function Invoke-Vitte {
  param([string[]]$CommandArgs)
  & $VitteBin @CommandArgs
  if ($LASTEXITCODE -ne 0) {
    throw ("Vitte command failed with exit code {0}: {1}" -f $LASTEXITCODE, ($CommandArgs -join ' '))
  }
}

$source = @"
proc main() -> int {
  give 0;
}
"@
$smoke = Join-Path $WorkDir "smoke.vit"
Set-Content -Path $smoke -Value $source -Encoding ASCII

Push-Location $WorkDir
try {
  # Required post-install contract:
  $doctor = Join-Path (Split-Path -Parent $VitteBin) "vitte-installer-doctor.cmd"
  if (Test-Path $doctor) {
    & $doctor
    if ($LASTEXITCODE -ne 0) { throw "Installer doctor failed with exit code $LASTEXITCODE" }
  }
  Invoke-Vitte -CommandArgs @("--version")
  Invoke-Vitte -CommandArgs @("--help")
  Invoke-Vitte -CommandArgs @("check", "smoke.vit")
  Invoke-Vitte -CommandArgs @("compile", "smoke.vit", "-o", "smoke")
  $smokeProgram = @(".\smoke.exe", ".\smoke") | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $smokeProgram) {
    throw "Vitte produced no executable in $WorkDir"
  }
  & $smokeProgram
  if ($LASTEXITCODE -ne 0) { throw "Vitte smoke program failed with exit code $LASTEXITCODE" }
  Write-Host "[real-install-smoke] OK bin=$VitteBin workdir=$WorkDir"
} finally {
  Pop-Location
  if ($temporaryWorkDir) {
    Remove-Item -LiteralPath $WorkDir -Recurse -Force
  }
}
