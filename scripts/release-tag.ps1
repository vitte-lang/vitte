param(
  [Parameter(Mandatory=$true)][string]$Version
)
$ErrorActionPreference = "Stop"
git fetch --tags
git tag -a $Version -m "release $Version"
git push origin $Version
Write-Host "✅ Tag $Version créé et poussé."
