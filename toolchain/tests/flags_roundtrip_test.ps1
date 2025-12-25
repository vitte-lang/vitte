<#
toolchain/tests/flags_roundtrip_test.ps1

Flags roundtrip test for drivers using diagnostic mode (--dry-run).

Verifies that common driver flags are forwarded into the generated clang argv:
  --target, --sysroot, -L, -l, -shared

This test does not compile or link; it only inspects printed argv.
#>

[CmdletBinding(PositionalBinding = $false)]
param(
  [string]$Target = "x86_64-unknown-linux-gnu"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Need([string]$Cmd) {
  $c = Get-Command $Cmd -ErrorAction SilentlyContinue
  if (-not $c) { throw "missing command: $Cmd" }
}

function Assert-Match([string]$Text, [string]$Pattern, [string]$What) {
  if ($Text -notmatch $Pattern) {
    throw "assert failed: missing $What (pattern: $Pattern)`noutput:`n$Text"
  }
}

Need "vitte-cc"
Need "vitte-ld"

$tmp = Join-Path ([IO.Path]::GetTempPath()) ("vitte_flags_roundtrip_{0}" -f ([Guid]::NewGuid().ToString("N")))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

try {
  $sysroot = Join-Path $tmp "sysroot"
  $libdir = Join-Path $tmp "lib"
  New-Item -ItemType Directory -Force -Path $sysroot | Out-Null
  New-Item -ItemType Directory -Force -Path $libdir | Out-Null

  $cSrc = Join-Path $tmp "t.c"
  "int main(void){return 0;}" | Set-Content -LiteralPath $cSrc -NoNewline

  $obj = Join-Path $tmp "t.o"
  "obj" | Set-Content -LiteralPath $obj -NoNewline

  $outCompile = & vitte-cc --dry-run --target $Target --sysroot $sysroot -c $cSrc -o (Join-Path $tmp "t2.o") 2>&1
  $outCompile = [string]$outCompile
  Assert-Match $outCompile ([regex]::Escape($Target)) "--target triple"
  Assert-Match $outCompile ([regex]::Escape($sysroot)) "--sysroot path"

  $outLinkCc = & vitte-cc --dry-run --target $Target --sysroot $sysroot -shared -L $libdir -l m $obj -o (Join-Path $tmp "a.dll") 2>&1
  $outLinkCc = [string]$outLinkCc
  Assert-Match $outLinkCc ([regex]::Escape($Target)) "--target triple"
  Assert-Match $outLinkCc ([regex]::Escape($sysroot)) "--sysroot path"
  Assert-Match $outLinkCc ("-L\\s*" + [regex]::Escape($libdir)) "-L forwarding"
  Assert-Match $outLinkCc ("\\s-l\\s*m(\\s|$)|\\s-lm(\\s|$)") "-l forwarding"
  Assert-Match $outLinkCc ("\\s-shared(\\s|$)") "-shared forwarding"

  $outLinkLd = & vitte-ld --dry-run --target $Target --sysroot $sysroot -shared -L $libdir -l m $obj -o (Join-Path $tmp "b.dll") 2>&1
  $outLinkLd = [string]$outLinkLd
  Assert-Match $outLinkLd ([regex]::Escape($Target)) "--target triple"
  Assert-Match $outLinkLd ([regex]::Escape($sysroot)) "--sysroot path"
  Assert-Match $outLinkLd ("-L\\s*" + [regex]::Escape($libdir)) "-L forwarding"
  Assert-Match $outLinkLd ("\\s-l\\s*m(\\s|$)|\\s-lm(\\s|$)") "-l forwarding"
  Assert-Match $outLinkLd ("\\s-shared(\\s|$)") "-shared forwarding"

  Write-Host "ok"
} finally {
  if (Test-Path -LiteralPath $tmp) { Remove-Item -LiteralPath $tmp -Force -Recurse }
}

