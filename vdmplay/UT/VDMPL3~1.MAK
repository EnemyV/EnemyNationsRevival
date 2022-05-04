# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Vdmpl32 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Vdmpl32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Vdmpl32 - Win32 Release" && "$(CFG)" !=\
 "Vdmpl32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vdmpl32ds.mak" CFG="Vdmpl32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vdmpl32 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Vdmpl32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "Vdmpl32 - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "Vdmpl32 - Win32 Release"

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

ALL : "$(OUTDIR)\vdmplay.dll" ".\Vdmpl32ds.bsc"

CLEAN : 
	-@erase ".\Vdmpl32ds.bsc"
	-@erase ".\WinRel\VDMPL32.SBR"
	-@erase ".\WinRel\vdmplay.dll"
	-@erase ".\WinRel\VDMPL32.OBJ"
	-@erase ".\WinRel\VDMPLAY.res"
	-@erase ".\vdmplay.lib"
	-@erase ".\vdmplay.exp"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".." /I "." /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vdmpl32ds.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /i ".." /d "NDEBUG" /d "THUNK"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/VDMPLAY.res" /i ".." /d "NDEBUG" /d "THUNK" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vdmpl32ds.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/VDMPL32.SBR"

".\Vdmpl32ds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /force /out:"WinRel\vdmplay.dll"
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/vdmplay.pdb" /machine:I386 /def:".\VDMPL32.DEF" /force\
 /out:"$(OUTDIR)/WinRel\vdmplay.dll" /implib:"$(OUTDIR)/vdmplay.lib" 
DEF_FILE= \
	".\VDMPL32.DEF"
LINK32_OBJS= \
	"$(INTDIR)/VDMPL32.OBJ" \
	"$(INTDIR)/VDMPLAY.res" \
	".\W32SUT32.LIB"

"$(OUTDIR)\vdmplay.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Vdmpl32 - Win32 Debug"

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

ALL : ".\WinDebug\vdmplay.dll" ".\Vdmpl32ds.bsc"

CLEAN : 
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\vc40.idb"
	-@erase ".\Vdmpl32ds.bsc"
	-@erase ".\WinDebug\VDMPL32.SBR"
	-@erase ".\WinDebug\vdmplay.dll"
	-@erase ".\WinDebug\VDMPL32.OBJ"
	-@erase ".\WinDebug\VDMPLAY.res"
	-@erase ".\WinDebug\vdmplay.ilk"
	-@erase ".\vdmplay.lib"
	-@erase ".\vdmplay.exp"
	-@erase ".\vdmplay.pdb"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "." /D "WIN32" /D "_DEBUG"\
 /D "_WINDOWS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vdmpl32ds.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /i ".." /d "_DEBUG" /d "THUNK"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/VDMPLAY.res" /i ".." /d "_DEBUG" /d "THUNK" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vdmpl32ds.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/VDMPL32.SBR"

".\Vdmpl32ds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /force /out:"WinDebug\vdmplay.dll"
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/vdmplay.pdb" /debug /machine:I386 /def:".\VDMPL32.DEF" /force\
 /out:"$(OUTDIR)/WinDebug\vdmplay.dll" /implib:"$(OUTDIR)/vdmplay.lib" 
DEF_FILE= \
	".\VDMPL32.DEF"
LINK32_OBJS= \
	"$(INTDIR)/VDMPL32.OBJ" \
	"$(INTDIR)/VDMPLAY.res" \
	".\W32SUT32.LIB"

".\WinDebug\vdmplay.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "Vdmpl32 - Win32 Release"
# Name "Vdmpl32 - Win32 Debug"

!IF  "$(CFG)" == "Vdmpl32 - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdmpl32 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\VDMPL32.CPP

!IF  "$(CFG)" == "Vdmpl32 - Win32 Release"

DEP_CPP_VDMPL=\
	".\Stdafx.h"\
	".\W32sut.h"\
	".\vdmplut.h"\
	

"$(INTDIR)\VDMPL32.OBJ" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"

"$(INTDIR)\VDMPL32.SBR" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmpl32 - Win32 Debug"

DEP_CPP_VDMPL=\
	".\Stdafx.h"\
	".\W32sut.h"\
	".\vdmplut.h"\
	".\..\nb30.h"\
	".\..\generic.h"\
	

"$(INTDIR)\VDMPL32.OBJ" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"

"$(INTDIR)\VDMPL32.SBR" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\W32SUT32.LIB

!IF  "$(CFG)" == "Vdmpl32 - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdmpl32 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPL32.DEF

!IF  "$(CFG)" == "Vdmpl32 - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdmpl32 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\PROJECTS\VDMPLAY\VDMPLAY.RC
DEP_RSC_VDMPLA=\
	".\..\icon1.ico"\
	

!IF  "$(CFG)" == "Vdmpl32 - Win32 Release"


"$(INTDIR)\VDMPLAY.res" : $(SOURCE) $(DEP_RSC_VDMPLA) "$(INTDIR)"
   $(RSC) /l 0x809 /fo"$(INTDIR)/VDMPLAY.res" /i ".." /i "\PROJECTS\VDMPLAY" /d\
 "NDEBUG" /d "THUNK" $(SOURCE)


!ELSEIF  "$(CFG)" == "Vdmpl32 - Win32 Debug"


"$(INTDIR)\VDMPLAY.res" : $(SOURCE) $(DEP_RSC_VDMPLA) "$(INTDIR)"
   $(RSC) /l 0x809 /fo"$(INTDIR)/VDMPLAY.res" /i ".." /i "\PROJECTS\VDMPLAY" /d\
 "_DEBUG" /d "THUNK" $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
