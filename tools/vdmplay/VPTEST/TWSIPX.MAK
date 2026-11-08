# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Twsipx - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Twsipx - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Twsipx - Win32 Release" && "$(CFG)" != "Twsipx - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Twsipx.mak" CFG="Twsipx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Twsipx - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Twsipx - Win32 Debug" (based on "Win32 (x86) Console Application")
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
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Twsipx - Win32 Release"

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

ALL : "$(OUTDIR)\Twsipx.exe" "$(OUTDIR)\Twsipx.bsc"

CLEAN : 
	-@erase ".\WinRel\Twsipx.exe"
	-@erase ".\WinRel\twsipx.obj"
	-@erase ".\WinRel\wsipxnet.obj"
	-@erase ".\WinRel\Twsipx.bsc"
	-@erase ".\WinRel\twsipx.sbr"
	-@erase ".\WinRel\wsipxnet.sbr"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/Twsipx.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Twsipx.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/twsipx.sbr" \
	"$(INTDIR)/wsipxnet.sbr"

"$(OUTDIR)\Twsipx.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/Twsipx.pdb" /machine:I386 /out:"$(OUTDIR)/Twsipx.exe" 
LINK32_OBJS= \
	"$(INTDIR)/twsipx.obj" \
	"$(INTDIR)/wsipxnet.obj"

"$(OUTDIR)\Twsipx.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Twsipx - Win32 Debug"

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

ALL : "$(OUTDIR)\Twsipx.exe" "$(OUTDIR)\Twsipx.bsc"

CLEAN : 
	-@erase ".\WinDebug\Twsipx.exe"
	-@erase ".\WinDebug\twsipx.obj"
	-@erase ".\WinDebug\wsipxnet.obj"
	-@erase ".\WinDebug\Twsipx.ilk"
	-@erase ".\WinDebug\Twsipx.pdb"
	-@erase ".\WinDebug\Twsipx.bsc"
	-@erase ".\WinDebug\twsipx.sbr"
	-@erase ".\WinDebug\wsipxnet.sbr"
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\vc40.idb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /ML /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Twsipx.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Twsipx.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/twsipx.sbr" \
	"$(INTDIR)/wsipxnet.sbr"

"$(OUTDIR)\Twsipx.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/Twsipx.pdb" /debug /machine:I386 /out:"$(OUTDIR)/Twsipx.exe" 
LINK32_OBJS= \
	"$(INTDIR)/twsipx.obj" \
	"$(INTDIR)/wsipxnet.obj"

"$(OUTDIR)\Twsipx.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "Twsipx - Win32 Release"
# Name "Twsipx - Win32 Debug"

!IF  "$(CFG)" == "Twsipx - Win32 Release"

!ELSEIF  "$(CFG)" == "Twsipx - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\twsipx.cpp

"$(INTDIR)\twsipx.obj" : $(SOURCE) "$(INTDIR)"

"$(INTDIR)\twsipx.sbr" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\wsipxnet.cpp

"$(INTDIR)\wsipxnet.obj" : $(SOURCE) "$(INTDIR)"

"$(INTDIR)\wsipxnet.sbr" : $(SOURCE) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
