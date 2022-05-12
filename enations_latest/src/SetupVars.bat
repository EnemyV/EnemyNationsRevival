

set TOOLROOTDIR=C:\MSVC
C:\Program Files\DevStudio\VC\bin
set PATH=C:\Program Files\DevStudio\VC\bin;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem
:: set PATH=C:\MSVC\BIN;%PATH%
set INCLUDE=C:\WING\INCLUDE;C:\MSVC\INCLUDE;C:\MSVC\MFC\INCLUDE;C:\Source\windward\include;C:\Program Files\DevStudio\VC\mfc\include;C:\Program Files\DevStudio\VC\include
set LIB=C:\WING\LIB;C:\MSVC\LIB;C:\MSVC\MFC\LIB;%LIB%
set INIT=C:\MSVC;%INIT%

echo %INCLUDE%

@echo off
rem
rem Root of Visual Developer Studio installed files.
rem
set MSDevDir=C:\Program Files\DevStudio\SharedIDE

rem
rem Root of Visual C++ installed files.
rem
set MSVCDir=C:\Program Files\DevStudio\VC

rem
rem Root of Visual C++ files on cd-rom.
rem Remove "set vcsource=" if you don't want include cdrom in Visual C++ environment.
rem
set vcsource=D:\DevStudio

rem
rem VcOsDir is used to help create either a Windows 95 or Windows NT specific path.
rem
set VcOsDir=WIN95
if "%OS%" == "Windows_NT" set VcOsDir=WINNT

rem
echo Setting environment for using Microsoft Visual C++ tools.
rem
if "%vcsource%" == "" goto main
rem
rem Include cdrom files in environment.
rem
if "%OS%" == "Windows_NT" set PATH=%PATH%;%vcsource%\VC\BIN;%vcsource%\VC\BIN\%VcOsDir%
if "%OS%" == "" set PATH=%PATH%;"%vcsource%\VC\BIN";"%vcsource%\VC\BIN\%VcOsDir%"
set INCLUDE=%INCLUDE%;%vcsource%\VC\INCLUDE;%vcsource%\VC\MFC\INCLUDE;%vcsource%\VC\ATL\INCLUDE
set LIB=%LIB%;%vcsource%\VC\LIB;%vcsource%\VC\MFC\LIB
set vcsource=

:main
if "%OS%" == "Windows_NT" set PATH=%MSDevDir%\BIN;%MSVCDir%\BIN;%MSVCDir%\BIN\%VcOsDir%;%PATH%
if "%OS%" == "" set PATH="%MSDevDir%\BIN;%MSVCDir%\BIN";"%MSVCDir%\BIN\%VcOsDir%";%PATH%
set INCLUDE=%MSVCDir%\INCLUDE;%MSVCDir%\MFC\INCLUDE;%MSVCDir%\ATL\INCLUDE;%INCLUDE%
set LIB=%MSVCDir%\LIB;%MSVCDir%\MFC\LIB;%LIB%

set VcOsDir=
