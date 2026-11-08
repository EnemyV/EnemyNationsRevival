# Microsoft Visual C++ generated build script - Do not modify

PROJ = TIPX16
DEBUG = 1
PROGTYPE = 3
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\PROJECTS\DAVET\VPTEST\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = IPX16NET.CPP
RC = rc
CFLAGS_D_WTTY = /nologo /G2 /Mq /W3 /Zi /ALu /Od /D "_DEBUG" /FR /Fd"TIPX16.PDB"
CFLAGS_R_WTTY = /nologo /Gs /G2 /Mq /W3 /AM /Ox /D "NDEBUG" /FR 
LFLAGS_D_WTTY = /NOLOGO /NOD /PACKC:57344 /ALIGN:16 /ONERROR:NOEXE /CO 
LFLAGS_R_WTTY = /NOLOGO /NOD /PACKC:57344 /ALIGN:16 /ONERROR:NOEXE 
LIBS_D_WTTY = oldnames libw llibcewq 
LIBS_R_WTTY = oldnames libw mlibcewq 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = TIPX16.DEF
OBJS_EXT = 
LIBS_EXT = ..\..\..\NOVSDK\NWIPXSPX.LIB 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WTTY)
LFLAGS = $(LFLAGS_D_WTTY)
LIBS = $(LIBS_D_WTTY)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
DEFFILE=C:\PROJECTS\DAVET\VPTEST\TIPX16.DEF
!else
CFLAGS = $(CFLAGS_R_WTTY)
LFLAGS = $(LFLAGS_R_WTTY)
LIBS = $(LIBS_R_WTTY)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
DEFFILE=C:\PROJECTS\DAVET\VPTEST\TIPX16.DEF
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = IPX16NET.SBR \
		TIPX16.SBR


IPX16NET_DEP = c:\projects\davet\stdafx.h \
	c:\projects\davet\vdmplay.h \
	c:\projects\davet\vputil.h \
	c:\projects\davet\vplist.h \
	c:\projects\davet\vpnet.h \
	c:\projects\davet\datagram.h \
	c:\projects\davet\ipx16net.h


TIPX16_DEP = c:\projects\davet\stdafx.h \
	c:\projects\davet\vdmplay.h \
	c:\projects\davet\vputil.h \
	c:\projects\davet\vplist.h \
	c:\projects\davet\vpnet.h


NWIPXSPX_DEP = 

all:	$(PROJ).EXE $(PROJ).BSC

IPX16NET.OBJ:	..\IPX16NET.CPP $(IPX16NET_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c ..\IPX16NET.CPP

TIPX16.OBJ:	TIPX16.CPP $(TIPX16_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TIPX16.CPP


$(PROJ).EXE::	IPX16NET.OBJ TIPX16.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
IPX16NET.OBJ +
TIPX16.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
p:\msvc15\lib\+
p:\msvc15\mfc\lib\+
k:\blicefax\imgplus\lib\+
k:\blicefax\tiff\lib\+
..\..\..\NOVSDK\NWIPXSPX.LIB+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) $@


run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
