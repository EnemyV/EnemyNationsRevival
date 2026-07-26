@echo off
setlocal
pushd "%~dp0"

echo ==========================================
echo      Enemy Nations Release Build Script
echo ==========================================
echo.

:: ---------------------------------------------------------------------------
:: [1/4] Locate Visual Studio (any version with the Desktop C++ workload).
:: We do NOT pin to 2022 - vswhere returns the newest install that has the
:: C++ tools, and we build the matching CMake generator name from it
:: ("Visual Studio <major> <year>", e.g. 17 2022, 18 2026). -products *
:: also picks up the standalone Build Tools SKU.
:: ---------------------------------------------------------------------------
echo [1/4] Locating Visual Studio with the C++ workload...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error: vswhere.exe not found. Is Visual Studio ^(or the VS Build Tools^) installed?
    pause
    exit /b 1
)

:: Capture each property to a temp file - avoids for /f quoting headaches.
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath    > "%TEMP%\en_vs_path.txt"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationVersion > "%TEMP%\en_vs_ver.txt"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Workload.NativeDesktop -property catalog_productLineVersion > "%TEMP%\en_vs_year.txt"
set "VS_PATH="
set "VS_VER="
set "VS_YEAR="
if exist "%TEMP%\en_vs_path.txt" set /p VS_PATH=<"%TEMP%\en_vs_path.txt"
if exist "%TEMP%\en_vs_ver.txt"  set /p VS_VER=<"%TEMP%\en_vs_ver.txt"
if exist "%TEMP%\en_vs_year.txt" set /p VS_YEAR=<"%TEMP%\en_vs_year.txt"
del "%TEMP%\en_vs_path.txt" "%TEMP%\en_vs_ver.txt" "%TEMP%\en_vs_year.txt" 2>nul

if "%VS_PATH%"=="" (
    echo Error: No Visual Studio with the 'Desktop development with C++' workload was found.
    echo   Fix: in the Visual Studio Installer, add the "Desktop development with C++"
    echo        workload ^(VS 2019/2022/2026 or the standalone Build Tools all work^).
    echo   Do NOT hand-open enations.dsp / enations.sln - those are dead legacy
    echo   MSVC-6 files ^(they reference mss\mssw.h, which no longer exists^). Only
    echo   the CMake-generated solution in cmakeBuild-x64\ is correct.
    pause
    exit /b 1
)

:: Major version = the part before the first dot of installationVersion (17, 18, ...).
for /f "tokens=1 delims=." %%v in ("%VS_VER%") do set "VS_MAJOR=%%v"

:: catalog_productLineVersion is the year (2022, 2026). Fall back to a major->year
:: map if an older vswhere didn't report it.
if not "%VS_YEAR%"=="" goto have_year
if "%VS_MAJOR%"=="15" set "VS_YEAR=2017"
if "%VS_MAJOR%"=="16" set "VS_YEAR=2019"
if "%VS_MAJOR%"=="17" set "VS_YEAR=2022"
if "%VS_MAJOR%"=="18" set "VS_YEAR=2026"
:have_year

if "%VS_MAJOR%"=="" goto no_generator
if "%VS_YEAR%"=="" goto no_generator
set "VS_GENERATOR=Visual Studio %VS_MAJOR% %VS_YEAR%"
echo Found Visual Studio %VS_YEAR% ^(v%VS_MAJOR%.x^) at: %VS_PATH%
echo CMake generator: %VS_GENERATOR%
echo.
goto find_cmake

:no_generator
echo Error: could not determine the Visual Studio version ^(got version="%VS_VER%" year="%VS_YEAR%"^).
echo Please report this; as a workaround install Visual Studio 2022 or 2026.
pause
exit /b 1

:: ---------------------------------------------------------------------------
:: [2/4] Locate CMake. Prefer the CMake bundled with the detected VS - it is
:: guaranteed to know that VS's generator. Fall back to a CMake on PATH.
:: ---------------------------------------------------------------------------
:find_cmake
echo [2/4] Locating CMake...
set "CMAKE_EXE="
set "VS_CMAKE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if exist "%VS_CMAKE%" (
    set "CMAKE_EXE="%VS_CMAKE%""
    echo Using Visual Studio's bundled CMake.
    goto have_cmake
)
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    set "CMAKE_EXE=cmake"
    echo Using CMake from PATH ^(must be new enough to know the "%VS_GENERATOR%" generator^).
    goto have_cmake
)
echo Error: CMake not found - neither bundled with Visual Studio nor on PATH.
echo   Fix: in the VS Installer, add the "C++ CMake tools for Windows" component,
echo        or install CMake separately and put it on PATH.
pause
exit /b 1

:have_cmake
echo Using CMake: %CMAKE_EXE%
echo.

:: ---------------------------------------------------------------------------
:: [3/4] ccache is OPTIONAL - it only speeds up repeat builds; the build works
:: fine without it. This check just prints a heads-up. The path is hard-coded
:: to D:\ccache-4.8.1-windows-x86_64\ccache.exe in TWO places:
::   1. here (this warning only), and
::   2. CMakeLists.txt line 9 - the PATHS arg of find_program(CCACHE_PROGRAM ...),
::      which is what actually enables ccache. Change it there (and here) if your
::      ccache lives elsewhere, or delete that PATHS arg to just use PATH.
:: ---------------------------------------------------------------------------
echo [3/4] Checking for ccache ^(OPTIONAL - only speeds up rebuilds^)...
if exist "D:\ccache-4.8.1-windows-x86_64\ccache.exe" (
    echo ccache found at D:\ccache-4.8.1-windows-x86_64\ccache.exe.
) else (
    echo Note: ccache not found at D:\ccache-4.8.1-windows-x86_64\ccache.exe - not required.
    echo The build will continue; rebuilds may just be slower. To use ccache, set its
    echo path here and in CMakeLists.txt line 9 ^(find_program CCACHE_PROGRAM PATHS ...^).
)
echo.

:: ---------------------------------------------------------------------------
:: [4/4] Generate + build. x64 Release only (Win32 is no longer supported).
:: ---------------------------------------------------------------------------
echo [4/4] Starting build process...

if not exist cmakeBuild-x64 (
    echo Creating cmakeBuild-x64 directory...
    mkdir cmakeBuild-x64
)

echo Generating build files (x64)...
%CMAKE_EXE% -S . -B cmakeBuild-x64 -G "%VS_GENERATOR%" -A x64
if %errorlevel% neq 0 (
    echo Error: CMake generation failed.
    pause
    exit /b 1
)

echo Building Release configuration (x64)...
:: Build only the game target ^(+ its real deps: wind22, vdmplay^). Building the
:: whole solution also compiles dev-only tools like compit/cdf, and compit fails
:: to compile ^(it pulls real MFC on top of the mfc_compat shim^) - which would make
:: this script report failure even though the game built fine. Matches build.ps1,
:: which builds the enations target alone.
%CMAKE_EXE% --build cmakeBuild-x64 --config Release --target enations
if %errorlevel% neq 0 (
    echo Error: Build failed.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo           Build Complete!
echo ==========================================
echo Output: cmakeBuild-x64\enations_latest\src\Release\enations.exe
pause

popd
endlocal
