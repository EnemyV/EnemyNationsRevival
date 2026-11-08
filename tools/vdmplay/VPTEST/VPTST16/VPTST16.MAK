# Microsoft Visual C++ generated build script - Do not modify

PROJ = VPTST16
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG
R_RCDEFINES = /dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\PROJECTS\VDMPLAY\VPTEST\VPTST16\
USEMFC = 1
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = /YcSTDAFX.H
CUSEPCHFLAG = 
CPPUSEPCHFLAG = /YuSTDAFX.H
FIRSTC =             
FIRSTCPP = STDAFX.CPP  
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AL /Gx- /Od /D "_DEBUG" /I "..\.." /FR /GA /Fd"VPTST16.PDB" 
CFLAGS_R_WEXE = /nologo /Gs /G2 /W3 /AM /O1 /D "NDEBUG" /I "..\.." /FR /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:16384 /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:16384 /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = lafxcwd llibcew oldnames libw commdlg.lib shell.lib 
LIBS_R_WEXE = mafxcw mlibcew oldnames libw commdlg.lib shell.lib 
RCFLAGS = /nologo /z
RESFLAGS = /nologo /t
RUNFLAGS = 
DEFFILE = VPTST16.DEF
OBJS_EXT = 
LIBS_EXT = ..\..\UT\VDMPL16.LIB 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = STDAFX.SBR \
		VPTST16.SBR \
		MAINFRM.SBR \
		VPTSTDOC.SBR \
		VPTSTVW.SBR


VDMPL16_DEP = 

VPTST16_RCDEP = c:\projects\vdmplay\vptest\vptst16\res\vptst16.ico \
	c:\projects\vdmplay\vptest\vptst16\res\vptst16.rc2


STDAFX_DEP = c:\projects\vdmplay\vptest\vptst16\stdafx.h


VPTST16_DEP = c:\projects\vdmplay\vptest\vptst16\stdafx.h \
	c:\projects\vdmplay\vptest\vptst16\vptst16.h \
	c:\projects\vdmplay\vptest\vptst16\mainfrm.h \
	c:\projects\vdmplay\vptest\vptst16\vptstdoc.h \
	c:\projects\vdmplay\vptest\vptst16\vptstvw.h


MAINFRM_DEP = c:\projects\vdmplay\vptest\vptst16\stdafx.h \
	c:\projects\vdmplay\vptest\vptst16\vptst16.h \
	c:\projects\vdmplay\vptest\vptst16\mainfrm.h


VPTSTDOC_DEP = c:\projects\vdmplay\vptest\vptst16\stdafx.h \
	c:\projects\vdmplay\vptest\vptst16\vptst16.h \
	c:\projects\vdmplay\vptest\vptst16\vptstdoc.h


VPTSTVW_DEP = c:\projects\vdmplay\vptest\vptst16\stdafx.h \
	c:\projects\vdmplay\vptest\vptst16\vptst16.h \
	c:\projects\vdmplay\vptest\vptst16\vptstdoc.h \
	c:\projects\vdmplay\vptest\vptst16\vptstvw.h


all:	$(PROJ).EXE $(PROJ).BSC

VPTST16.RES:	VPTST16.RC $(VPTST16_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r VPTST16.RC

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

VPTST16.OBJ:	VPTST16.CPP $(VPTST16_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c VPTST16.CPP

MAINFRM.OBJ:	MAINFRM.CPP $(MAINFRM_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c MAINFRM.CPP

VPTSTDOC.OBJ:	VPTSTDOC.CPP $(VPTSTDOC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c VPTSTDOC.CPP

VPTSTVW.OBJ:	VPTSTVW.CPP $(VPTSTVW_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c VPTSTVW.CPP


$(PROJ).EXE::	VPTST16.RES

$(PROJ).EXE::	STDAFX.OBJ VPTST16.OBJ MAINFRM.OBJ VPTSTDOC.OBJ VPTSTVW.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
VPTST16.OBJ +
MAINFRM.OBJ +
VPTSTDOC.OBJ +
VPTSTVW.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\msvc\lib\+
c:\msvc\mfc\lib\+
c:\util\rogue.61\lib\+
..\..\UT\VDMPL16.LIB+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) VPTST16.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	VPTST16.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) VPTST16.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
