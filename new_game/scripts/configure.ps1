# Configure CMake build for Fallen (clang++ x86, Ninja).
# Must be run once before building. Re-run when CMakeLists.txt changes.
#
# Prerequisites (install once):
#   winget install Kitware.CMake
#   winget install Ninja-build.Ninja
#   winget install LLVM.LLVM
#
# Usage:
#   powershell -File new_game/scripts/configure.ps1 -Backend opengl|d3d6
#
# clang++ auto-detects MSVC installation — no vcvarsall.bat needed.

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("opengl", "d3d6")]
    [string]$Backend  # Graphics backend: opengl or d3d6
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Find tools
# ---------------------------------------------------------------------------

$CMake = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
if (-not $CMake) { Write-Error "cmake not found on PATH. Install: winget install Kitware.CMake" }

$Ninja = Get-Command ninja -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
if (-not $Ninja) { Write-Error "ninja not found on PATH. Install: winget install Ninja-build.Ninja" }

# vcpkg.cmake: auto-detect from VS/Build Tools (via vswhere), then VCPKG_ROOT
$VcpkgCmake = $null
$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $VsWhere) {
    $VsPath = & $VsWhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($VsPath) {
        $candidate = "$VsPath\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"
        if (Test-Path $candidate) { $VcpkgCmake = $candidate }
    }
}
if (-not $VcpkgCmake -and $env:VCPKG_ROOT) {
    $candidate = "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
    if (Test-Path $candidate) { $VcpkgCmake = $candidate }
}
if (-not $VcpkgCmake) {
    Write-Error "vcpkg.cmake not found. Install VS Build Tools with C++ workload, or set VCPKG_ROOT."
}

$ProjectRoot  = Resolve-Path "$PSScriptRoot\.."
$BuildDir     = "$ProjectRoot\build"
$OurToolchain = "$ProjectRoot\cmake\clang-x86-windows.cmake"

Write-Host "Graphics backend: $Backend"
Write-Host "CMake: $CMake"
Write-Host "Ninja: $Ninja"
Write-Host "vcpkg: $VcpkgCmake"
Write-Host "Configuring CMake (Ninja Multi-Config) -> $BuildDir"

& $CMake -S $ProjectRoot `
      -B $BuildDir `
      -G "Ninja Multi-Config" `
      "-DCMAKE_MAKE_PROGRAM=$Ninja" `
      "-DCMAKE_TOOLCHAIN_FILE=$VcpkgCmake" `
      "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$OurToolchain" `
      "-DVCPKG_INSTALLED_DIR=$ProjectRoot\vcpkg_installed" `
      "-DGRAPHICS_BACKEND=$Backend"

if ($LASTEXITCODE -ne 0) {
    Write-Error "cmake configuration failed (exit $LASTEXITCODE)"
}

Write-Host ""
Write-Host "Configuration complete. Now build with:"
Write-Host "  make build-release"
Write-Host "  make build-debug"
