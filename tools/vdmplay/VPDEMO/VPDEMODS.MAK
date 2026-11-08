# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Vpdemo - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Vpdemo - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Vpdemo - Win32 Debug" && "$(CFG)" != "Vpdemo - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vpdemods.mak" CFG="Vpdemo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vpdemo - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vpdemo - Win32 Release" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "Vpdemo - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"

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

ALL : "$(OUTDIR)\Vpdemods.exe" "$(OUTDIR)\Vpdemods.bsc"

CLEAN : 
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\Vpdemods.pch"
	-@erase ".\WinDebug\vc40.idb"
	-@erase ".\WinDebug\Vpdemods.bsc"
	-@erase ".\WinDebug\stdafx.sbr"
	-@erase ".\WinDebug\vpxfer.sbr"
	-@erase ".\WinDebug\loadtest.sbr"
	-@erase ".\WinDebug\vpdemdlg.sbr"
	-@erase ".\WinDebug\dxfer.sbr"
	-@erase ".\WinDebug\nbdlg.sbr"
	-@erase ".\WinDebug\tcpipxdl.sbr"
	-@erase ".\WinDebug\vpdemo.sbr"
	-@erase ".\WinDebug\TSimCfg.sbr"
	-@erase ".\WinDebug\Vpdemods.exe"
	-@erase ".\WinDebug\vpdemo.obj"
	-@erase ".\WinDebug\TSimCfg.obj"
	-@erase ".\WinDebug\stdafx.obj"
	-@erase ".\WinDebug\vpxfer.obj"
	-@erase ".\WinDebug\loadtest.obj"
	-@erase ".\WinDebug\vpdemdlg.obj"
	-@erase ".\WinDebug\dxfer.obj"
	-@erase ".\WinDebug\nbdlg.obj"
	-@erase ".\WinDebug\tcpipxdl.obj"
	-@erase ".\WinDebug\vpdemo.res"
	-@erase ".\WinDebug\Vpdemods.ilk"
	-@erase ".\WinDebug\Vpdemods.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fr /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /Fr"$(INTDIR)/" /Fp"$(INTDIR)/Vpdemods.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vpdemo.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vpdemods.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/stdafx.sbr" \
	"$(INTDIR)/vpxfer.sbr" \
	"$(INTDIR)/loadtest.sbr" \
	"$(INTDIR)/vpdemdlg.sbr" \
	"$(INTDIR)/dxfer.sbr" \
	"$(INTDIR)/nbdlg.sbr" \
	"$(INTDIR)/tcpipxdl.sbr" \
	"$(INTDIR)/vpdemo.sbr" \
	"$(INTDIR)/TSimCfg.sbr"

"$(OUTDIR)\Vpdemods.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\windebug\vdmplay.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none /map
LINK32_FLAGS=..\windebug\vdmplay.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/Vpdemods.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/Vpdemods.exe" 
LINK32_OBJS= \
	"$(INTDIR)/vpdemo.obj" \
	"$(INTDIR)/TSimCfg.obj" \
	"$(INTDIR)/stdafx.obj" \
	"$(INTDIR)/vpxfer.obj" \
	"$(INTDIR)/loadtest.obj" \
	"$(INTDIR)/vpdemdlg.obj" \
	"$(INTDIR)/dxfer.obj" \
	"$(INTDIR)/nbdlg.obj" \
	"$(INTDIR)/tcpipxdl.obj" \
	"$(INTDIR)/vpdemo.res"

"$(OUTDIR)\Vpdemods.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"

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

ALL : "$(OUTDIR)\Vpdemods.exe"

CLEAN : 
	-@erase ".\WinRel\Vpdemods.exe"
	-@erase ".\WinRel\tcpipxdl.obj"
	-@erase ".\WinRel\Vpdemods.pch"
	-@erase ".\WinRel\dxfer.obj"
	-@erase ".\WinRel\nbdlg.obj"
	-@erase ".\WinRel\TSimCfg.obj"
	-@erase ".\WinRel\stdafx.obj"
	-@erase ".\WinRel\loadtest.obj"
	-@erase ".\WinRel\vpdemdlg.obj"
	-@erase ".\WinRel\vpxfer.obj"
	-@erase ".\WinRel\vpdemo.obj"
	-@erase ".\WinRel\vpdemo.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /Fp"$(INTDIR)/Vpdemods.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vpdemo.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vpdemods.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\windebug\vdmplay.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none /map
LINK32_FLAGS=..\windebug\vdmplay.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Vpdemods.pdb" /machine:I386 /out:"$(OUTDIR)/Vpdemods.exe" 
LINK32_OBJS= \
	"$(INTDIR)/tcpipxdl.obj" \
	"$(INTDIR)/dxfer.obj" \
	"$(INTDIR)/nbdlg.obj" \
	"$(INTDIR)/TSimCfg.obj" \
	"$(INTDIR)/stdafx.obj" \
	"$(INTDIR)/loadtest.obj" \
	"$(INTDIR)/vpdemdlg.obj" \
	"$(INTDIR)/vpxfer.obj" \
	"$(INTDIR)/vpdemo.obj" \
	"$(INTDIR)/vpdemo.res"

"$(OUTDIR)\Vpdemods.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "Vpdemo - Win32 Debug"
# Name "Vpdemo - Win32 Release"

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"

!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\stdafx.cpp
DEP_CPP_STDAF=\
	".\Stdafx.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_MBCS" /Fr"$(INTDIR)/" /Fp"$(INTDIR)/Vpdemods.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\stdafx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vpdemods.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_MBCS" /Fp"$(INTDIR)/Vpdemods.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vpdemods.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpdemo.cpp
DEP_CPP_VPDEM=\
	".\Stdafx.h"\
	".\Vpdemo.h"\
	".\Vpdemdlg.h"\
	".\Nbdlg.h"\
	".\tsimcfg.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"


"$(INTDIR)\vpdemo.obj" : $(SOURCE) $(DEP_CPP_VPDEM) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"

"$(INTDIR)\vpdemo.sbr" : $(SOURCE) $(DEP_CPP_VPDEM) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"


"$(INTDIR)\vpdemo.obj" : $(SOURCE) $(DEP_CPP_VPDEM) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpdemdlg.cpp
DEP_CPP_VPDEMD=\
	".\Stdafx.h"\
	".\Vpdemo.h"\
	".\Vpdemdlg.h"\
	".\..\dxfer.h"\
	".\..\vpxfer.h"\
	".\Loadtest.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"


"$(INTDIR)\vpdemdlg.obj" : $(SOURCE) $(DEP_CPP_VPDEMD) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"

"$(INTDIR)\vpdemdlg.sbr" : $(SOURCE) $(DEP_CPP_VPDEMD) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"


"$(INTDIR)\vpdemdlg.obj" : $(SOURCE) $(DEP_CPP_VPDEMD) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpdemo.rc
DEP_RSC_VPDEMO=\
	".\res\vpdemo.ico"\
	".\res\vpdemo.rc2"\
	

"$(INTDIR)\vpdemo.res" : $(SOURCE) $(DEP_RSC_VPDEMO) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"

!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tcpipxdl.cpp
DEP_CPP_TCPIP=\
	".\Stdafx.h"\
	".\Vpdemo.h"\
	".\Tcpipxdl.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"


"$(INTDIR)\tcpipxdl.obj" : $(SOURCE) $(DEP_CPP_TCPIP) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"

"$(INTDIR)\tcpipxdl.sbr" : $(SOURCE) $(DEP_CPP_TCPIP) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"


"$(INTDIR)\tcpipxdl.obj" : $(SOURCE) $(DEP_CPP_TCPIP) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nbdlg.cpp
DEP_CPP_NBDLG=\
	".\Stdafx.h"\
	".\Vpdemo.h"\
	".\Nbdlg.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"


"$(INTDIR)\nbdlg.obj" : $(SOURCE) $(DEP_CPP_NBDLG) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"

"$(INTDIR)\nbdlg.sbr" : $(SOURCE) $(DEP_CPP_NBDLG) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"


"$(INTDIR)\nbdlg.obj" : $(SOURCE) $(DEP_CPP_NBDLG) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\Vdmplay\vpxfer.cpp

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"

DEP_CPP_VPXFE=\
	".\..\stdafx.h"\
	".\..\dxfer.h"\
	".\..\vpxfer.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\vpxfer.obj" : $(SOURCE) $(DEP_CPP_VPXFE) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"
   $(BuildCmds)

"$(INTDIR)\vpxfer.sbr" : $(SOURCE) $(DEP_CPP_VPXFE) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"

DEP_CPP_VPXFE=\
	".\..\stdafx.h"\
	".\..\dxfer.h"\
	".\..\vpxfer.h"\
	".\..\generic.h"\
	

"$(INTDIR)\vpxfer.obj" : $(SOURCE) $(DEP_CPP_VPXFE) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\Vdmplay\dxfer.cpp

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"

DEP_CPP_DXFER=\
	".\..\stdafx.h"\
	".\..\dxfer.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\dxfer.obj" : $(SOURCE) $(DEP_CPP_DXFER) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"
   $(BuildCmds)

"$(INTDIR)\dxfer.sbr" : $(SOURCE) $(DEP_CPP_DXFER) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"

DEP_CPP_DXFER=\
	".\..\stdafx.h"\
	".\..\dxfer.h"\
	".\..\generic.h"\
	

"$(INTDIR)\dxfer.obj" : $(SOURCE) $(DEP_CPP_DXFER) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\loadtest.cpp
DEP_CPP_LOADT=\
	".\Stdafx.h"\
	".\Vpdemo.h"\
	".\Vpdemdlg.h"\
	".\..\dxfer.h"\
	".\..\vpxfer.h"\
	".\Loadtest.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"


"$(INTDIR)\loadtest.obj" : $(SOURCE) $(DEP_CPP_LOADT) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"

"$(INTDIR)\loadtest.sbr" : $(SOURCE) $(DEP_CPP_LOADT) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"


"$(INTDIR)\loadtest.obj" : $(SOURCE) $(DEP_CPP_LOADT) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TSimCfg.cpp
DEP_CPP_TSIMC=\
	".\Stdafx.h"\
	".\Vpdemo.h"\
	".\tsimcfg.h"\
	".\..\vdmplay.h"\
	

!IF  "$(CFG)" == "Vpdemo - Win32 Debug"


"$(INTDIR)\TSimCfg.obj" : $(SOURCE) $(DEP_CPP_TSIMC) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"

"$(INTDIR)\TSimCfg.sbr" : $(SOURCE) $(DEP_CPP_TSIMC) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ELSEIF  "$(CFG)" == "Vpdemo - Win32 Release"


"$(INTDIR)\TSimCfg.obj" : $(SOURCE) $(DEP_CPP_TSIMC) "$(INTDIR)"\
 "$(INTDIR)\Vpdemods.pch"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
