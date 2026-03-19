# Copy original game resources to build output directory.
# Run once after fresh clone, or when resources change.
#
# Usage:
#   powershell -File new_game/scripts/copy_resources.ps1 [-Config Debug|Release|Both]
#
# Source:  <repo>/original_game_resources/
# Dest:    <repo>/new_game/build/Debug/  and/or  .../Release/

param(
    [ValidateSet("Debug", "Release", "Both")]
    [string]$Config = "Both"
)

$ErrorActionPreference = "Stop"

$ScriptDir   = Split-Path $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path "$ScriptDir\..\.."
$Src         = "$ProjectRoot\original_game_resources"
$BuildRoot   = Resolve-Path "$ScriptDir\.." | Join-Path -ChildPath "build"
$RoboCopy    = "$env:SystemRoot\System32\robocopy.exe"

# ---------------------------------------------------------------------------
# Exclusion lists — add any file/dir that should never land in the game folder.
# ---------------------------------------------------------------------------

# Files to exclude (matched by name, any directory depth)
$ExcludeFiles = @(
    ".gitkeep",
    ".gitignore",
    ".gitattributes",
    ".gitmodules"
)

# Directories to exclude
$ExcludeDirs = @(
    ".git"
)

# ---------------------------------------------------------------------------

function Copy-ToConfig([string]$Dest) {
    Write-Host "Copying resources -> $Dest"
    New-Item -ItemType Directory -Force -Path $Dest | Out-Null

    $roboArgs  = @($Src, $Dest, "/E")
    $roboArgs += $ExcludeFiles | ForEach-Object { "/XF"; $_ }
    $roboArgs += $ExcludeDirs  | ForEach-Object { "/XD"; $_ }
    $roboArgs += "/NFL", "/NDL", "/NJH", "/NJS", "/NC", "/NS"

    & $RoboCopy @roboArgs

    # robocopy exit codes: 0-7 = success (bitmask of what changed), 8+ = error
    if ($LASTEXITCODE -ge 8) {
        Write-Error "robocopy failed (exit $LASTEXITCODE)"
        exit $LASTEXITCODE
    }
}

if ($Config -eq "Debug"   -or $Config -eq "Both") { Copy-ToConfig "$BuildRoot\Debug"   }
if ($Config -eq "Release" -or $Config -eq "Both") { Copy-ToConfig "$BuildRoot\Release" }

Write-Host "Done."
