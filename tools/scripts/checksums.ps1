# tools/scripts/checksums.ps1
<#
Compute and verify SHA-256 checksums for the repo (build-critical files).
Produces: checksums.sha256

Usage:
  pwsh tools/scripts/checksums.ps1                 # write/update checksums.sha256
  pwsh tools/scripts/checksums.ps1 -Verify         # verify checksums.sha256
  pwsh tools/scripts/checksums.ps1 -All            # include more file types
  pwsh tools/scripts/checksums.ps1 -Out foo.sha256

Exit codes:
  0 ok
  1 usage / missing files
  2 checksum mismatch / verification failed
#>

[CmdletBinding()]
param(
  [switch]$Verify,
  [switch]$All,
  [string]$Out = "checksums.sha256"
)

function Fail([string]$Msg, [int]$Code) {
  Write-Error $Msg
  exit $Code
}

function Get-FileList {
  if ($All) {
    Get-ChildItem -Recurse -File -Force |
      Where-Object { $_.FullName -notmatch '\\build\\' -and $_.FullName -notmatch '\\.git\\' } |
      ForEach-Object { $_.FullName } |
      Sort-Object
  } else {
    # Build-critical + repo meta; mirrors the .sh selection logic.
    $patterns = @(
      ".editorconfig",
      "CMakeLists.txt",
      "Makefile",
      "LICENSE",
      "VERSION",
      "muffin.muf",
      "muffin.lock"
    )

    $list = New-Object System.Collections.Generic.List[string]

    foreach ($p in $patterns) {
      Get-ChildItem -Path "." -Recurse -File -Force -Filter $p -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -notmatch '\\build\\' -and $_.FullName -notmatch '\\.git\\' } |
        ForEach-Object { $list.Add($_.FullName) } | Out-Null
    }

    foreach ($dir in @("tools\scripts","compiler","runtime","std")) {
      if (Test-Path $dir) {
        Get-ChildItem -Path $dir -Recurse -File -Force -ErrorAction SilentlyContinue |
          Where-Object { $_.FullName -notmatch '\\build\\' -and $_.FullName -notmatch '\\.git\\' } |
          ForEach-Object { $list.Add($_.FullName) } | Out-Null
      }
    }

    $list | Sort-Object -Unique
  }
}

function Get-RelPath([string]$FullPath) {
  $root = (Get-Location).Path.TrimEnd('\')
  $p = $FullPath
  if ($p.StartsWith($root)) {
    $p = $p.Substring($root.Length).TrimStart('\')
  }
  # Normalize to forward slashes for determinism
  return ($p -replace '\\','/')
}

function Write-Checksums {
  $tmp = "$Out.tmp.$PID"
  "# SHA-256 checksums (generated)" | Out-File -FilePath $tmp -Encoding utf8
  "# format: <sha256>␠␠<path>"     | Out-File -FilePath $tmp -Encoding utf8 -Append

  $files = Get-FileList
  foreach ($f in $files) {
    if (-not (Test-Path $f)) { continue }
    $h = (Get-FileHash -Algorithm SHA256 -Path $f).Hash.ToLowerInvariant()
    $rel = Get-RelPath $f
    "$h  $rel" | Out-File -FilePath $tmp -Encoding utf8 -Append
  }

  Move-Item -Force $tmp $Out
  Write-Host ("[write] {0}" -f $Out)
  exit 0
}

function Verify-Checksums {
  if (-not (Test-Path $Out)) {
    Fail "Missing $Out (run without -Verify first)." 1
  }

  $fail = $false
  $lines = Get-Content -Path $Out -Encoding utf8
  foreach ($line in $lines) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    if ($line.TrimStart().StartsWith("#")) { continue }

    # Expect: "<hash>  <path>"
    if ($line -notmatch '^(?<h>[0-9a-fA-F]{64})\s{2,}(?<p>.+)$') { continue }
    $expected = $Matches['h'].ToLowerInvariant()
    $path = $Matches['p']

    if (-not (Test-Path $path)) {
      Write-Error ("[missing] {0}" -f $path)
      $fail = $true
      continue
    }

    $got = (Get-FileHash -Algorithm SHA256 -Path $path).Hash.ToLowerInvariant()
    if ($got -ne $expected) {
      Write-Error ("[mismatch] {0}`n  expected={1}`n  got     ={2}" -f $path, $expected, $got)
      $fail = $true
    }
  }

  if ($fail) { exit 2 }
  Write-Host "[verify] OK"
  exit 0
}

if ($Verify) { Verify-Checksums } else { Write-Checksums }
