# Build Fallen via cmake --build.
# Ensures powershell.exe is on PATH so post-build DLL copy steps work
# (cmake post-build commands run via cmd.exe which uses the Windows PATH).
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
$CMake       = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if (-not (Test-Path $BuildDir)) {
    Write-Error "Build directory not found: $BuildDir`nRun 'make configure' first."
}

# Ensure powershell.exe is on PATH — cmake's post-build steps invoke it via cmd.exe
# which uses the Windows PATH (inherited from this process).
$psDir = "$env:SystemRoot\System32\WindowsPowerShell\v1.0"
if ($env:Path -notlike "*WindowsPowerShell*") {
    $env:Path = "$psDir;$env:Path"
}

$buildArgs = @("--build", $BuildDir, "--config", $Config)
if ($Clean) { $buildArgs += "--clean-first" }
& $CMake @buildArgs

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
