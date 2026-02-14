@echo off
setlocal
pushd "%~dp0"

echo ==========================================
echo      Enemy Nations Release Build Script
echo ==========================================
echo.

set "CMAKE_EXE=cmake"

:: Check for CMake in PATH
echo [1/4] Checking for CMake...
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    echo CMake found in PATH.
    goto :check_vs
)

echo CMake not found in PATH. Checking Visual Studio bundled CMake...

:: Check for Visual Studio 2022 (needed for both CMake check and build env)
:check_vs
echo [2/4] Checking for Visual Studio 2022...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error: Visual Studio Installer not found. Is Visual Studio installed?
    pause
    exit /b 1
)

:: Use a temporary file to capture output, avoiding complex quoting issues with for /f
"%VSWHERE%" -latest -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath > "%TEMP%\vs_path.txt"
set /p VS_PATH=<"%TEMP%\vs_path.txt"
del "%TEMP%\vs_path.txt"

if "%VS_PATH%"=="" (
    echo Error: Visual Studio 2022 with 'Desktop development with C++' workload not found.
    echo Please install Visual Studio 2022 and ensure the C++ workload is selected.
    pause
    exit /b 1
)
echo Found VS 2022 at: %VS_PATH%
echo.

:: If CMake was not found in PATH, check VS install
if not defined CMAKE_EXE set "CMAKE_EXE=cmake"
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    set "CMAKE_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if not exist "%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        echo Error: CMake not found in PATH and not found in Visual Studio installation.
        echo Please install CMake or ensure the 'C++ CMake tools for Windows' component is selected in VS Installer.
        pause
        exit /b 1
    )
    echo Found bundled CMake.
)

:: Wrap CMAKE_EXE in quotes for execution if it contains spaces (which the long path does)
if not "%CMAKE_EXE%"=="cmake" set "CMAKE_EXE="%CMAKE_EXE%""
echo Using CMake: %CMAKE_EXE%
echo.

:: Check for ccache
echo [3/4] Checking for ccache...
if exist "D:\ccache-4.8.1-windows-x86_64\ccache.exe" (
    echo ccache found.
) else (
    echo Warning: ccache not found at D:\ccache-4.8.1-windows-x86_64\ccache.exe.
    echo The build will continue, but might be slower.
)
echo.

:: Build Process
echo [4/4] Starting Build Process...

if not exist cmakeBuild (
    echo Creating cmakeBuild directory...
    mkdir cmakeBuild
)

echo Generating build files...
%CMAKE_EXE% -S . -B cmakeBuild -G "Visual Studio 17 2022" -A Win32
if %errorlevel% neq 0 (
    echo Error: CMake generation failed.
    pause
    exit /b 1
)

echo Building Release configuration...
%CMAKE_EXE% --build cmakeBuild --config Release
if %errorlevel% neq 0 (
    echo Error: Build failed.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo           Build Complete!
echo ==========================================
pause

popd
endlocal
