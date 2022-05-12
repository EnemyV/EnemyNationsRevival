# Microsoft Visual C++ generated build script - Do not modify

PROJ = VPTST
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG
R_RCDEFINES = /dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\PROJECTS\DAVET\VPTEST\
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
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AM /Od /D "_DEBUG" /FR /GA /Fd"VPTST.PDB"
CFLAGS_R_WEXE = /nologo /Gs /G2 /W3 /AM /O1 /D "NDEBUG" /FR /GA
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE
LIBS_D_WEXE = mafxcwd oldnames libw mlibcew commdlg shell 
LIBS_R_WEXE = mafxcw oldnames libw mlibcew commdlg shell 
RCFLAGS = /nologo /z
RESFLAGS = /nologo /t
RUNFLAGS = 
DEFFILE = VPTST.DEF
OBJS_EXT = 
LIBS_EXT = ..\UT\VDMPL16.LIB 
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
		VPTST.SBR \
		MAINFRM.SBR \
		VPTSTDOC.SBR \
		VPTSTVW.SBR


VDMPL16_DEP = 

VPTST_RCDEP = res\vptst.ico \
	res\vptst.rc2


STDAFX_DEP = c:\projects\davet\vptest\stdafx.h


VPTST_DEP = c:\projects\davet\vptest\stdafx.h \
	c:\projects\davet\vptest\vptst.h \
	c:\projects\davet\vptest\mainfrm.h \
	c:\projects\davet\vptest\vptstdoc.h \
	c:\projects\davet\vptest\vptstvw.h


MAINFRM_DEP = c:\projects\davet\vptest\stdafx.h \
	c:\projects\davet\vptest\vptest.h \
	c:\projects\davet\vptest\mainfrm.h


VPTSTDOC_DEP = c:\projects\davet\vptest\stdafx.h \
	c:\projects\davet\vptest\vptst.h \
	c:\projects\davet\vptest\vptstdoc.h


VPTSTVW_DEP = c:\projects\davet\vptest\stdafx.h \
	c:\projects\davet\vptest\vptst.h \
	c:\projects\davet\vptest\vptstdoc.h \
	c:\projects\davet\vptest\vptstvw.h


all:	$(PROJ).EXE $(PROJ).BSC

VPTST.RES:	VPTST.RC $(VPTST_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r VPTST.RC

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c STDAFX.CPP

VPTST.OBJ:	VPTST.CPP $(VPTST_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c VPTST.CPP

MAINFRM.OBJ:	MAINFRM.CPP $(MAINFRM_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c MAINFRM.CPP

VPTSTDOC.OBJ:	VPTSTDOC.CPP $(VPTSTDOC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c VPTSTDOC.CPP

VPTSTVW.OBJ:	VPTSTVW.CPP $(VPTSTVW_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c VPTSTVW.CPP


$(PROJ).EXE::	VPTST.RES

$(PROJ).EXE::	STDAFX.OBJ VPTST.OBJ MAINFRM.OBJ VPTSTDOC.OBJ VPTSTVW.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
STDAFX.OBJ +
VPTST.OBJ +
MAINFRM.OBJ +
VPTSTDOC.OBJ +
VPTSTVW.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
p:\msvc15\lib\+
p:\msvc15\mfc\lib\+
k:\blicefax\imgplus\lib\+
k:\blicefax\tiff\lib\+
..\UT\VDMPL16.LIB+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) VPTST.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	VPTST.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) VPTST.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
