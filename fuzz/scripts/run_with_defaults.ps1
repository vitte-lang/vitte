Param(
  [Parameter(Mandatory=$true)][string]$Target,
  [Parameter(Mandatory=$true)][string]$Bin,
  [string[]]$Extra = @()
)

$dict = ""
switch ($Target) {
  "fuzz_lexer" { $dict = "fuzz/dict/lexer_tokers.dict" }
  "fuzz_parser" { $dict = "fuzz/dict/parsergrammar.dict" }
  "fuzz_parser_recovery" { $dict = "fuzz/dict/parsergrammar.dict" }
  "fuzz_lowering" { $dict = "fuzz/dict/parsergrammar.dict" }
  "fuzz_vitte_parser" { $dict = "fuzz/dict/parsergrammar.dict" }
  default { $dict = "" }
}

if ($dict -ne "" -and (Test-Path $dict)) {
  & sh -lc "./fuzz/scripts/run.sh '$Target' '$Bin' --dict '$dict' $($Extra -join ' ')" 
  exit $LASTEXITCODE
}

& sh -lc "./fuzz/scripts/run.sh '$Target' '$Bin' $($Extra -join ' ')"
exit $LASTEXITCODE

