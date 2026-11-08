# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Iserve - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Iserve - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Iserve - Win32 Debug" && "$(CFG)" != "Iserve - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Iserveds.mak" CFG="Iserve - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Iserve - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Iserve - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "Iserve - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "Iserve - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\Iserveds.exe" "$(OUTDIR)\Iserveds.bsc"

CLEAN : 
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\Iserveds.pch"
	-@erase ".\WinDebug\vc40.idb"
	-@erase ".\WinDebug\Iserveds.bsc"
	-@erase ".\WinDebug\ISERVDLG.SBR"
	-@erase ".\WinDebug\ISERVE.SBR"
	-@erase ".\WinDebug\STDAFX.SBR"
	-@erase ".\WinDebug\Iserveds.exe"
	-@erase ".\WinDebug\ISERVE.OBJ"
	-@erase ".\WinDebug\STDAFX.OBJ"
	-@erase ".\WinDebug\ISERVDLG.OBJ"
	-@erase ".\WinDebug\ISERVE.res"
	-@erase ".\WinDebug\Iserveds.ilk"
	-@erase ".\WinDebug\Iserveds.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Iserveds.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/ISERVE.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Iserveds.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/ISERVDLG.SBR" \
	"$(INTDIR)/ISERVE.SBR" \
	"$(INTDIR)/STDAFX.SBR"

"$(OUTDIR)\Iserveds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\windebug\vdmplay.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=..\windebug\vdmplay.lib wsock32.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/Iserveds.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/Iserveds.exe" 
LINK32_OBJS= \
	"$(INTDIR)/ISERVE.OBJ" \
	"$(INTDIR)/STDAFX.OBJ" \
	"$(INTDIR)/ISERVDLG.OBJ" \
	"$(INTDIR)/ISERVE.res"

"$(OUTDIR)\Iserveds.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Iserve - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : "$(OUTDIR)\Iserveds.exe" "$(OUTDIR)\Iserveds.bsc"

CLEAN : 
	-@erase ".\WinRel\Iserveds.bsc"
	-@erase ".\WinRel\ISERVDLG.SBR"
	-@erase ".\WinRel\Iserveds.pch"
	-@erase ".\WinRel\STDAFX.SBR"
	-@erase ".\WinRel\ISERVE.SBR"
	-@erase ".\WinRel\Iserveds.exe"
	-@erase ".\WinRel\ISERVE.OBJ"
	-@erase ".\WinRel\ISERVDLG.OBJ"
	-@erase ".\WinRel\STDAFX.OBJ"
	-@erase ".\WinRel\ISERVE.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Iserveds.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/ISERVE.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Iserveds.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/ISERVDLG.SBR" \
	"$(INTDIR)/STDAFX.SBR" \
	"$(INTDIR)/ISERVE.SBR"

"$(OUTDIR)\Iserveds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\windebug\vdmplay.lib wsock32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=..\windebug\vdmplay.lib wsock32.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/Iserveds.pdb" /machine:I386\
 /out:"$(OUTDIR)/Iserveds.exe" 
LINK32_OBJS= \
	"$(INTDIR)/ISERVE.OBJ" \
	"$(INTDIR)/ISERVDLG.OBJ" \
	"$(INTDIR)/STDAFX.OBJ" \
	"$(INTDIR)/ISERVE.res"

"$(OUTDIR)\Iserveds.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "Iserve - Win32 Debug"
# Name "Iserve - Win32 Release"

!IF  "$(CFG)" == "Iserve - Win32 Debug"

!ELSEIF  "$(CFG)" == "Iserve - Win32 Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\STDAFX.CPP
DEP_CPP_STDAF=\
	".\stdafx.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Iserve - Win32 Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Iserveds.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\STDAFX.OBJ" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\STDAFX.SBR" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Iserveds.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Iserve - Win32 Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Iserveds.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\STDAFX.OBJ" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\STDAFX.SBR" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Iserveds.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ISERVE.CPP
DEP_CPP_ISERV=\
	".\stdafx.h"\
	".\iserve.h"\
	".\iservdlg.h"\
	".\..\vdmplay.h"\
	

"$(INTDIR)\ISERVE.OBJ" : $(SOURCE) $(DEP_CPP_ISERV) "$(INTDIR)"\
 "$(INTDIR)\Iserveds.pch"

"$(INTDIR)\ISERVE.SBR" : $(SOURCE) $(DEP_CPP_ISERV) "$(INTDIR)"\
 "$(INTDIR)\Iserveds.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ISERVDLG.CPP
DEP_CPP_ISERVD=\
	".\stdafx.h"\
	".\iserve.h"\
	".\iservdlg.h"\
	".\..\base.h"\
	".\..\vdmplay.h"\
	

"$(INTDIR)\ISERVDLG.OBJ" : $(SOURCE) $(DEP_CPP_ISERVD) "$(INTDIR)"\
 "$(INTDIR)\Iserveds.pch"

"$(INTDIR)\ISERVDLG.SBR" : $(SOURCE) $(DEP_CPP_ISERVD) "$(INTDIR)"\
 "$(INTDIR)\Iserveds.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ISERVE.RC
DEP_RSC_ISERVE=\
	".\res\iserve.ico"\
	".\res\Discon.ico"\
	".\res\iserve.rc2"\
	

"$(INTDIR)\ISERVE.res" : $(SOURCE) $(DEP_RSC_ISERVE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt

!IF  "$(CFG)" == "Iserve - Win32 Debug"

!ELSEIF  "$(CFG)" == "Iserve - Win32 Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\iserve.txt

!IF  "$(CFG)" == "Iserve - Win32 Debug"

!ELSEIF  "$(CFG)" == "Iserve - Win32 Release"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
