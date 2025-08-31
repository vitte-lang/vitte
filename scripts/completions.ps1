param()
$ErrorActionPreference = "Stop"
$bins = @("vitte","vitte-fmt","vitte-check","vitte-pack","vitte-dump","vitte-graph","vitte-run")
foreach ($b in $bins) {
  if (Get-Command $b -ErrorAction SilentlyContinue) {
    Write-Host "▶ $b completions --install"
    & $b completions --install | Out-Null
  } else {
    Write-Host "• $b introuvable — skip"
  }
}
Write-Host "ℹ️ Voir docs/COMPLETIONS.md pour l'installation manuelle."
