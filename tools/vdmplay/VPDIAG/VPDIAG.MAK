# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=vpdiag - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to vpdiag - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vpdiag - Win32 Release" && "$(CFG)" != "vpdiag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vpdiag.mak" CFG="vpdiag - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vpdiag - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vpdiag - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "vpdiag - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "vpdiag - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\Vpdiag.exe"

CLEAN : 
	-@erase ".\Release\Vpdiag.exe"
	-@erase ".\Release\NbDlg.obj"
	-@erase ".\Release\Vpdiag.pch"
	-@erase ".\Release\vpdiag.obj"
	-@erase ".\Release\TcpDlg.obj"
	-@erase ".\Release\net.obj"
	-@erase ".\Release\StdAfx.obj"
	-@erase ".\Release\Vpwinsk.obj"
	-@erase ".\Release\vputil.obj"
	-@erase ".\Release\vpdiagdl.obj"
	-@erase ".\Release\vpdiag.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NOVPTRACE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_MBCS" /D "NOVPTRACE" /Fp"$(INTDIR)/Vpdiag.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vpdiag.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vpdiag.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 netapi32.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=netapi32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Vpdiag.pdb" /machine:I386 /out:"$(OUTDIR)/Vpdiag.exe" 
LINK32_OBJS= \
	"$(INTDIR)/NbDlg.obj" \
	"$(INTDIR)/vpdiag.obj" \
	"$(INTDIR)/TcpDlg.obj" \
	"$(INTDIR)/net.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/Vpwinsk.obj" \
	"$(INTDIR)/vputil.obj" \
	"$(INTDIR)/vpdiagdl.obj" \
	"$(INTDIR)/vpdiag.res"

"$(OUTDIR)\Vpdiag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\Vpdiag.exe" "$(OUTDIR)\Vpdiag.bsc"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\Vpdiag.pch"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\Vpdiag.bsc"
	-@erase ".\Debug\vputil.sbr"
	-@erase ".\Debug\vpdiagdl.sbr"
	-@erase ".\Debug\TcpDlg.sbr"
	-@erase ".\Debug\StdAfx.sbr"
	-@erase ".\Debug\NbDlg.sbr"
	-@erase ".\Debug\net.sbr"
	-@erase ".\Debug\Vpwinsk.sbr"
	-@erase ".\Debug\vpdiag.sbr"
	-@erase ".\Debug\Vpdiag.exe"
	-@erase ".\Debug\vputil.obj"
	-@erase ".\Debug\vpdiagdl.obj"
	-@erase ".\Debug\TcpDlg.obj"
	-@erase ".\Debug\StdAfx.obj"
	-@erase ".\Debug\NbDlg.obj"
	-@erase ".\Debug\net.obj"
	-@erase ".\Debug\Vpwinsk.obj"
	-@erase ".\Debug\vpdiag.obj"
	-@erase ".\Debug\vpdiag.res"
	-@erase ".\Debug\Vpdiag.ilk"
	-@erase ".\Debug\Vpdiag.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NOVPTRACE" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /D "NOVPTRACE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vpdiag.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vpdiag.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vpdiag.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/vputil.sbr" \
	"$(INTDIR)/vpdiagdl.sbr" \
	"$(INTDIR)/TcpDlg.sbr" \
	"$(INTDIR)/StdAfx.sbr" \
	"$(INTDIR)/NbDlg.sbr" \
	"$(INTDIR)/net.sbr" \
	"$(INTDIR)/Vpwinsk.sbr" \
	"$(INTDIR)/vpdiag.sbr"

"$(OUTDIR)\Vpdiag.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 netapi32.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=netapi32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/Vpdiag.pdb" /debug /machine:I386 /out:"$(OUTDIR)/Vpdiag.exe" 
LINK32_OBJS= \
	"$(INTDIR)/vputil.obj" \
	"$(INTDIR)/vpdiagdl.obj" \
	"$(INTDIR)/TcpDlg.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/NbDlg.obj" \
	"$(INTDIR)/net.obj" \
	"$(INTDIR)/Vpwinsk.obj" \
	"$(INTDIR)/vpdiag.obj" \
	"$(INTDIR)/vpdiag.res"

"$(OUTDIR)\Vpdiag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "vpdiag - Win32 Release"
# Name "vpdiag - Win32 Debug"

!IF  "$(CFG)" == "vpdiag - Win32 Release"

!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "vpdiag - Win32 Release"

!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpdiag.cpp
DEP_CPP_VPDIA=\
	".\StdAfx.h"\
	".\vpdiag.h"\
	".\vpdiagdl.h"\
	".\NbDlg.h"\
	".\TcpDlg.h"\
	".\net.h"\
	

!IF  "$(CFG)" == "vpdiag - Win32 Release"


"$(INTDIR)\vpdiag.obj" : $(SOURCE) $(DEP_CPP_VPDIA) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"


"$(INTDIR)\vpdiag.obj" : $(SOURCE) $(DEP_CPP_VPDIA) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"

"$(INTDIR)\vpdiag.sbr" : $(SOURCE) $(DEP_CPP_VPDIA) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpdiagdl.cpp
DEP_CPP_VPDIAG=\
	".\StdAfx.h"\
	".\vpdiag.h"\
	".\vpdiagdl.h"\
	".\NbDlg.h"\
	".\TcpDlg.h"\
	".\net.h"\
	

!IF  "$(CFG)" == "vpdiag - Win32 Release"


"$(INTDIR)\vpdiagdl.obj" : $(SOURCE) $(DEP_CPP_VPDIAG) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"


"$(INTDIR)\vpdiagdl.obj" : $(SOURCE) $(DEP_CPP_VPDIAG) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"

"$(INTDIR)\vpdiagdl.sbr" : $(SOURCE) $(DEP_CPP_VPDIAG) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "vpdiag - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS"\
 /D "NOVPTRACE" /Fp"$(INTDIR)/Vpdiag.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vpdiag.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /D "NOVPTRACE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vpdiag.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vpdiag.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpdiag.rc
DEP_RSC_VPDIAG_=\
	".\res\vpdiag.ico"\
	".\res\vpdiag.rc2"\
	

"$(INTDIR)\vpdiag.res" : $(SOURCE) $(DEP_RSC_VPDIAG_) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\Vdmplay\Vpwinsk.cpp

!IF  "$(CFG)" == "vpdiag - Win32 Release"

DEP_CPP_VPWIN=\
	".\..\stdafx.h"\
	".\..\vputil.h"\
	".\..\vpwinsk.h"\
	".\..\vdmplay.h"\
	".\..\vplist.h"\
	

"$(INTDIR)\Vpwinsk.obj" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

DEP_CPP_VPWIN=\
	".\..\stdafx.h"\
	".\..\vputil.h"\
	".\..\vpwinsk.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Vpwinsk.obj" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"
   $(BuildCmds)

"$(INTDIR)\Vpwinsk.sbr" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\net.cpp

!IF  "$(CFG)" == "vpdiag - Win32 Release"

DEP_CPP_NET_C=\
	".\StdAfx.h"\
	".\net.h"\
	".\..\vpwinsk.h"\
	

"$(INTDIR)\net.obj" : $(SOURCE) $(DEP_CPP_NET_C) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

DEP_CPP_NET_C=\
	".\StdAfx.h"\
	".\net.h"\
	".\..\vpwinsk.h"\
	
NODEP_CPP_NET_C=\
	".\a"\
	

"$(INTDIR)\net.obj" : $(SOURCE) $(DEP_CPP_NET_C) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"

"$(INTDIR)\net.sbr" : $(SOURCE) $(DEP_CPP_NET_C) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NbDlg.cpp
DEP_CPP_NBDLG=\
	".\StdAfx.h"\
	".\vpdiag.h"\
	".\NbDlg.h"\
	".\net.h"\
	".\..\vputil.h"\
	

!IF  "$(CFG)" == "vpdiag - Win32 Release"


"$(INTDIR)\NbDlg.obj" : $(SOURCE) $(DEP_CPP_NBDLG) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"


"$(INTDIR)\NbDlg.obj" : $(SOURCE) $(DEP_CPP_NBDLG) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"

"$(INTDIR)\NbDlg.sbr" : $(SOURCE) $(DEP_CPP_NBDLG) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TcpDlg.cpp
DEP_CPP_TCPDL=\
	".\StdAfx.h"\
	".\vpdiag.h"\
	".\TcpDlg.h"\
	".\net.h"\
	

!IF  "$(CFG)" == "vpdiag - Win32 Release"


"$(INTDIR)\TcpDlg.obj" : $(SOURCE) $(DEP_CPP_TCPDL) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"


"$(INTDIR)\TcpDlg.obj" : $(SOURCE) $(DEP_CPP_TCPDL) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"

"$(INTDIR)\TcpDlg.sbr" : $(SOURCE) $(DEP_CPP_TCPDL) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\Vdmplay\vputil.cpp

!IF  "$(CFG)" == "vpdiag - Win32 Release"

DEP_CPP_VPUTI=\
	".\..\stdafx.h"\
	".\..\vputil.h"\
	".\..\vdmplay.h"\
	".\..\vplist.h"\
	

"$(INTDIR)\vputil.obj" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vpdiag - Win32 Debug"

DEP_CPP_VPUTI=\
	".\..\stdafx.h"\
	".\..\vputil.h"\
	".\..\vdmplay.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\vputil.obj" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"
   $(BuildCmds)

"$(INTDIR)\vputil.sbr" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"\
 "$(INTDIR)\Vpdiag.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
