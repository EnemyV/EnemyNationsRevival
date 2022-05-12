SETLOCAL
call c:\msvc\bin\msvcvars.bat
set INCLUDE=%INCLUDE%;C:\util\rogue.61;c:\sdkcd6\client\include;..
set LIB=%LIB%;C:\util\rogue.61\lib;c:\sdkcd6\client\win
NMAKE  %1 %2 %3 /f "vdmpl16.mak" DEBUG=1
if exist vdmpl16.dll copy vdmpl16.dll windebug
NMAKE  %2 %3 /a /f "vdmpl16.mak" DEBUG=0
if exist vdmpl16.dll copy vdmpl16.dll winrel
endlocal
