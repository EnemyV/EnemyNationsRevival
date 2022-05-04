
all:	dave32ut dave16ut ..\dave32ut.h ..\dave32ut.lib ..\dave32ut.dll ..\dave16ut.dll

..\dave32ut.h: dave32ut.h
	copy dave32ut.h ..

..\dave32ut.lib: dave32ut.lib
	copy dave32ut.lib ..

..\dave32ut.dll: dave32ut.dll
	copy dave32ut.dll ..

..\dave16ut.dll: dave16ut.dll
	copy dave16ut.dll ..

dave32ut:
	nmake /f makefile.32

dave16ut:
	nmake /f makefile.16

clean:
	-del	dave16ut.dll
	-del	dave32ut.dll
	-del	dave16ut.exp
	-del	dave32ut.exp
	-del	dave16ut.lib
	-del	dave32ut.lib
	-del	dave16ut.map
	-del	dave32ut.map
	-del	dave16ut.obj
	-del	dave32ut.obj
	-del	dave16ut.pdb
	-del	dave32ut.pdb
