# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\debug_run.ps1
<#
Debug run helper for Vitte (runs the Debug build of vittec with args).

Behavior:
  - ensures Debug build exists (CMake configure+build if missing or -Rebuild)
  - runs vittec with provided arguments
  - optional: runs under a debugger (best-effort):
      * Windows: vsdbg (if installed) or just run
      * macOS/Linux: lldb or gdb if present

Usage:
  pwsh tools/scripts/debug_run.ps1 -- --help
  pwsh tools/scripts/debug_run.ps1 -Args "--help"
  pwsh tools/scripts/debug_run.ps1 -Rebuild -Args "compile examples/hello.vitte"
  pwsh tools/scripts/debug_run.ps1 -Debugger lldb -Args "--version"

Exit codes:
  0 ok
  2 configure/build failed
  3 run failed
#>

[CmdletBinding()]
param(
    # If set, always rebuild Debug before running.
    [switch]$Rebuild,

    # Build root / debug dir
    [string]$BuildDir = "build/debug",

    # Binary name
    [string]$BinName = "vittec",

    # Args passed to the binary as a single string (simple split) or use trailing `--` passthrough.
    [string]$Args = "",

    # Debugger preference: none|lldb|gdb|vsdbg
    [ValidateSet("none", "lldb", "gdb", "vsdbg")]
    [string]$Debugger = "none"
)

# Passthrough args after `--`
if ($Args -eq "" -and $MyInvocation.UnboundArguments.Count -gt 0) {
    $Args = ($MyInvocation.UnboundArguments -join " ")
}

function Fail([string]$Msg, [int]$Code) { Write-Error $Msg; exit $Code }

function Has([string]$Cmd) { return [bool](Get-Command $Cmd -ErrorAction SilentlyContinue) }

function Ensure-Build {
    if (-not (Has "cmake")) { Fail "cmake not found in PATH." 2 }

    if ($Rebuild -and (Test-Path $BuildDir)) {
        Remove-Item -Recurse -Force -Path $BuildDir -ErrorAction SilentlyContinue
    }

    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    }

    # Configure if CMakeCache is missing
    $cache = Join-Path $BuildDir "CMakeCache.txt"
    if ($Rebuild -or -not (Test-Path $cache)) {
        $gen = @()
        if (Has "ninja") { $gen = @("-G", "Ninja") }

        & cmake -S . -B $BuildDir @gen `
            -DCMAKE_BUILD_TYPE=Debug `
            -DVITTE_BUILD_TESTS=OFF `
            -DVITTE_BUILD_BENCH=OFF `
            -DVITTE_ENABLE_SANITIZERS=OFF | Out-Host
        if ($LASTEXITCODE -ne 0) { Fail "CMake configure failed." 2 }
    }

    & cmake --build $BuildDir --config Debug | Out-Host
    if ($LASTEXITCODE -ne 0) { Fail "CMake build failed." 2 }
}

function Resolve-Bin {
    $p1 = Join-Path $BuildDir $BinName
    $p2 = Join-Path $BuildDir ($BinName + ".exe")
    if (Test-Path $p1) { return $p1 }
    if (Test-Path $p2) { return $p2 }
    Fail "Binary not found: $p1 or $p2" 2
    return $null
}

function Split-Args([string]$s) {
    if ([string]::IsNullOrWhiteSpace($s)) { return @() }
    # Best-effort tokenization (supports basic quoting).
    $tokens = [System.Management.Automation.PSParser]::Tokenize($s, [ref]$null) |
    Where-Object { $_.Type -in @('CommandArgument', 'String') } |
    ForEach-Object { $_.Content }
    return , $tokens
}

Ensure-Build
$bin = Resolve-Bin
$argv = Split-Args $Args

Write-Host ("[debug_run] bin={0}" -f $bin)
Write-Host ("[debug_run] args={0}" -f ($argv -join " "))

# Run with debugger if requested
switch ($Debugger) {
    "none" {
        & $bin @argv
        if ($LASTEXITCODE -ne 0) { Fail "Program exited with code $LASTEXITCODE" 3 }
    }

    "lldb" {
        if (-not (Has "lldb")) { Fail "lldb not found in PATH." 2 }
        # lldb -- <exe> <args...>
        & lldb -- $bin @argv
        if ($LASTEXITCODE -ne 0) { Fail "lldb session exited with code $LASTEXITCODE" 3 }
    }

    "gdb" {
        if (-not (Has "gdb")) { Fail "gdb not found in PATH." 2 }
        # gdb --args <exe> <args...>
        & gdb --args $bin @argv
        if ($LASTEXITCODE -ne 0) { Fail "gdb session exited with code $LASTEXITCODE" 3 }
    }

    "vsdbg" {
        # Best-effort: requires vsdbg installation + configuration.
        # If not found, fallback to direct run.
        if (-not (Has "vsdbg")) {
            Write-Warning "vsdbg not found; running without debugger."
            & $bin @argv
            if ($LASTEXITCODE -ne 0) { Fail "Program exited with code $LASTEXITCODE" 3 }
        }
        else {
            # vsdbg typically wants --interpreter=vscode and exec; exact flags vary by install.
            & vsdbg --interpreter=vscode -- $bin @argv
            if ($LASTEXITCODE -ne 0) { Fail "vsdbg session exited with code $LASTEXITCODE" 3 }
        }
    }
}

exit 0
