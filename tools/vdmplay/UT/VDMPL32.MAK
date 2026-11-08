# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "VDMPL32.MAK" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "WinRel"
OUTDIR=.
INTDIR=.\WinRel

ALL : $(OUTDIR)/WinRel\vdmplay.dll $(OUTDIR)/VDMPL32.bsc

$(INTDIR) : 
    if not exist $(INTDIR)/nul mkdir $(INTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /YX /O2 /I ".." /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /YX /O2 /I ".." /I "." /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"VDMPL32.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /i ".." /d "NDEBUG" /d "THUNK"
RSC_PROJ=/l 0x809 /fo$(INTDIR)/"VDMPLAY.res" /i ".." /d "NDEBUG" /d "THUNK" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"VDMPL32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/VDMPL32.SBR \
	$(INTDIR)/ADVANCED.SBR

$(OUTDIR)/VDMPL32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL /PDB:"vdmplay.pdb" /MACHINE:I386 /FORCE /OUT:"WinRel\vdmplay.dll"
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL /INCREMENTAL:no\
 /PDB:"vdmplay.pdb" /MACHINE:I386 /DEF:".\VDMPL32.DEF" /FORCE\
 /OUT:"WinRel\vdmplay.dll" /IMPLIB:$(OUTDIR)/"VDMPL32.lib" 
DEF_FILE=.\VDMPL32.DEF
LINK32_OBJS= \
	$(INTDIR)/VDMPL32.OBJ \
	.\W32SUT32.LIB \
	$(INTDIR)/VDMPLAY.res \
	$(INTDIR)/ADVANCED.OBJ

$(OUTDIR)/WinRel\vdmplay.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.
INTDIR=.\WinDebug

ALL : $(OUTDIR)/WinDebug\vdmplay.dll $(OUTDIR)/VDMPL32.bsc

$(INTDIR) : 
    if not exist $(INTDIR)/nul mkdir $(INTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /Zi /YX /Od /I ".." /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /Zi /YX /Od /I ".." /I "." /D "WIN32" /D "_DEBUG"\
 /D "_WINDOWS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"VDMPL32.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"VDMPL32.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /i ".." /d "_DEBUG" /d "THUNK"
RSC_PROJ=/l 0x809 /fo$(INTDIR)/"VDMPLAY.res" /i ".." /d "_DEBUG" /d "THUNK" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"VDMPL32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/VDMPL32.SBR \
	$(INTDIR)/ADVANCED.SBR

$(OUTDIR)/VDMPL32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386 /FORCE /OUT:"WinDebug\vdmplay.dll"
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"VDMPL32.pdb" /DEBUG /MACHINE:I386 /DEF:".\VDMPL32.DEF" /FORCE\
 /OUT:"WinDebug\vdmplay.dll" /IMPLIB:$(OUTDIR)/"VDMPL32.lib" 
DEF_FILE=.\VDMPL32.DEF
LINK32_OBJS= \
	$(INTDIR)/VDMPL32.OBJ \
	.\W32SUT32.LIB \
	$(INTDIR)/VDMPLAY.res \
	$(INTDIR)/ADVANCED.OBJ

$(OUTDIR)/WinDebug\vdmplay.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\VDMPL32.CPP
DEP_VDMPL=\
	.\stdafx.h\
	.\W32SUT.H\
	.\VDMPLUT.H\
	\PROJECTS\VDMPLAY\GENERIC.H\
	\PROJECTS\VDMPLAY\VDMPLAY.H\
	\PROJECTS\VDMPLAY\VPUTIL.H\
	\PROJECTS\VDMPLAY\VPLIST.H\
	\PROJECTS\VDMPLAY\VPNET.H\
	m:\rogue.61\rw\generic.h\
	m:\rogue.61\rw\defs.h\
	m:\rogue.61\rw\compiler.h\
	m:\rogue.61\rw\rwwind.h

$(INTDIR)/VDMPL32.OBJ :  $(SOURCE)  $(DEP_VDMPL) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\W32SUT32.LIB
# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPL32.DEF
# End Source File
################################################################################
# Begin Source File

SOURCE=\PROJECTS\VDMPLAY\VDMPLAY.RC
DEP_VDMPLA=\
	\PROJECTS\VDMPLAY\VERSION.RC\
	\PROJECTS\VDMPLAY\VER32.RC\
	\PROJECTS\VDMPLAY\VERSION.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/VDMPLAY.res :  $(SOURCE)  $(DEP_VDMPLA) $(INTDIR)
   $(RSC) /l 0x809 /fo$(INTDIR)/"VDMPLAY.res" /i ".." /i "\PROJECTS\VDMPLAY" /d\
 "NDEBUG" /d "THUNK"  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/VDMPLAY.res :  $(SOURCE)  $(DEP_VDMPLA) $(INTDIR)
   $(RSC) /l 0x809 /fo$(INTDIR)/"VDMPLAY.res" /i ".." /i "\PROJECTS\VDMPLAY" /d\
 "_DEBUG" /d "THUNK"  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\PROJECTS\VDMPLAY\ADVANCED.CPP
DEP_ADVAN=\
	\PROJECTS\VDMPLAY\STDAFX.H\
	\PROJECTS\VDMPLAY\BASE.H\
	\PROJECTS\VDMPLAY\VPENGINE.H\
	\PROJECTS\VDMPLAY\WNOTQUE.H\
	\PROJECTS\VDMPLAY\VPWINSK.H\
	\PROJECTS\VDMPLAY\HELP.H\
	.\W32SUT.H\
	\PROJECTS\VDMPLAY\DATAGRAM.H\
	\PROJECTS\VDMPLAY\TCPNET.H\
	\PROJECTS\VDMPLAY\WSIPXNET.H\
	\PROJECTS\VDMPLAY\IPX16NET.H\
	\PROJECTS\VDMPLAY\NBNET.H\
	\PROJECTS\VDMPLAY\COMMPORT.H\
	\PROJECTS\VDMPLAY\COMMNET.H\
	\PROJECTS\VDMPLAY\MDMNET.H\
	\PROJECTS\VDMPLAY\W32COMM.H\
	\PROJECTS\VDMPLAY\SMAP.H\
	\PROJECTS\VDMPLAY\VERSION.H\
	\PROJECTS\VDMPLAY\GENERIC.H\
	\PROJECTS\VDMPLAY\VDMPLAY.H\
	\PROJECTS\VDMPLAY\VPUTIL.H\
	\PROJECTS\VDMPLAY\VPLIST.H\
	\PROJECTS\VDMPLAY\VPNET.H\
	m:\rogue.61\rw\generic.h\
	m:\rogue.61\rw\defs.h\
	m:\rogue.61\rw\compiler.h\
	m:\rogue.61\rw\rwwind.h

$(INTDIR)/ADVANCED.OBJ :  $(SOURCE)  $(DEP_ADVAN) $(INTDIR)
   $(CPP) $(CPP_PROJ)  $(SOURCE) 

# End Source File
# End Group
# End Project
################################################################################
