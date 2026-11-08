# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=makeriff - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to makeriff - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "makeriff - Win32 Release" && "$(CFG)" !=\
 "makeriff - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "makeriff.mak" CFG="makeriff - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "makeriff - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "makeriff - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "makeriff - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "makeriff - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\makeriff.exe"

CLEAN : 
	-@erase ".\Release\makeriff.exe"
	-@erase ".\Release\makeriff.obj"
	-@erase ".\Release\rifffile.obj"
	-@erase ".\Release\Stdafx.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /G5 /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /G5 /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/makeriff.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/makeriff.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 /nologo /subsystem:console /machine:I386
LINK32_FLAGS=/nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/makeriff.pdb" /machine:I386 /out:"$(OUTDIR)/makeriff.exe" 
LINK32_OBJS= \
	"$(INTDIR)/makeriff.obj" \
	"$(INTDIR)/rifffile.obj" \
	"$(INTDIR)/Stdafx.obj"

"$(OUTDIR)\makeriff.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "makeriff - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\makeriff.exe" "$(OUTDIR)\makeriff.bsc"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\makeriff.bsc"
	-@erase ".\Debug\Stdafx.sbr"
	-@erase ".\Debug\makeriff.sbr"
	-@erase ".\Debug\rifffile.sbr"
	-@erase ".\Debug\makeriff.exe"
	-@erase ".\Debug\rifffile.obj"
	-@erase ".\Debug\Stdafx.obj"
	-@erase ".\Debug\makeriff.obj"
	-@erase ".\Debug\makeriff.ilk"
	-@erase ".\Debug\makeriff.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /G5 /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /FR /YX /c
CPP_PROJ=/nologo /G5 /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/makeriff.pch"\
 /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/makeriff.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/Stdafx.sbr" \
	"$(INTDIR)/makeriff.sbr" \
	"$(INTDIR)/rifffile.sbr"

"$(OUTDIR)\makeriff.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/makeriff.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/makeriff.exe" 
LINK32_OBJS= \
	"$(INTDIR)/rifffile.obj" \
	"$(INTDIR)/Stdafx.obj" \
	"$(INTDIR)/makeriff.obj"

"$(OUTDIR)\makeriff.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "makeriff - Win32 Release"
# Name "makeriff - Win32 Debug"

!IF  "$(CFG)" == "makeriff - Win32 Release"

!ELSEIF  "$(CFG)" == "makeriff - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\makeriff.cpp
DEP_CPP_MAKER=\
	".\Stdafx.h"\
	".\rifffile.h"\
	

!IF  "$(CFG)" == "makeriff - Win32 Release"


"$(INTDIR)\makeriff.obj" : $(SOURCE) $(DEP_CPP_MAKER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "makeriff - Win32 Debug"


"$(INTDIR)\makeriff.obj" : $(SOURCE) $(DEP_CPP_MAKER) "$(INTDIR)"

"$(INTDIR)\makeriff.sbr" : $(SOURCE) $(DEP_CPP_MAKER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rifffile.cpp

!IF  "$(CFG)" == "makeriff - Win32 Release"

DEP_CPP_RIFFF=\
	".\Stdafx.h"\
	".\rifffile.h"\
	

"$(INTDIR)\rifffile.obj" : $(SOURCE) $(DEP_CPP_RIFFF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "makeriff - Win32 Debug"

DEP_CPP_RIFFF=\
	".\Stdafx.h"\
	".\rifffile.h"\
	
NODEP_CPP_RIFFF=\
	".\PeekNextChar()"\
	

"$(INTDIR)\rifffile.obj" : $(SOURCE) $(DEP_CPP_RIFFF) "$(INTDIR)"

"$(INTDIR)\rifffile.sbr" : $(SOURCE) $(DEP_CPP_RIFFF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Stdafx.cpp
DEP_CPP_STDAF=\
	".\Stdafx.h"\
	

!IF  "$(CFG)" == "makeriff - Win32 Release"


"$(INTDIR)\Stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "makeriff - Win32 Debug"


"$(INTDIR)\Stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

"$(INTDIR)\Stdafx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
