Param(
  [Parameter(Mandatory=$true)][string]$Target,
  [Parameter(Mandatory=$true)][string]$Bin,
  [string[]]$Extra = @()
)

function Quote-Sh([string]$s) {
  return "'" + ($s -replace "'", "'\"'\"'") + "'"
}

$dict = (& python fuzz/scripts/target_map.py dict $Target).Trim()

if ($dict -ne "" -and (Test-Path $dict)) {
  $targetQ = Quote-Sh $Target
  $binQ = Quote-Sh $Bin
  $dictQ = Quote-Sh $dict
  $extraQ = ($Extra | ForEach-Object { Quote-Sh $_ }) -join " "
  & sh -lc "./fuzz/scripts/run.sh $targetQ $binQ --dict $dictQ $extraQ"
  exit $LASTEXITCODE
}

$targetQ = Quote-Sh $Target
$binQ = Quote-Sh $Bin
$extraQ = ($Extra | ForEach-Object { Quote-Sh $_ }) -join " "
& sh -lc "./fuzz/scripts/run.sh $targetQ $binQ $extraQ"
exit $LASTEXITCODE
