# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=comptest - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to comptest - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "comptest - Win32 Release" && "$(CFG)" !=\
 "comptest - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "compit.MAK" CFG="comptest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "comptest - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "comptest - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "comptest - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "comptest - Win32 Release"

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

ALL : "$(OUTDIR)\compit.exe"

CLEAN : 
	-@erase ".\Release\compit.exe"
	-@erase ".\Release\Stdafx.obj"
	-@erase ".\Release\main.obj"
	-@erase ".\Release\vc40.pdb"
	-@erase ".\Release\compit.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /G5 /MD /W4 /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /G5 /MD /W4 /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D\
 "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/compit.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/compit.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wind40.lib winmm.lib setargv.obj /nologo /subsystem:console /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=wind40.lib winmm.lib setargv.obj /nologo /subsystem:console\
 /incremental:no /pdb:"$(OUTDIR)/compit.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/compit.exe" 
LINK32_OBJS= \
	"$(INTDIR)/Stdafx.obj" \
	"$(INTDIR)/main.obj"

"$(OUTDIR)\compit.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "comptest - Win32 Debug"

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

ALL : "$(OUTDIR)\compit.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\compit.exe"
	-@erase ".\Debug\main.obj"
	-@erase ".\Debug\Stdafx.obj"
	-@erase ".\Debug\compit.ilk"
	-@erase ".\Debug\compit.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /G5 /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /G5 /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/compit.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/compit.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wind40d.lib winmm.lib setargv.obj /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=wind40d.lib winmm.lib setargv.obj /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/compit.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/compit.exe" 
LINK32_OBJS= \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/Stdafx.obj"

"$(OUTDIR)\compit.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "comptest - Win32 Release"
# Name "comptest - Win32 Debug"

!IF  "$(CFG)" == "comptest - Win32 Release"

!ELSEIF  "$(CFG)" == "comptest - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Stdafx.cpp
DEP_CPP_STDAF=\
	".\stdafx.h"\
	

"$(INTDIR)\Stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.cpp
DEP_CPP_MAIN_=\
	".\stdafx.h"\
	{$(INCLUDE)}"\CODEC.H"\
	{$(INCLUDE)}"\BPECODEC.H"\
	{$(INCLUDE)}"\LZSSCODE.H"\
	{$(INCLUDE)}"\HUFFMANC.H"\
	".\lzwcodec.h"\
	{$(INCLUDE)}"\BITBUFFE.H"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
