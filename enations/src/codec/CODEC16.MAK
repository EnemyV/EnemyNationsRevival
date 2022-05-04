# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=CoDec16 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to CoDec16 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "CoDec16 - Win32 Release" && "$(CFG)" !=\
 "CoDec16 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "CoDec16.mak" CFG="CoDec16 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CoDec16 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CoDec16 - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "CoDec16 - Win32 Release"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CoDec16 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\CoDec16.exe"

CLEAN : 
	-@erase ".\Release\CoDec16.exe"
	-@erase ".\Release\Wavefile.obj"
	-@erase ".\Release\Acmutil.obj"
	-@erase ".\Release\Toadpcm.obj"
	-@erase ".\Release\Assert.obj"
	-@erase ".\Release\Debug.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/CoDec16.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/CoDec16.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib msacm32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib winmm.lib msacm32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/CoDec16.pdb" /machine:I386 /out:"$(OUTDIR)/CoDec16.exe" 
LINK32_OBJS= \
	".\Release\Wavefile.obj" \
	".\Release\Acmutil.obj" \
	".\Release\Toadpcm.obj" \
	".\Release\Assert.obj" \
	".\Release\Debug.obj"

"$(OUTDIR)\CoDec16.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CoDec16 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\CoDec16.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\CoDec16.exe"
	-@erase ".\Debug\Acmutil.obj"
	-@erase ".\Debug\Assert.obj"
	-@erase ".\Debug\Debug.obj"
	-@erase ".\Debug\Wavefile.obj"
	-@erase ".\Debug\Toadpcm.obj"
	-@erase ".\Debug\CoDec16.ilk"
	-@erase ".\Debug\CoDec16.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/CoDec16.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/CoDec16.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib msacm32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib winmm.lib msacm32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/CoDec16.pdb" /debug /machine:I386 /out:"$(OUTDIR)/CoDec16.exe" 
LINK32_OBJS= \
	".\Debug\Acmutil.obj" \
	".\Debug\Assert.obj" \
	".\Debug\Debug.obj" \
	".\Debug\Wavefile.obj" \
	".\Debug\Toadpcm.obj"

"$(OUTDIR)\CoDec16.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "CoDec16 - Win32 Release"
# Name "CoDec16 - Win32 Debug"

!IF  "$(CFG)" == "CoDec16 - Win32 Release"

!ELSEIF  "$(CFG)" == "CoDec16 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Toadpcm.cpp

!IF  "$(CFG)" == "CoDec16 - Win32 Release"

DEP_CPP_TOADP=\
	".\assert.h"\
	".\debug.h"\
	".\wavefile.h"\
	".\acmutil.h"\
	

"$(INTDIR)\Toadpcm.obj" : $(SOURCE) $(DEP_CPP_TOADP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "CoDec16 - Win32 Debug"

DEP_CPP_TOADP=\
	".\assert.h"\
	".\debug.h"\
	".\wavefile.h"\
	".\acmutil.h"\
	
NODEP_CPP_TOADP=\
	".\size"\
	

"$(INTDIR)\Toadpcm.obj" : $(SOURCE) $(DEP_CPP_TOADP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Wavefile.cpp
DEP_CPP_WAVEF=\
	".\assert.h"\
	".\debug.h"\
	".\wavefile.h"\
	

"$(INTDIR)\Wavefile.obj" : $(SOURCE) $(DEP_CPP_WAVEF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Assert.c
DEP_CPP_ASSER=\
	".\assert.h"\
	

"$(INTDIR)\Assert.obj" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Debug.c

"$(INTDIR)\Debug.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Acmutil.cpp

!IF  "$(CFG)" == "CoDec16 - Win32 Release"

DEP_CPP_ACMUT=\
	".\assert.h"\
	".\debug.h"\
	".\acmutil.h"\
	

"$(INTDIR)\Acmutil.obj" : $(SOURCE) $(DEP_CPP_ACMUT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "CoDec16 - Win32 Debug"

DEP_CPP_ACMUT=\
	".\assert.h"\
	".\debug.h"\
	".\acmutil.h"\
	
NODEP_CPP_ACMUT=\
	".\m_fmt"\
	

"$(INTDIR)\Acmutil.obj" : $(SOURCE) $(DEP_CPP_ACMUT) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
