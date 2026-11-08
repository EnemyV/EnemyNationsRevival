# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Debug" && "$(CFG)" != "Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "UtDemo.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : .\WinDebug\UtDemo.exe .\WinDebug\UtDemo.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /Zi /Od /I "..\.." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /Od /I "..\.." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"UtDemo.pch"\
 /Yu"stdafx.h" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"UtDemo.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"UtDemo.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"UtDemo.bsc" 
BSC32_SBRS= \
	.\WinDebug\stdafx.sbr \
	.\WinDebug\UtDemo.sbr \
	.\WinDebug\UtDemdlg.sbr

.\WinDebug\UtDemo.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# SUBTRACT BASE LINK32 /PDB:none
# ADD LINK32 /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=/NOLOGO /SUBSYSTEM:windows /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"UtDemo.pdb" /DEBUG /MACHINE:I386 /OUT:$(OUTDIR)/"UtDemo.exe" 
DEF_FILE=
LINK32_OBJS= \
	.\WinDebug\stdafx.obj \
	.\WinDebug\UtDemo.obj \
	.\WinDebug\UtDemdlg.obj \
	.\WinDebug\UtDemo.res \
	\Projects\Vdmplay\ut\vdmplay32.lib

.\WinDebug\UtDemo.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : .\WinRel\UtDemo.exe .\WinRel\UtDemo.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\.." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\.." /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"UtDemo.pch"\
 /Yu"stdafx.h" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"UtDemo.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"UtDemo.bsc" 
BSC32_SBRS= \
	.\WinRel\stdafx.sbr \
	.\WinRel\UtDemo.sbr \
	.\WinRel\UtDemdlg.sbr

.\WinRel\UtDemo.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# SUBTRACT BASE LINK32 /PDB:none
# ADD LINK32 /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=/NOLOGO /SUBSYSTEM:windows /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"UtDemo.pdb" /MACHINE:I386 /OUT:$(OUTDIR)/"UtDemo.exe" 
DEF_FILE=
LINK32_OBJS= \
	.\WinRel\stdafx.obj \
	.\WinRel\UtDemo.obj \
	.\WinRel\UtDemdlg.obj \
	.\WinRel\UtDemo.res \
	\Projects\Vdmplay\ut\vdmplay32.lib

.\WinRel\UtDemo.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

MTL_PROJ=

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

SOURCE=.\stdafx.cpp
DEP_STDAF=\
	.\stdafx.h

!IF  "$(CFG)" == "Win32 Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

.\WinDebug\stdafx.obj :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /MD /W3 /GX /Zi /Od /I "..\.." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"UtDemo.pch"\
 /Yc"stdafx.h" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"UtDemo.pdb" /c  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

.\WinRel\stdafx.obj :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /MD /W3 /GX /O2 /I "..\.." /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"UtDemo.pch"\
 /Yc"stdafx.h" /Fo$(INTDIR)/ /c  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UtDemo.cpp
DEP_UTDEM=\
	.\UtDemdlg.h\
	.\UtDemo.h

!IF  "$(CFG)" == "Win32 Debug"

.\WinDebug\UtDemo.obj :  $(SOURCE)  $(DEP_UTDEM) $(INTDIR)\
 .\WinDebug\stdafx.obj

!ELSEIF  "$(CFG)" == "Win32 Release"

.\WinRel\UtDemo.obj :  $(SOURCE)  $(DEP_UTDEM) $(INTDIR) .\WinRel\stdafx.obj

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UtDemdlg.cpp
DEP_UTDEMD=\
	.\UtDemo.h\
	.\UtDemdlg.h

!IF  "$(CFG)" == "Win32 Debug"

.\WinDebug\UtDemdlg.obj :  $(SOURCE)  $(DEP_UTDEMD) $(INTDIR)\
 .\WinDebug\stdafx.obj

!ELSEIF  "$(CFG)" == "Win32 Release"

.\WinRel\UtDemdlg.obj :  $(SOURCE)  $(DEP_UTDEMD) $(INTDIR) .\WinRel\stdafx.obj

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UtDemo.rc
DEP_UTDEMO=\
	.\res\UtDemo.ico\
	.\res\UtDemo.rc2

!IF  "$(CFG)" == "Win32 Debug"

.\WinDebug\UtDemo.res :  $(SOURCE)  $(DEP_UTDEMO) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Release"

.\WinRel\UtDemo.res :  $(SOURCE)  $(DEP_UTDEMO) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt
# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\Vdmplay\ut\vdmplay32.lib
# End Source File
# End Group
# End Project
################################################################################
