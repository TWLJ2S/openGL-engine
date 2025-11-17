# PowerShell build script for OpenGL project
# Usage: .\build.ps1 [Debug|Release] [x64|Win32]

param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

# Change to the script's directory (where opengl.sln is located)
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptPath

Write-Host "Building $Configuration|$Platform configuration..." -ForegroundColor Cyan
Write-Host "Working directory: $scriptPath" -ForegroundColor Gray

# Find MSBuild (Visual Studio 2022)
$msbuildPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
)

$msbuild = $null
foreach ($path in $msbuildPaths) {
    if (Test-Path $path) {
        $msbuild = $path
        break
    }
}

if ($null -eq $msbuild) {
    Write-Host "MSBuild not found in standard locations. Trying PATH..." -ForegroundColor Yellow
    $msbuild = "MSBuild.exe"
}

Write-Host "Using MSBuild: $msbuild" -ForegroundColor Gray

# Build the solution
& $msbuild opengl.sln /p:Configuration=$Configuration /p:Platform=$Platform /m /v:minimal

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild succeeded!" -ForegroundColor Green
    Write-Host "Executable location: $Platform\$Configuration\openGL.exe" -ForegroundColor Green
} else {
    Write-Host "`nBuild failed with error code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}

