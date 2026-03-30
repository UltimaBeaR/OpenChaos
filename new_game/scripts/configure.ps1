# Configure CMake build for Fallen (clang-cl x86, Ninja).
# Must be run once before building. Re-run when CMakeLists.txt changes.
#
# Usage:
#   powershell -File new_game/scripts/configure.ps1 [-Config Debug|Release]
#
# Sets up the VS x86 developer environment (for Windows SDK headers/libs),
# then invokes cmake with Ninja generator and clang-x86-windows toolchain.

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("opengl", "d3d6")]
    [string]$Backend  # Graphics backend: opengl or d3d6
)

$ErrorActionPreference = "Stop"

# Paths (VS 2022 Community — adjust if using a different edition/version)
$VsRoot      = "C:\Program Files\Microsoft Visual Studio\18\Community"
$VcvarsAll   = "$VsRoot\VC\Auxiliary\Build\vcvarsall.bat"
$VcpkgCmake  = "$VsRoot\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"
$CMake       = "$VsRoot\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

$ProjectRoot  = Resolve-Path "$PSScriptRoot\.."
$BuildDir     = "$ProjectRoot\build"
$OurToolchain = "$ProjectRoot\cmake\clang-x86-windows.cmake"

if (-not (Test-Path $VcvarsAll)) {
    Write-Error "vcvarsall.bat not found at: $VcvarsAll`nAdjust `$VsRoot in configure.ps1 to match your VS installation."
}
if (-not (Test-Path $VcpkgCmake)) {
    Write-Error "vcpkg.cmake not found at: $VcpkgCmake`nAdjust `$VsRoot in configure.ps1."
}

# Ensure essential Windows paths are available (they may be stripped when called from make/bash).
$env:Path = "$env:SystemRoot\System32;$env:SystemRoot;$env:SystemRoot\System32\Wbem;" +
            "C:\Program Files (x86)\Microsoft Visual Studio\Installer;" +
            $env:Path

# Import VS x86 environment variables into current process.
# This makes Windows SDK headers and libs available to cmake/clang-cl.
Write-Host "Setting up VS x86 environment..."
$cmd = "$env:SystemRoot\System32\cmd.exe"
$envLines = & $cmd /c "`"$VcvarsAll`" x86 && set" 2>&1
foreach ($line in $envLines) {
    if ($line -match "^([^=]+)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

Write-Host "Graphics backend: $Backend"
Write-Host "Configuring CMake (Ninja Multi-Config) -> $BuildDir"
& $CMake -S $ProjectRoot `
      -B $BuildDir `
      -G "Ninja Multi-Config" `
      "-DCMAKE_TOOLCHAIN_FILE=$VcpkgCmake" `
      "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$OurToolchain" `
      "-DVCPKG_INSTALLED_DIR=$ProjectRoot\vcpkg_installed" `
      "-DGRAPHICS_BACKEND=$Backend"

if ($LASTEXITCODE -ne 0) {
    Write-Error "cmake configuration failed (exit $LASTEXITCODE)"
}

Write-Host ""
Write-Host "Configuration complete. Now build with:"
Write-Host "  cmake --build $BuildDir --config Release"
Write-Host "  cmake --build $BuildDir --config Debug"
