@echo off
REM Build script for OpenGL project
REM Usage: build.bat [Debug|Release] [x64|Win32]

REM Change to the script's directory
cd /d "%~dp0"

set CONFIG=%~1
set PLATFORM=%~2

if "%CONFIG%"=="" set CONFIG=Debug
if "%PLATFORM%"=="" set PLATFORM=x64

echo Building %CONFIG%|%PLATFORM% configuration...
echo Working directory: %CD%

REM Find MSBuild (Visual Studio 2022)
set "MSBUILD_PATH="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
)

if "%MSBUILD_PATH%"=="" (
    echo ERROR: MSBuild not found. Please install Visual Studio 2022 or 2019.
    echo Trying to use MSBuild from PATH...
    MSBuild.exe opengl.sln /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m
) else (
    "%MSBUILD_PATH%" opengl.sln /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build succeeded!
    echo Executable location: %PLATFORM%\%CONFIG%\openGL.exe
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

