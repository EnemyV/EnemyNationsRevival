
MSVC	= c:\tools\msvc20
OTHER	= c:\tools
PATH    = $(MSVC)\bin
LINK    =
SH      = c:\tools\smrtheap


INCLUDE = $(OTHER)\include;$(SH)\include;$(MSVC)\mfc\include;$(MSVC)\include
LIB     = $(OTHER)\lib;$(SH)\msc8;$(MSVC)\mfc\lib;$(MSVC)\lib

release:	build_release copy_release ..\davenet.h

debug:	build_debug copy_debug ..\davenet.h

..\davenet.h: davenet.h
	copy davenet.h ..

copy_release:
	copy winrel\davenet.lib ..
	copy winrel\davenet.dll ..

copy_debug:
	copy windebug\davenet.lib ..
	copy windebug\davenet.dll ..

clean:
	del stdafx.pch
	del *.obj
	del *.sbr
	del davenet.aps
	del davenet.pdb
	del davenet.dll
	del davenet.lib
	del davenet.map
	del davenet.res

build_debug:
	NMAKE /f "davenet.mak" CFG="Win32 Debug"

build_release:
	NMAKE /f "davenet.mak" CFG="Win32 Release"
