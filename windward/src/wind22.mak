# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "wind22.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
CPP=cl.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel22"
# PROP Intermediate_Dir "WinRel22"
OUTDIR=.\WinRel22
INTDIR=.\WinRel22

ALL : $(OUTDIR)/wind22.lib $(OUTDIR)/wind22.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Zi /YX"stdafx.h" /O2 /I "..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_CHEAT" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /YX"stdafx.h" /O2 /I "..\include" /D "NDEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_CHEAT" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"wind22.pch" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"wind22.pdb" /c 
CPP_OBJS=.\WinRel22/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"wind22.bsc" 
BSC32_SBRS= \
	$(INTDIR)/MSG_BOX.SBR \
	$(INTDIR)/THREADS.SBR \
	$(INTDIR)/STDAFX.SBR \
	$(INTDIR)/GLOBAL.SBR \
	$(INTDIR)/MMIO.SBR \
	$(INTDIR)/INIT.SBR \
	$(INTDIR)/RAND.SBR \
	$(INTDIR)/DAVIDINL.SBR \
	$(INTDIR)/MUSIC.SBR \
	$(INTDIR)/STREXT.SBR \
	$(INTDIR)/APPPALET.SBR \
	$(INTDIR)/SCANLIST.SBR \
	$(INTDIR)/DATAFILE.SBR \
	$(INTDIR)/FIXPOINT.SBR \
	$(INTDIR)/DIBWND.SBR \
	$(INTDIR)/BLT.SBR \
	$(INTDIR)/DIB.SBR \
	$(INTDIR)/BTREE.SBR \
	$(INTDIR)/wndbase.sbr \
	$(INTDIR)/FLCANIM.SBR \
	$(INTDIR)/FLCCTRL.SBR

$(OUTDIR)/wind22.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=lib.exe
# ADD BASE LIB32 /NOLOGO
# ADD LIB32 /NOLOGO
LIB32_FLAGS=/NOLOGO /OUT:$(OUTDIR)\"wind22.lib" 
DEF_FLAGS=
DEF_FILE=
LIB32_OBJS= \
	$(INTDIR)/MSG_BOX.OBJ \
	$(INTDIR)/THREADS.OBJ \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/GLOBAL.OBJ \
	$(INTDIR)/MMIO.OBJ \
	$(INTDIR)/INIT.OBJ \
	$(INTDIR)/RAND.OBJ \
	$(INTDIR)/DAVIDINL.OBJ \
	$(INTDIR)/MUSIC.OBJ \
	$(INTDIR)/STREXT.OBJ \
	$(INTDIR)/APPPALET.OBJ \
	$(INTDIR)/SCANLIST.OBJ \
	$(INTDIR)/DATAFILE.OBJ \
	$(INTDIR)/FIXPOINT.OBJ \
	$(INTDIR)/DIBWND.OBJ \
	$(INTDIR)/BLT.OBJ \
	$(INTDIR)/DIB.OBJ \
	$(INTDIR)/BTREE.OBJ \
	$(INTDIR)/wndbase.obj \
	$(INTDIR)/FLCANIM.OBJ \
	$(INTDIR)/FLCCTRL.OBJ

$(OUTDIR)/wind22.lib : $(OUTDIR)  $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDeb22"
# PROP Intermediate_Dir "WinDeb22"
OUTDIR=.\WinDeb22
INTDIR=.\WinDeb22

ALL : $(OUTDIR)/Wind22d.lib $(OUTDIR)/wind22.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /Z7 /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Zi /YX"stdafx.h" /Od /I "..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_CHEAT" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /YX"stdafx.h" /Od /I "..\include" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_CHEAT" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"wind22.pch" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"wind22.pdb" /c 
CPP_OBJS=.\WinDeb22/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"wind22.bsc" 
BSC32_SBRS= \
	$(INTDIR)/MSG_BOX.SBR \
	$(INTDIR)/THREADS.SBR \
	$(INTDIR)/STDAFX.SBR \
	$(INTDIR)/GLOBAL.SBR \
	$(INTDIR)/MMIO.SBR \
	$(INTDIR)/INIT.SBR \
	$(INTDIR)/RAND.SBR \
	$(INTDIR)/DAVIDINL.SBR \
	$(INTDIR)/MUSIC.SBR \
	$(INTDIR)/STREXT.SBR \
	$(INTDIR)/APPPALET.SBR \
	$(INTDIR)/SCANLIST.SBR \
	$(INTDIR)/DATAFILE.SBR \
	$(INTDIR)/FIXPOINT.SBR \
	$(INTDIR)/DIBWND.SBR \
	$(INTDIR)/BLT.SBR \
	$(INTDIR)/DIB.SBR \
	$(INTDIR)/BTREE.SBR \
	$(INTDIR)/wndbase.sbr \
	$(INTDIR)/FLCANIM.SBR \
	$(INTDIR)/FLCCTRL.SBR

$(OUTDIR)/wind22.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=lib.exe
# ADD BASE LIB32 /NOLOGO
# ADD LIB32 /NOLOGO /OUT:"WinDeb22\Wind22d.lib"
LIB32_FLAGS=/NOLOGO /OUT:"WinDeb22\Wind22d.lib" 
DEF_FLAGS=
DEF_FILE=
LIB32_OBJS= \
	$(INTDIR)/MSG_BOX.OBJ \
	$(INTDIR)/THREADS.OBJ \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/GLOBAL.OBJ \
	$(INTDIR)/MMIO.OBJ \
	$(INTDIR)/INIT.OBJ \
	$(INTDIR)/RAND.OBJ \
	$(INTDIR)/DAVIDINL.OBJ \
	$(INTDIR)/MUSIC.OBJ \
	$(INTDIR)/STREXT.OBJ \
	$(INTDIR)/APPPALET.OBJ \
	$(INTDIR)/SCANLIST.OBJ \
	$(INTDIR)/DATAFILE.OBJ \
	$(INTDIR)/FIXPOINT.OBJ \
	$(INTDIR)/DIBWND.OBJ \
	$(INTDIR)/BLT.OBJ \
	$(INTDIR)/DIB.OBJ \
	$(INTDIR)/BTREE.OBJ \
	$(INTDIR)/wndbase.obj \
	$(INTDIR)/FLCANIM.OBJ \
	$(INTDIR)/FLCCTRL.OBJ

$(OUTDIR)/Wind22d.lib : $(OUTDIR)  $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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

SOURCE=.\MSG_BOX.CPP
DEP_MSG_B=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/MSG_BOX.OBJ :  $(SOURCE)  $(DEP_MSG_B) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\THREADS.CPP
DEP_THREA=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/THREADS.OBJ :  $(SOURCE)  $(DEP_THREA) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\STDAFX.CPP
DEP_STDAF=\
	.\STDAFX.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\

$(INTDIR)/STDAFX.OBJ :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\GLOBAL.CPP
DEP_GLOBA=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/GLOBAL.OBJ :  $(SOURCE)  $(DEP_GLOBA) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MMIO.CPP
DEP_MMIO_=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/MMIO.OBJ :  $(SOURCE)  $(DEP_MMIO_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\INIT.CPP
DEP_INIT_=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/INIT.OBJ :  $(SOURCE)  $(DEP_INIT_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RAND.CPP
DEP_RAND_=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\RAND.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/RAND.OBJ :  $(SOURCE)  $(DEP_RAND_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DAVIDINL.CPP
DEP_DAVID=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\DAVE32UT.H

$(INTDIR)/DAVIDINL.OBJ :  $(SOURCE)  $(DEP_DAVID) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MUSIC.CPP
DEP_MUSIC=\
	.\STDAFX.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\RAND.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/MUSIC.OBJ :  $(SOURCE)  $(DEP_MUSIC) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\STREXT.CPP
DEP_STREX=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/STREXT.OBJ :  $(SOURCE)  $(DEP_STREX) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\APPPALET.CPP
DEP_APPPA=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/APPPALET.OBJ :  $(SOURCE)  $(DEP_APPPA) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SCANLIST.CPP
DEP_SCANL=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/SCANLIST.OBJ :  $(SOURCE)  $(DEP_SCANL) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DATAFILE.CPP
DEP_DATAF=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/DATAFILE.OBJ :  $(SOURCE)  $(DEP_DATAF) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\FIXPOINT.CPP
DEP_FIXPO=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/FIXPOINT.OBJ :  $(SOURCE)  $(DEP_FIXPO) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Windward.RC
# End Source File
################################################################################
# Begin Source File

SOURCE=.\DIBWND.CPP
DEP_DIBWN=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/DIBWND.OBJ :  $(SOURCE)  $(DEP_DIBWN) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BLT.CPP
DEP_BLT_C=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/BLT.OBJ :  $(SOURCE)  $(DEP_BLT_C) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DIB.CPP
DEP_DIB_C=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/DIB.OBJ :  $(SOURCE)  $(DEP_DIB_C) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BTREE.CPP
DEP_BTREE=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\BTREE.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/BTREE.OBJ :  $(SOURCE)  $(DEP_BTREE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\wndbase.cpp
DEP_WNDBA=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/wndbase.obj :  $(SOURCE)  $(DEP_WNDBA) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\FLCANIM.CPP
DEP_FLCAN=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\FLCANIM.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/FLCANIM.OBJ :  $(SOURCE)  $(DEP_FLCAN) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\FLCCTRL.CPP
DEP_FLCCT=\
	.\STDAFX.H\
	C:\Source\windward\include\_WINDWRD.H\
	C:\Source\windward\include\FLCCTRL.H\
	C:\Source\windward\include\INIT.H\
	C:\Source\windward\include\GAMES.H\
	C:\Source\Include\include\ddraw.h\
	C:\Source\Include\include\dsound.h\
	C:\WING\INCLUDE\WING.H\
	C:\Source\tools\mss\MSS.H\
	C:\Source\windward\include\WINDWARD.H\
	C:\Source\windward\include\THIELEN.H\
	C:\Source\windward\include\FLCANIM.H\
	C:\Source\windward\include\DIBWND.H\
	C:\Source\windward\include\PTR.H\
	C:\Source\windward\include\_ERROR.H\
	C:\Source\windward\include\_HELP.H\
	C:\Source\windward\include\DATAFILE.H\
	C:\Source\windward\include\FIXPOINT.H\
	C:\Source\windward\include\MMIO.H\
	C:\Source\windward\include\music.h\
	C:\Source\windward\include\RAND.H\
	C:\Source\windward\include\SCANLIST.H\
	C:\Source\windward\include\THREADS.H\
	C:\Source\windward\include\BLT.H\
	C:\Source\windward\include\DIB.H\
	C:\Source\windward\include\wndbase.h\
	C:\Source\windward\include\_msgs.h\
	C:\Source\windward\include\WINDWARD.INL\
	C:\Source\windward\include\DAVE32UT.H\
	C:\Source\windward\include\THIELEN.INL\
	C:\Source\windward\include\mmio.inl

$(INTDIR)/FLCCTRL.OBJ :  $(SOURCE)  $(DEP_FLCCT) $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
