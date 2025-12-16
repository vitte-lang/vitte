# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\ci_fail_fast.ps1
<#
CI fail-fast runner for Vitte.

Runs a sequence of steps and stops immediately on failure, with clear log headers.
Also supports "optional" steps that warn but don't fail the job.

Typical use in CI:
  pwsh tools/scripts/ci_fail_fast.ps1 -Steps @(
    @{ Name="env";   Cmd="pwsh tools/scripts/ci_env.ps1 -PrepareDirs" },
    @{ Name="build"; Cmd="pwsh tools/scripts/build_all.ps1 -Mode All -Tests Off" },
    @{ Name="test";  Cmd="pwsh tools/scripts/build_once.ps1 -Mode Debug -Tests On" }
  )

Usage:
  pwsh tools/scripts/ci_fail_fast.ps1 -Steps (Get-Content .ci/steps.json | ConvertFrom-Json)

Exit codes:
  0 ok
  2 bad args
  <step exit code> first failing step's exit code
#>

[CmdletBinding()]
param(
  # Array of hashtables/PSCustomObjects:
  #   @{ Name="build"; Cmd="pwsh tools/scripts/build_all.ps1 -Mode All" ; Optional=$false ; TimeoutSec=0 }
  [Parameter(Mandatory=$true)]
  [object[]]$Steps,

  # Print commands before execution.
  [switch]$Echo,

  # If set, still runs all steps and reports failures at end (not fail-fast).
  [switch]$NoFailFast
)

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }

function Banner([string]$Name) {
  $line = ("=" * 78)
  Write-Host ""
  Write-Host $line
  Write-Host ("[step] {0}" -f $Name)
  Write-Host $line
}

function Run-Step($step) {
  $name = [string]($step.Name)
  $cmd  = [string]($step.Cmd)

  if ([string]::IsNullOrWhiteSpace($name) -or [string]::IsNullOrWhiteSpace($cmd)) {
    Fail "Each step must have Name and Cmd." 2
  }

  $optional = $false
  if ($null -ne $step.Optional) { $optional = [bool]$step.Optional }

  $timeout = 0
  if ($null -ne $step.TimeoutSec) { $timeout = [int]$step.TimeoutSec }

  Banner $name

  if ($Echo) { Write-Host ("[cmd] {0}" -f $cmd) }

  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = if ($IsWindows) { "pwsh" } else { "pwsh" }
  $psi.Arguments = "-NoProfile -NonInteractive -Command $cmd"
  $psi.RedirectStandardOutput = $false
  $psi.RedirectStandardError  = $false
  $psi.UseShellExecute = $true

  $p = New-Object System.Diagnostics.Process
  $p.StartInfo = $psi

  $null = $p.Start()

  if ($timeout -gt 0) {
    if (-not $p.WaitForExit($timeout * 1000)) {
      try { $p.Kill($true) } catch {}
      if ($optional) {
        Write-Warning ("[timeout][optional] {0} after {1}s" -f $name, $timeout)
        return @{ Name=$name; Code=124; Optional=$true; Status="timeout" }
      }
      Write-Error ("[timeout] {0} after {1}s" -f $name, $timeout)
      exit 124
    }
  } else {
    $p.WaitForExit() | Out-Null
  }

  $code = $p.ExitCode
  if ($code -eq 0) {
    Write-Host ("[ok] {0}" -f $name)
    return @{ Name=$name; Code=0; Optional=$optional; Status="ok" }
  }

  if ($optional) {
    Write-Warning ("[fail][optional] {0} exit={1}" -f $name, $code)
    return @{ Name=$name; Code=$code; Optional=$true; Status="failed" }
  }

  Write-Error ("[fail] {0} exit={1}" -f $name, $code)
  exit $code
}

if (-not $Steps -or $Steps.Count -eq 0) {
  Fail "Steps is required and must be non-empty." 2
}

$results = @()
$failed = $false

foreach ($s in $Steps) {
  $r = Run-Step $s
  $results += $r

  if ($NoFailFast -and $r.Code -ne 0 -and -not $r.Optional) {
    $failed = $true
  }
}

if ($NoFailFast) {
  Write-Host ""
  Write-Host ("[summary] steps={0}" -f $results.Count)
  foreach ($r in $results) {
    $tag = if ($r.Code -eq 0) { "OK" } elseif ($r.Optional) { "WARN" } else { "FAIL" }
    Write-Host ("  {0,-4} {1,-20} code={2}" -f $tag, $r.Name, $r.Code)
  }
  if ($failed) { exit 2 }
}

exit 0
