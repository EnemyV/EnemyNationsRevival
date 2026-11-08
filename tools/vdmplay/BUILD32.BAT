SETLOCAL
call d:\msdev\bin\vcvars32.bat x86
set INCLUDE=%INCLUDE%;C:\util\rogue.61;d:\dxsdk\sdk\inc
set LIB=%LIB%;C:\util\rogue.61\lib;d:\dxsdk\sdk\lib
NMAKE  %1 %2 %3 /f "vdmplayds.MAK" CFG="Vdmplay - Win32 Debug"
NMAKE  %1 %2 %3 /f "vdmplayds.MAK" CFG="Vdmplay - Win32 Release"
endlocal
