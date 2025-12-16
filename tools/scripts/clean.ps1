# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\clean.ps1
<#
Clean workspace artifacts (build/, dist/, caches, temp files).

Usage:
  pwsh tools/scripts/clean.ps1                # safe default (build+dist+tmp)
  pwsh tools/scripts/clean.ps1 -All           # also removes caches (.cache/.ci-cache/.muffin/muffin/, rust/target)
  pwsh tools/scripts/clean.ps1 -Build         # only build/
  pwsh tools/scripts/clean.ps1 -Dist          # only dist/
  pwsh tools/scripts/clean.ps1 -Tmp           # only temp/log artifacts
  pwsh tools/scripts/clean.ps1 -DryRun        # show what would be removed

Exit codes:
  0 ok
  2 bad args
#>

[CmdletBinding()]
param(
  [switch]$Build,
  [switch]$Dist,
  [switch]$Tmp,
  [switch]$All,
  [switch]$DryRun
)

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }

function Remove-PathSafe([string]$Path) {
  if (-not (Test-Path $Path)) { return }
  if ($DryRun) {
    Write-Host ("[dry] rm -rf {0}" -f $Path)
    return
  }
  Remove-Item -Recurse -Force -LiteralPath $Path -ErrorAction SilentlyContinue
}

# If any scope switch provided, only enable those scopes.
$scoped = ($PSBoundParameters.ContainsKey("Build") -or $PSBoundParameters.ContainsKey("Dist") -or $PSBoundParameters.ContainsKey("Tmp"))
$doBuild = if ($scoped) { [bool]$Build } else { $true }
$doDist  = if ($scoped) { [bool]$Dist  } else { $true }
$doTmp   = if ($scoped) { [bool]$Tmp   } else { $true }

Write-Host ("[clean] build={0} dist={1} tmp={2} all={3} dry={4}" -f $doBuild, $doDist, $doTmp, [bool]$All, [bool]$DryRun)

if ($doBuild) { Remove-PathSafe "build" }
if ($doDist)  { Remove-PathSafe "dist" }

if ($doTmp) {
  Remove-PathSafe "checksums.sha256"
  Remove-PathSafe "compile_commands.json"
  Remove-PathSafe "Testing"
  Remove-PathSafe "CMakeFiles"
  Remove-PathSafe "CMakeCache.txt"
}

if ($All) {
  Remove-PathSafe ".cache"
  Remove-PathSafe ".ci-cache"
  Remove-PathSafe ".muffin"
  Remove-PathSafe "muffin"
  Remove-PathSafe "rust/target"
}

Write-Host "[clean] done"
exit 0
