# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "TNBNET.MAK" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : $(OUTDIR)/TNBNET.exe $(OUTDIR)/TNBNET.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /MT /W3 /GX /YX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /YX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D\
 "_CONSOLE" /FR$(INTDIR)/ /Fp$(OUTDIR)/"TNBNET.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"TNBNET.bsc" 
BSC32_SBRS= \
	$(INTDIR)/TNBNET.SBR \
	$(INTDIR)/NBNET.SBR

$(OUTDIR)/TNBNET.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib /NOLOGO\
 /SUBSYSTEM:console /INCREMENTAL:no /PDB:$(OUTDIR)/"TNBNET.pdb" /MACHINE:I386\
 /OUT:$(OUTDIR)/"TNBNET.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/TNBNET.OBJ \
	$(INTDIR)/NBNET.OBJ

$(OUTDIR)/TNBNET.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/TNBNET.exe $(OUTDIR)/TNBNET.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /MT /W3 /GX /Zi /YX /Od /I ".." /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /Zi /YX /Od /I ".." /D "_DEBUG" /D "WIN32" /D\
 "_CONSOLE" /FR$(INTDIR)/ /Fp$(OUTDIR)/"TNBNET.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"TNBNET.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"TNBNET.bsc" 
BSC32_SBRS= \
	$(INTDIR)/TNBNET.SBR \
	$(INTDIR)/NBNET.SBR

$(OUTDIR)/TNBNET.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib /NOLOGO\
 /SUBSYSTEM:console /INCREMENTAL:yes /PDB:$(OUTDIR)/"TNBNET.pdb" /DEBUG\
 /MACHINE:I386 /OUT:$(OUTDIR)/"TNBNET.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/TNBNET.OBJ \
	$(INTDIR)/NBNET.OBJ

$(OUTDIR)/TNBNET.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

SOURCE=.\TNBNET.CPP
DEP_TNBNE=\
	\Projects\davet\STDAFX.H\
	\Projects\davet\datagram.h\
	\Projects\davet\NBNET.H\
	\Projects\davet\GENERIC.H\
	\Projects\davet\VDMPLAY.H\
	\Projects\davet\VPUTIL.H\
	\Projects\davet\VPLIST.H\
	\Projects\davet\VPNET.H\
	\Projects\davet\VPENGINE.H

$(INTDIR)/TNBNET.OBJ :  $(SOURCE)  $(DEP_TNBNE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\davet\NBNET.CPP
DEP_NBNET=\
	\Projects\davet\STDAFX.H\
	\Projects\davet\datagram.h\
	\Projects\davet\NBNET.H\
	\Projects\davet\GENERIC.H\
	\Projects\davet\VDMPLAY.H\
	\Projects\davet\VPUTIL.H\
	\Projects\davet\VPLIST.H\
	\Projects\davet\VPNET.H\
	\Projects\davet\VPENGINE.H

$(INTDIR)/NBNET.OBJ :  $(SOURCE)  $(DEP_NBNET) $(INTDIR)
   $(CPP) $(CPP_PROJ)  $(SOURCE) 

# End Source File
# End Group
# End Project
################################################################################
