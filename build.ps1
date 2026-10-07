<#
.SYNOPSIS
    Builds Talmon end to end: vcpkg dependencies + CMake core + GoogleTest suite + WinUI 3 app.

.EXAMPLE
    ./build.ps1                      # Debug build + tests
    ./build.ps1 -Configuration Release -Run
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$SkipTests,
    [switch]$Run
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# --- Locate Visual Studio (MSVC, MSBuild, bundled CMake and vcpkg) -------------------------------
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -prerelease -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio with the C++ desktop workload was not found.' }

$msbuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue)?.Source
if (-not $cmake) { $cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' }
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
if (-not $env:VCPKG_ROOT) { $env:VCPKG_ROOT = Join-Path $vs 'VC\vcpkg' }

function Invoke-Step([string]$name, [scriptblock]$action) {
    Write-Host "==> $name" -ForegroundColor Cyan
    & $action
    if ($LASTEXITCODE -ne 0) { throw "$name failed (exit code $LASTEXITCODE)" }
}

$preset = $Configuration.ToLowerInvariant()

Push-Location $root # presets are resolved from the current directory
try {
    Invoke-Step 'Configure (CMake + vcpkg)' { & $cmake --preset windows-x64 }
    Invoke-Step "Build core ($Configuration)" { & $cmake --build --preset $preset }

    if (-not $SkipTests) {
        Invoke-Step 'Run tests (GoogleTest)' { & $ctest --preset $preset }
    }

    Invoke-Step "Build WinUI app ($Configuration)" {
        & $msbuild (Join-Path $root 'src\app\Talmon.vcxproj') -restore -m -nologo -v:minimal `
            "-p:Configuration=$Configuration" '-p:Platform=x64'
    }
}
finally {
    Pop-Location
}

$exe = Join-Path $root "build\app\x64\$Configuration\Talmon.exe"
Write-Host "Talmon built: $exe" -ForegroundColor Green
if ($Run) { Start-Process $exe }
