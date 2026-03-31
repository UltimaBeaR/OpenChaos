# Build Fallen via cmake --build.
#
# Usage:
#   powershell -File new_game/scripts/build.ps1 [-Config Debug|Release] [-Clean]

param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ScriptDir   = Split-Path $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path "$ScriptDir\.."
$BuildDir    = "$ProjectRoot\build"

$CMake = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
if (-not $CMake) { Write-Error "cmake not found on PATH. Install: winget install Kitware.CMake" }

if (-not (Test-Path $BuildDir)) {
    Write-Error "Build directory not found: $BuildDir`nRun 'make configure-opengl' first."
}

$buildArgs = @("--build", $BuildDir, "--config", $Config)
if ($Clean) { $buildArgs += "--clean-first" }
& $CMake @buildArgs

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
