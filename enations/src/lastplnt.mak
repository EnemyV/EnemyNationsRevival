# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug" && "$(CFG)" !=\
 "Profile" && "$(CFG)" != "Release No Inline"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "LASTPLNT.MAK" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Profile" (based on "Win32 (x86) Application")
!MESSAGE "Release No Inline" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Release"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : $(OUTDIR)/ENations.exe $(OUTDIR)/LASTPLNT.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /G3 /MT /W4 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CHEAT" /D "_MBCS" /FR /c
# ADD CPP /nologo /Zp4 /MD /W4 /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /Yu"STDAFX.H" /c
CPP_PROJ=/nologo /Zp4 /MD /W4 /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /Fp$(OUTDIR)/"LASTPLNT.pch"\
 /Yu"STDAFX.H" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"LASTPLNT.pdb" /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "f:\windward\lib" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"LASTPLNT.res" /i "f:\windward\lib" /d "NDEBUG"\
 /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"LASTPLNT.bsc" 

$(OUTDIR)/LASTPLNT.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 ldavecw.lib shlw16m.lib davenet.lib wing.lib ctl3dv2.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x10240 /SUBSYSTEM:windows /MACHINE:IX86
# ADD LINK32 wind22.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x10240 /SUBSYSTEM:windows /INCREMENTAL:yes /PDB:"WinRel/ENations.pdb" /DEBUG /MACHINE:IX86 /OUT:"WinRel/ENations.exe"
# SUBTRACT LINK32 /PROFILE /PDB:none /MAP /NODEFAULTLIB
LINK32_FLAGS=wind22.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO\
 /STACK:0x10240 /SUBSYSTEM:windows /INCREMENTAL:yes /PDB:"WinRel/ENations.pdb"\
 /DEBUG /MACHINE:IX86 /DEF:".\LASTPLNT.DEF" /OUT:"WinRel/ENations.exe" 
DEF_FILE=.\LASTPLNT.DEF
LINK32_OBJS= \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/AREA.OBJ \
	$(INTDIR)/AI.OBJ \
	$(INTDIR)/NEW_UNIT.OBJ \
	$(INTDIR)/MAIN.OBJ \
	$(INTDIR)/BMBUTTON.OBJ \
	$(INTDIR)/MAINLOOP.OBJ \
	$(INTDIR)/SPRTINIT.OBJ \
	$(INTDIR)/PLAYER.OBJ \
	$(INTDIR)/UI.OBJ \
	$(INTDIR)/JOIN.OBJ \
	$(INTDIR)/SPRITE.OBJ \
	$(INTDIR)/WORLD.OBJ \
	$(INTDIR)/TERRAIN.OBJ \
	$(INTDIR)/NETAPI.OBJ \
	$(INTDIR)/UNIT.OBJ \
	$(INTDIR)/NEWWORLD.OBJ \
	$(INTDIR)/WRLDINIT.OBJ \
	$(INTDIR)/CPATHMGR.OBJ \
	$(INTDIR)/IPCCOMM.OBJ \
	$(INTDIR)/IPCSEND.OBJ \
	$(INTDIR)/IPCREAD.OBJ \
	$(INTDIR)/CHATBAR.OBJ \
	$(INTDIR)/IPCMSG.OBJ \
	$(INTDIR)/IPCCHAT.OBJ \
	$(INTDIR)/LASTPLNT.OBJ \
	$(INTDIR)/LASTPLNT.res \
	$(INTDIR)/IPCPLAY.OBJ \
	$(INTDIR)/EVENT.OBJ \
	$(INTDIR)/MINERALS.OBJ \
	$(INTDIR)/CAIUNIT.OBJ \
	$(INTDIR)/CAIMGR.OBJ \
	$(INTDIR)/CAIDATA.OBJ \
	$(INTDIR)/CAIOPFOR.OBJ \
	$(INTDIR)/CAIMSG.OBJ \
	$(INTDIR)/CAIINIT.OBJ \
	$(INTDIR)/CAIROUTE.OBJ \
	$(INTDIR)/CAICOPY.OBJ \
	$(INTDIR)/CAITASK.OBJ \
	$(INTDIR)/CAIMAP.OBJ \
	$(INTDIR)/CAIMAPUT.OBJ \
	$(INTDIR)/CAISAVLD.OBJ \
	$(INTDIR)/CAIGOAL.OBJ \
	$(INTDIR)/CAISTART.OBJ \
	$(INTDIR)/CAIGMGR.OBJ \
	$(INTDIR)/CAITMGR.OBJ \
	$(INTDIR)/CAIHEX.OBJ \
	$(INTDIR)/NETCMD.OBJ \
	$(INTDIR)/SCENARIO.OBJ \
	$(INTDIR)/CREATSIN.OBJ \
	$(INTDIR)/PICKRACE.OBJ \
	$(INTDIR)/RACEDATA.OBJ \
	$(INTDIR)/CREATMUL.OBJ \
	$(INTDIR)/CPATHMAP.OBJ \
	$(INTDIR)/RELATION.OBJ \
	$(INTDIR)/UNIT_WND.OBJ \
	$(INTDIR)/VEHMOVE.OBJ \
	$(INTDIR)/CAILOG.OBJ \
	$(INTDIR)/RESEARCH.OBJ \
	$(INTDIR)/OPTIONS.OBJ \
	$(INTDIR)/CHAT.OBJ \
	$(INTDIR)/PROJBASE.OBJ \
	$(INTDIR)/CHPROUTE.OBJ \
	$(INTDIR)/CITIZEN.OBJ \
	$(INTDIR)/NEW_GAME.OBJ \
	$(INTDIR)/CUTSCENE.OBJ \
	$(INTDIR)/ICONS.OBJ \
	$(INTDIR)/TOOLBAR.OBJ \
	$(INTDIR)/DLGFLIC.OBJ

$(OUTDIR)/ENations.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/ENations.exe $(OUTDIR)/LASTPLNT.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /MT /W4 /GX /Zi /YX /Od /Gf /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /c
# ADD CPP /nologo /Zp4 /MD /W4 /GX /Zi /Od /Ob1 /Gf /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"STDAFX.H" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /Zp4 /MD /W4 /GX /Zi /Od /Ob1 /Gf /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp$(OUTDIR)/"LASTPLNT.pch" /Yu"STDAFX.H"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"LASTPLNT.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "f:\windward\lib" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"LASTPLNT.res" /i "f:\windward\lib" /d "_DEBUG"\
 /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"LASTPLNT.bsc" 

$(OUTDIR)/LASTPLNT.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 ldavecwd.lib shlmfcmd.lib shlw16md.lib davenet.lib wing.lib ctl3dv2.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x16384 /SUBSYSTEM:windows /DEBUG /MACHINE:IX86
# ADD LINK32 wind22d.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x16384 /SUBSYSTEM:windows /PDB:"WinDebug/ENations.pdb" /DEBUG /MACHINE:IX86 /OUT:"WinDebug/ENations.exe"
# SUBTRACT LINK32 /PROFILE /PDB:none /INCREMENTAL:no /MAP
LINK32_FLAGS=wind22d.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO\
 /STACK:0x16384 /SUBSYSTEM:windows /INCREMENTAL:yes /PDB:"WinDebug/ENations.pdb"\
 /DEBUG /MACHINE:IX86 /DEF:".\LASTPLNT.DEF" /OUT:"WinDebug/ENations.exe" 
DEF_FILE=.\LASTPLNT.DEF
LINK32_OBJS= \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/AREA.OBJ \
	$(INTDIR)/AI.OBJ \
	$(INTDIR)/NEW_UNIT.OBJ \
	$(INTDIR)/MAIN.OBJ \
	$(INTDIR)/BMBUTTON.OBJ \
	$(INTDIR)/MAINLOOP.OBJ \
	$(INTDIR)/SPRTINIT.OBJ \
	$(INTDIR)/PLAYER.OBJ \
	$(INTDIR)/UI.OBJ \
	$(INTDIR)/JOIN.OBJ \
	$(INTDIR)/SPRITE.OBJ \
	$(INTDIR)/WORLD.OBJ \
	$(INTDIR)/TERRAIN.OBJ \
	$(INTDIR)/NETAPI.OBJ \
	$(INTDIR)/UNIT.OBJ \
	$(INTDIR)/NEWWORLD.OBJ \
	$(INTDIR)/WRLDINIT.OBJ \
	$(INTDIR)/CPATHMGR.OBJ \
	$(INTDIR)/IPCCOMM.OBJ \
	$(INTDIR)/IPCSEND.OBJ \
	$(INTDIR)/IPCREAD.OBJ \
	$(INTDIR)/CHATBAR.OBJ \
	$(INTDIR)/IPCMSG.OBJ \
	$(INTDIR)/IPCCHAT.OBJ \
	$(INTDIR)/LASTPLNT.OBJ \
	$(INTDIR)/LASTPLNT.res \
	$(INTDIR)/IPCPLAY.OBJ \
	$(INTDIR)/EVENT.OBJ \
	$(INTDIR)/MINERALS.OBJ \
	$(INTDIR)/CAIUNIT.OBJ \
	$(INTDIR)/CAIMGR.OBJ \
	$(INTDIR)/CAIDATA.OBJ \
	$(INTDIR)/CAIOPFOR.OBJ \
	$(INTDIR)/CAIMSG.OBJ \
	$(INTDIR)/CAIINIT.OBJ \
	$(INTDIR)/CAIROUTE.OBJ \
	$(INTDIR)/CAICOPY.OBJ \
	$(INTDIR)/CAITASK.OBJ \
	$(INTDIR)/CAIMAP.OBJ \
	$(INTDIR)/CAIMAPUT.OBJ \
	$(INTDIR)/CAISAVLD.OBJ \
	$(INTDIR)/CAIGOAL.OBJ \
	$(INTDIR)/CAISTART.OBJ \
	$(INTDIR)/CAIGMGR.OBJ \
	$(INTDIR)/CAITMGR.OBJ \
	$(INTDIR)/CAIHEX.OBJ \
	$(INTDIR)/NETCMD.OBJ \
	$(INTDIR)/SCENARIO.OBJ \
	$(INTDIR)/CREATSIN.OBJ \
	$(INTDIR)/PICKRACE.OBJ \
	$(INTDIR)/RACEDATA.OBJ \
	$(INTDIR)/CREATMUL.OBJ \
	$(INTDIR)/CPATHMAP.OBJ \
	$(INTDIR)/RELATION.OBJ \
	$(INTDIR)/UNIT_WND.OBJ \
	$(INTDIR)/VEHMOVE.OBJ \
	$(INTDIR)/CAILOG.OBJ \
	$(INTDIR)/RESEARCH.OBJ \
	$(INTDIR)/OPTIONS.OBJ \
	$(INTDIR)/CHAT.OBJ \
	$(INTDIR)/PROJBASE.OBJ \
	$(INTDIR)/CHPROUTE.OBJ \
	$(INTDIR)/CITIZEN.OBJ \
	$(INTDIR)/NEW_GAME.OBJ \
	$(INTDIR)/CUTSCENE.OBJ \
	$(INTDIR)/ICONS.OBJ \
	$(INTDIR)/TOOLBAR.OBJ \
	$(INTDIR)/DLGFLIC.OBJ

$(OUTDIR)/ENations.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Profile"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Profile"
# PROP BASE Intermediate_Dir "Profile"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
OUTDIR=.\Profile
INTDIR=.\Profile

ALL : $(OUTDIR)/ENations.exe $(OUTDIR)/LASTPLNT.map $(OUTDIR)/LASTPLNT.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /Zp4 /MD /W4 /GX /Zi /Od /Ob1 /Gf /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"STDAFX.H" /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /Zp4 /MD /W4 /GX /Z7 /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /D "_PROFILE" /Yu"STDAFX.H" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /Zp4 /MD /W4 /GX /Z7 /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /D "_PROFILE"\
 /Fp$(OUTDIR)/"LASTPLNT.pch" /Yu"STDAFX.H" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\Profile/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "f:\windward\lib" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"LASTPLNT.res" /i "f:\windward\lib" /d "_DEBUG"\
 /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"LASTPLNT.bsc" 

$(OUTDIR)/LASTPLNT.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 daved.lib wail32.lib vdmplay.lib wing32.lib dave32ut.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x16384 /SUBSYSTEM:windows /DEBUG /MACHINE:IX86
# SUBTRACT BASE LINK32 /PROFILE /INCREMENTAL:no /MAP
# ADD LINK32 tbs32.lib wind22.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x16384 /SUBSYSTEM:windows /PDB:none /MAP /DEBUG /MACHINE:IX86 /OUT:"Profile/ENations.exe"
# SUBTRACT LINK32 /PROFILE
LINK32_FLAGS=tbs32.lib wind22.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib\
 /NOLOGO /STACK:0x16384 /SUBSYSTEM:windows /PDB:none\
 /MAP:$(INTDIR)/"LASTPLNT.map" /DEBUG /MACHINE:IX86 /DEF:".\LASTPLNT.DEF"\
 /OUT:"Profile/ENations.exe" 
DEF_FILE=.\LASTPLNT.DEF
LINK32_OBJS= \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/AREA.OBJ \
	$(INTDIR)/AI.OBJ \
	$(INTDIR)/NEW_UNIT.OBJ \
	$(INTDIR)/MAIN.OBJ \
	$(INTDIR)/BMBUTTON.OBJ \
	$(INTDIR)/MAINLOOP.OBJ \
	$(INTDIR)/SPRTINIT.OBJ \
	$(INTDIR)/PLAYER.OBJ \
	$(INTDIR)/UI.OBJ \
	$(INTDIR)/JOIN.OBJ \
	$(INTDIR)/SPRITE.OBJ \
	$(INTDIR)/WORLD.OBJ \
	$(INTDIR)/TERRAIN.OBJ \
	$(INTDIR)/NETAPI.OBJ \
	$(INTDIR)/UNIT.OBJ \
	$(INTDIR)/NEWWORLD.OBJ \
	$(INTDIR)/WRLDINIT.OBJ \
	$(INTDIR)/CPATHMGR.OBJ \
	$(INTDIR)/IPCCOMM.OBJ \
	$(INTDIR)/IPCSEND.OBJ \
	$(INTDIR)/IPCREAD.OBJ \
	$(INTDIR)/CHATBAR.OBJ \
	$(INTDIR)/IPCMSG.OBJ \
	$(INTDIR)/IPCCHAT.OBJ \
	$(INTDIR)/LASTPLNT.OBJ \
	$(INTDIR)/LASTPLNT.res \
	$(INTDIR)/IPCPLAY.OBJ \
	$(INTDIR)/EVENT.OBJ \
	$(INTDIR)/MINERALS.OBJ \
	$(INTDIR)/CAIUNIT.OBJ \
	$(INTDIR)/CAIMGR.OBJ \
	$(INTDIR)/CAIDATA.OBJ \
	$(INTDIR)/CAIOPFOR.OBJ \
	$(INTDIR)/CAIMSG.OBJ \
	$(INTDIR)/CAIINIT.OBJ \
	$(INTDIR)/CAIROUTE.OBJ \
	$(INTDIR)/CAICOPY.OBJ \
	$(INTDIR)/CAITASK.OBJ \
	$(INTDIR)/CAIMAP.OBJ \
	$(INTDIR)/CAIMAPUT.OBJ \
	$(INTDIR)/CAISAVLD.OBJ \
	$(INTDIR)/CAIGOAL.OBJ \
	$(INTDIR)/CAISTART.OBJ \
	$(INTDIR)/CAIGMGR.OBJ \
	$(INTDIR)/CAITMGR.OBJ \
	$(INTDIR)/CAIHEX.OBJ \
	$(INTDIR)/NETCMD.OBJ \
	$(INTDIR)/SCENARIO.OBJ \
	$(INTDIR)/CREATSIN.OBJ \
	$(INTDIR)/PICKRACE.OBJ \
	$(INTDIR)/RACEDATA.OBJ \
	$(INTDIR)/CREATMUL.OBJ \
	$(INTDIR)/CPATHMAP.OBJ \
	$(INTDIR)/RELATION.OBJ \
	$(INTDIR)/UNIT_WND.OBJ \
	$(INTDIR)/VEHMOVE.OBJ \
	$(INTDIR)/CAILOG.OBJ \
	$(INTDIR)/RESEARCH.OBJ \
	$(INTDIR)/OPTIONS.OBJ \
	$(INTDIR)/CHAT.OBJ \
	$(INTDIR)/PROJBASE.OBJ \
	$(INTDIR)/CHPROUTE.OBJ \
	$(INTDIR)/CITIZEN.OBJ \
	$(INTDIR)/NEW_GAME.OBJ \
	$(INTDIR)/CUTSCENE.OBJ \
	$(INTDIR)/ICONS.OBJ \
	$(INTDIR)/TOOLBAR.OBJ \
	$(INTDIR)/DLGFLIC.OBJ

$(OUTDIR)/ENations.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Release No Inline"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_"
# PROP BASE Intermediate_Dir "Release_"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NoInline"
# PROP Intermediate_Dir "NoInline"
OUTDIR=.\NoInline
INTDIR=.\NoInline

ALL : $(OUTDIR)/ENations.exe $(OUTDIR)/LASTPLNT.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /Zp4 /MD /W4 /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /Yu"STDAFX.H" /c
# ADD CPP /nologo /Zp4 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /D "_TRAP" /Yu"STDAFX.H" /c
CPP_PROJ=/nologo /Zp4 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /D "_TRAP" /Fp$(OUTDIR)/"LASTPLNT.pch"\
 /Yu"STDAFX.H" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"LASTPLNT.pdb" /c 
CPP_OBJS=.\NoInline/
# ADD BASE RSC /l 0x409 /i "e:\windward\lib" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "f:\windward\lib" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"LASTPLNT.res" /i "f:\windward\lib" /d "NDEBUG"\
 /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"LASTPLNT.bsc" 

$(OUTDIR)/LASTPLNT.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 windwrd.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x10240 /SUBSYSTEM:windows /INCREMENTAL:yes /PDB:"WinRel/ENations.pdb" /DEBUG /MACHINE:IX86 /OUT:"WinRel/ENations.exe"
# SUBTRACT BASE LINK32 /PROFILE /PDB:none /MAP /NODEFAULTLIB
# ADD LINK32 wind22.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO /STACK:0x10240 /SUBSYSTEM:windows /INCREMENTAL:yes /PDB:"NoInline/ENations.pdb" /DEBUG /MACHINE:IX86 /OUT:"NoInline/ENations.exe"
# SUBTRACT LINK32 /PROFILE /PDB:none /MAP /NODEFAULTLIB
LINK32_FLAGS=wind22.lib wail32.lib vdmplay.lib oldnames.lib winmm.lib /NOLOGO\
 /STACK:0x10240 /SUBSYSTEM:windows /INCREMENTAL:yes /PDB:"NoInline/ENations.pdb"\
 /DEBUG /MACHINE:IX86 /DEF:".\LASTPLNT.DEF" /OUT:"NoInline/ENations.exe" 
DEF_FILE=.\LASTPLNT.DEF
LINK32_OBJS= \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/AREA.OBJ \
	$(INTDIR)/AI.OBJ \
	$(INTDIR)/NEW_UNIT.OBJ \
	$(INTDIR)/MAIN.OBJ \
	$(INTDIR)/BMBUTTON.OBJ \
	$(INTDIR)/MAINLOOP.OBJ \
	$(INTDIR)/SPRTINIT.OBJ \
	$(INTDIR)/PLAYER.OBJ \
	$(INTDIR)/UI.OBJ \
	$(INTDIR)/JOIN.OBJ \
	$(INTDIR)/SPRITE.OBJ \
	$(INTDIR)/WORLD.OBJ \
	$(INTDIR)/TERRAIN.OBJ \
	$(INTDIR)/NETAPI.OBJ \
	$(INTDIR)/UNIT.OBJ \
	$(INTDIR)/NEWWORLD.OBJ \
	$(INTDIR)/WRLDINIT.OBJ \
	$(INTDIR)/CPATHMGR.OBJ \
	$(INTDIR)/IPCCOMM.OBJ \
	$(INTDIR)/IPCSEND.OBJ \
	$(INTDIR)/IPCREAD.OBJ \
	$(INTDIR)/CHATBAR.OBJ \
	$(INTDIR)/IPCMSG.OBJ \
	$(INTDIR)/IPCCHAT.OBJ \
	$(INTDIR)/LASTPLNT.OBJ \
	$(INTDIR)/LASTPLNT.res \
	$(INTDIR)/IPCPLAY.OBJ \
	$(INTDIR)/EVENT.OBJ \
	$(INTDIR)/MINERALS.OBJ \
	$(INTDIR)/CAIUNIT.OBJ \
	$(INTDIR)/CAIMGR.OBJ \
	$(INTDIR)/CAIDATA.OBJ \
	$(INTDIR)/CAIOPFOR.OBJ \
	$(INTDIR)/CAIMSG.OBJ \
	$(INTDIR)/CAIINIT.OBJ \
	$(INTDIR)/CAIROUTE.OBJ \
	$(INTDIR)/CAICOPY.OBJ \
	$(INTDIR)/CAITASK.OBJ \
	$(INTDIR)/CAIMAP.OBJ \
	$(INTDIR)/CAIMAPUT.OBJ \
	$(INTDIR)/CAISAVLD.OBJ \
	$(INTDIR)/CAIGOAL.OBJ \
	$(INTDIR)/CAISTART.OBJ \
	$(INTDIR)/CAIGMGR.OBJ \
	$(INTDIR)/CAITMGR.OBJ \
	$(INTDIR)/CAIHEX.OBJ \
	$(INTDIR)/NETCMD.OBJ \
	$(INTDIR)/SCENARIO.OBJ \
	$(INTDIR)/CREATSIN.OBJ \
	$(INTDIR)/PICKRACE.OBJ \
	$(INTDIR)/RACEDATA.OBJ \
	$(INTDIR)/CREATMUL.OBJ \
	$(INTDIR)/CPATHMAP.OBJ \
	$(INTDIR)/RELATION.OBJ \
	$(INTDIR)/UNIT_WND.OBJ \
	$(INTDIR)/VEHMOVE.OBJ \
	$(INTDIR)/CAILOG.OBJ \
	$(INTDIR)/RESEARCH.OBJ \
	$(INTDIR)/OPTIONS.OBJ \
	$(INTDIR)/CHAT.OBJ \
	$(INTDIR)/PROJBASE.OBJ \
	$(INTDIR)/CHPROUTE.OBJ \
	$(INTDIR)/CITIZEN.OBJ \
	$(INTDIR)/NEW_GAME.OBJ \
	$(INTDIR)/CUTSCENE.OBJ \
	$(INTDIR)/ICONS.OBJ \
	$(INTDIR)/TOOLBAR.OBJ \
	$(INTDIR)/DLGFLIC.OBJ

$(OUTDIR)/ENations.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\STDAFX.CPP
DEP_STDAF=\
	.\STDAFX.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"

$(INTDIR)/STDAFX.OBJ :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /Zp4 /MD /W4 /GX /Zi /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /Fp$(OUTDIR)/"LASTPLNT.pch"\
 /Yc"STDAFX.H" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"LASTPLNT.pdb" /c  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Debug"

# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"

$(INTDIR)/STDAFX.OBJ :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /Zp4 /MD /W4 /GX /Zi /Od /Ob1 /Gf /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp$(OUTDIR)/"LASTPLNT.pch" /Yc"STDAFX.H"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"LASTPLNT.pdb" /c  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Profile"

# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"

$(INTDIR)/STDAFX.OBJ :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /Zp4 /MD /W4 /GX /Z7 /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /D "_PROFILE"\
 /Fp$(OUTDIR)/"LASTPLNT.pch" /Yc"STDAFX.H" /Fo$(INTDIR)/ /c  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Release No Inline"

# ADD BASE CPP /Yc"STDAFX.H"
# ADD CPP /Yc"STDAFX.H"

$(INTDIR)/STDAFX.OBJ :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /Zp4 /MD /W4 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /D "_CHEAT" /D "_AFXDLL" /D "_MBCS" /D "_TRAP" /Fp$(OUTDIR)/"LASTPLNT.pch"\
 /Yc"STDAFX.H" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"LASTPLNT.pdb" /c  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AREA.CPP
DEP_AREA_=\
	.\STDAFX.H\
	.\AREA.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\SPRITE.H\
	.\ERROR.H\
	.\EVENT.H\
	.\BMBUTTON.H\
	.\BITMAPS.H\
	.\SFX.H\
	.\TERRAIN.INL\
	.\UI.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\UI.H\
	.\VERSION.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\MSGS.H\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/AREA.OBJ :  $(SOURCE)  $(DEP_AREA_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/AREA.OBJ :  $(SOURCE)  $(DEP_AREA_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/AREA.OBJ :  $(SOURCE)  $(DEP_AREA_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/AREA.OBJ :  $(SOURCE)  $(DEP_AREA_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AI.CPP
DEP_AI_CP=\
	.\STDAFX.H\
	F:\windward\include\THREADS.H\
	.\AI.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\RACEDATA.H\
	.\CAIDATA.HPP\
	.\CAIMGR.HPP\
	.\CAISAVLD.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	F:\windward\include\DAVE32UT.H\
	.\NETAPI.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RESEARCH.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	.\CAITMGR.HPP\
	.\CAIROUTE.HPP\
	.\CAIGOAL.HPP\
	.\CAITASK.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\MINERALS.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	.\CAIGMGR.HPP\
	.\CAIMSG.HPP\
	.\CAIMAP.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H\
	.\CAIOPFOR.HPP\
	.\CAIMAPUT.HPP

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/AI.OBJ :  $(SOURCE)  $(DEP_AI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/AI.OBJ :  $(SOURCE)  $(DEP_AI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/AI.OBJ :  $(SOURCE)  $(DEP_AI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/AI.OBJ :  $(SOURCE)  $(DEP_AI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NEW_UNIT.CPP
DEP_NEW_U=\
	.\STDAFX.H\
	.\EVENT.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\SPRITE.H\
	.\CPATHMGR.H\
	.\ERROR.H\
	.\CHPROUTE.HPP\
	.\AI.H\
	.\ICONS.H\
	.\BITMAPS.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAICOPY.HPP\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H\
	.\CAI.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/NEW_UNIT.OBJ :  $(SOURCE)  $(DEP_NEW_U) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/NEW_UNIT.OBJ :  $(SOURCE)  $(DEP_NEW_U) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/NEW_UNIT.OBJ :  $(SOURCE)  $(DEP_NEW_U) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/NEW_UNIT.OBJ :  $(SOURCE)  $(DEP_NEW_U) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MAIN.CPP
DEP_MAIN_=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\RELATION.H\
	.\IPCCOMM.H\
	.\RESEARCH.H\
	.\ERROR.H\
	.\EVENT.H\
	.\CUTSCENE.H\
	.\ICONS.H\
	.\SFX.H\
	.\UI.INL\
	.\CAILOG.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\CHATBAR.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/MAIN.OBJ :  $(SOURCE)  $(DEP_MAIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/MAIN.OBJ :  $(SOURCE)  $(DEP_MAIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/MAIN.OBJ :  $(SOURCE)  $(DEP_MAIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/MAIN.OBJ :  $(SOURCE)  $(DEP_MAIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BMBUTTON.CPP
DEP_BMBUT=\
	.\STDAFX.H\
	.\BMBUTTON.H\
	.\SFX.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\SPRITE.H\
	.\MSGS.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\BASE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/BMBUTTON.OBJ :  $(SOURCE)  $(DEP_BMBUT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/BMBUTTON.OBJ :  $(SOURCE)  $(DEP_BMBUT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/BMBUTTON.OBJ :  $(SOURCE)  $(DEP_BMBUT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/BMBUTTON.OBJ :  $(SOURCE)  $(DEP_BMBUT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MAINLOOP.CPP
DEP_MAINL=\
	.\STDAFX.H\
	.\EVENT.H\
	.\LASTPLNT.H\
	.\CPATHMGR.H\
	.\PLAYER.H\
	.\SPRITE.H\
	.\RESEARCH.H\
	.\RELATION.H\
	.\CHPROUTE.HPP\
	.\CUTSCENE.H\
	.\SCENARIO.H\
	.\AI.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\UNIT.INL\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\ICONS.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\MINERALS.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAICOPY.HPP\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H\
	.\CAI.H\
	.\ERROR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/MAINLOOP.OBJ :  $(SOURCE)  $(DEP_MAINL) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/MAINLOOP.OBJ :  $(SOURCE)  $(DEP_MAINL) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/MAINLOOP.OBJ :  $(SOURCE)  $(DEP_MAINL) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/MAINLOOP.OBJ :  $(SOURCE)  $(DEP_MAINL) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SPRTINIT.CPP
DEP_SPRTI=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\SPRITE.H\
	.\HELP.H\
	.\ERROR.H\
	.\SFX.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/SPRTINIT.OBJ :  $(SOURCE)  $(DEP_SPRTI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/SPRTINIT.OBJ :  $(SOURCE)  $(DEP_SPRTI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/SPRTINIT.OBJ :  $(SOURCE)  $(DEP_SPRTI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/SPRTINIT.OBJ :  $(SOURCE)  $(DEP_SPRTI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PLAYER.CPP
DEP_PLAYE=\
	.\STDAFX.H\
	.\EVENT.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\NETAPI.H\
	.\MINERALS.H\
	.\RESEARCH.H\
	.\CPATHMGR.H\
	.\AI.H\
	.\HELP.H\
	.\ERROR.H\
	.\CHPROUTE.HPP\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\ICONS.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\TERRAIN.H\
	.\UNIT.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAICOPY.HPP\
	.\SPRITE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H\
	.\CAI.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/PLAYER.OBJ :  $(SOURCE)  $(DEP_PLAYE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/PLAYER.OBJ :  $(SOURCE)  $(DEP_PLAYE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/PLAYER.OBJ :  $(SOURCE)  $(DEP_PLAYE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/PLAYER.OBJ :  $(SOURCE)  $(DEP_PLAYE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UI.CPP
DEP_UI_CP=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\WORLD.H\
	.\BITMAPS.H\
	.\UI.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BMBUTTON.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\BASE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/UI.OBJ :  $(SOURCE)  $(DEP_UI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/UI.OBJ :  $(SOURCE)  $(DEP_UI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/UI.OBJ :  $(SOURCE)  $(DEP_UI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/UI.OBJ :  $(SOURCE)  $(DEP_UI_CP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\JOIN.CPP
DEP_JOIN_=\
	.\STDAFX.H\
	.\NETAPI.H\
	.\LASTPLNT.H\
	.\JOIN.H\
	.\PLAYER.H\
	.\RACEDATA.H\
	.\HELP.H\
	.\CREATMUL.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\CREATMUL.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RESEARCH.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/JOIN.OBJ :  $(SOURCE)  $(DEP_JOIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/JOIN.OBJ :  $(SOURCE)  $(DEP_JOIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/JOIN.OBJ :  $(SOURCE)  $(DEP_JOIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/JOIN.OBJ :  $(SOURCE)  $(DEP_JOIN_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SPRITE.CPP
DEP_SPRIT=\
	.\STDAFX.H\
	.\SPRITE.H\
	.\LASTPLNT.H\
	.\ERROR.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BASE.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/SPRITE.OBJ :  $(SOURCE)  $(DEP_SPRIT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/SPRITE.OBJ :  $(SOURCE)  $(DEP_SPRIT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/SPRITE.OBJ :  $(SOURCE)  $(DEP_SPRIT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/SPRITE.OBJ :  $(SOURCE)  $(DEP_SPRIT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WORLD.CPP
DEP_WORLD=\
	.\STDAFX.H\
	.\WORLD.H\
	.\LASTPLNT.H\
	.\ERROR.H\
	.\UI.INL\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BMBUTTON.H\
	.\TERRAIN.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\MSGS.H\
	.\BASE.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/WORLD.OBJ :  $(SOURCE)  $(DEP_WORLD) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/WORLD.OBJ :  $(SOURCE)  $(DEP_WORLD) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/WORLD.OBJ :  $(SOURCE)  $(DEP_WORLD) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/WORLD.OBJ :  $(SOURCE)  $(DEP_WORLD) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TERRAIN.CPP
DEP_TERRA=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	F:\windward\include\BTREE.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\BASE.H\
	.\SPRITE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/TERRAIN.OBJ :  $(SOURCE)  $(DEP_TERRA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/TERRAIN.OBJ :  $(SOURCE)  $(DEP_TERRA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/TERRAIN.OBJ :  $(SOURCE)  $(DEP_TERRA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/TERRAIN.OBJ :  $(SOURCE)  $(DEP_TERRA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NETAPI.CPP
DEP_NETAP=\
	.\STDAFX.H\
	.\HELP.H\
	.\EVENT.H\
	.\NETAPI.H\
	.\NETCMD.H\
	.\JOIN.H\
	.\PLAYER.H\
	.\LASTPLNT.H\
	.\AI.H\
	.\CREATMUL.INL\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\RACEDATA.H\
	.\UNIT.H\
	.\NEW_GAME.H\
	.\CREATMUL.H\
	.\BASE.H\
	.\RESEARCH.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\IPCCOMM.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\ICONS.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/NETAPI.OBJ :  $(SOURCE)  $(DEP_NETAP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/NETAPI.OBJ :  $(SOURCE)  $(DEP_NETAP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/NETAPI.OBJ :  $(SOURCE)  $(DEP_NETAP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/NETAPI.OBJ :  $(SOURCE)  $(DEP_NETAP) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UNIT.CPP
DEP_UNIT_=\
	.\STDAFX.H\
	.\BASE.H\
	.\LASTPLNT.H\
	.\CPATHMGR.H\
	.\NETAPI.H\
	.\PLAYER.H\
	.\SPRITE.H\
	.\CHPROUTE.HPP\
	.\ICONS.H\
	.\BITMAPS.H\
	.\MINERALS.INL\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\MINERALS.H\
	.\TERRAIN.H\
	.\UNIT.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAICOPY.HPP\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H\
	.\CAI.H\
	.\ERROR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/UNIT.OBJ :  $(SOURCE)  $(DEP_UNIT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/UNIT.OBJ :  $(SOURCE)  $(DEP_UNIT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/UNIT.OBJ :  $(SOURCE)  $(DEP_UNIT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/UNIT.OBJ :  $(SOURCE)  $(DEP_UNIT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NEWWORLD.CPP
DEP_NEWWO=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\RACEDATA.H\
	.\NEW_GAME.H\
	.\RELATION.H\
	.\AI.H\
	.\IPCCOMM.H\
	.\CPATHMGR.H\
	.\HELP.H\
	.\RESEARCH.H\
	.\CHPROUTE.HPP\
	.\BITMAPS.H\
	.\SFX.H\
	.\TERRAIN.INL\
	.\UI.INL\
	.\UNIT.INL\
	.\MINERALS.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\BASE.H\
	.\NETCMD.H\
	.\NETAPI.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\ICONS.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\TERRAIN.H\
	.\UNIT.H\
	.\MINERALS.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\CHATBAR.H\
	.\CAICOPY.HPP\
	.\SPRITE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CAI.H\
	.\ERROR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/NEWWORLD.OBJ :  $(SOURCE)  $(DEP_NEWWO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/NEWWORLD.OBJ :  $(SOURCE)  $(DEP_NEWWO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/NEWWORLD.OBJ :  $(SOURCE)  $(DEP_NEWWO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/NEWWORLD.OBJ :  $(SOURCE)  $(DEP_NEWWO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WRLDINIT.CPP
DEP_WRLDI=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\AI.H\
	.\HELP.H\
	.\MINERALS.INL\
	.\TERRAIN.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\NETAPI.H\
	.\MINERALS.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\SPRITE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/WRLDINIT.OBJ :  $(SOURCE)  $(DEP_WRLDI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/WRLDINIT.OBJ :  $(SOURCE)  $(DEP_WRLDI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/WRLDINIT.OBJ :  $(SOURCE)  $(DEP_WRLDI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/WRLDINIT.OBJ :  $(SOURCE)  $(DEP_WRLDI) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CPATHMGR.CPP
DEP_CPATH=\
	.\STDAFX.H\
	.\CPATHMGR.H\
	.\CAIHEX.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\UNIT.INL\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\UNIT.H\
	.\TERRAIN.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\MINERALS.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CPATHMGR.OBJ :  $(SOURCE)  $(DEP_CPATH) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CPATHMGR.OBJ :  $(SOURCE)  $(DEP_CPATH) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CPATHMGR.OBJ :  $(SOURCE)  $(DEP_CPATH) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CPATHMGR.OBJ :  $(SOURCE)  $(DEP_CPATH) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPCCOMM.CPP
DEP_IPCCO=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\IPCCOMM.H\
	.\IPCPLAY.H\
	.\ERROR.H\
	.\EVENT.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\CHATBAR.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/IPCCOMM.OBJ :  $(SOURCE)  $(DEP_IPCCO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/IPCCOMM.OBJ :  $(SOURCE)  $(DEP_IPCCO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/IPCCOMM.OBJ :  $(SOURCE)  $(DEP_IPCCO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/IPCCOMM.OBJ :  $(SOURCE)  $(DEP_IPCCO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPCSEND.CPP
DEP_IPCSE=\
	.\STDAFX.H\
	.\PLAYER.H\
	.\IPCSEND.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\IPCMSG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\UNIT.H\
	.\ICONS.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/IPCSEND.OBJ :  $(SOURCE)  $(DEP_IPCSE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/IPCSEND.OBJ :  $(SOURCE)  $(DEP_IPCSE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/IPCSEND.OBJ :  $(SOURCE)  $(DEP_IPCSE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/IPCSEND.OBJ :  $(SOURCE)  $(DEP_IPCSE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPCREAD.CPP
DEP_IPCRE=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\IPCREAD.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\IPCMSG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCCHAT.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/IPCREAD.OBJ :  $(SOURCE)  $(DEP_IPCRE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/IPCREAD.OBJ :  $(SOURCE)  $(DEP_IPCRE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/IPCREAD.OBJ :  $(SOURCE)  $(DEP_IPCRE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/IPCREAD.OBJ :  $(SOURCE)  $(DEP_IPCRE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CHATBAR.CPP
DEP_CHATB=\
	.\STDAFX.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\UNIT.H\
	.\ICONS.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CHATBAR.OBJ :  $(SOURCE)  $(DEP_CHATB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CHATBAR.OBJ :  $(SOURCE)  $(DEP_CHATB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CHATBAR.OBJ :  $(SOURCE)  $(DEP_CHATB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CHATBAR.OBJ :  $(SOURCE)  $(DEP_CHATB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPCMSG.CPP
DEP_IPCMS=\
	.\STDAFX.H\
	.\IPCMSG.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\NETCMD.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\PLAYER.H\
	.\RACEDATA.H\
	.\UNIT.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\RESEARCH.H\
	.\SPRITE.H\
	.\ICONS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/IPCMSG.OBJ :  $(SOURCE)  $(DEP_IPCMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/IPCMSG.OBJ :  $(SOURCE)  $(DEP_IPCMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/IPCMSG.OBJ :  $(SOURCE)  $(DEP_IPCMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/IPCMSG.OBJ :  $(SOURCE)  $(DEP_IPCMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPCCHAT.CPP
DEP_IPCCH=\
	.\STDAFX.H\
	.\IPCCHAT.H\
	.\PLAYER.H\
	.\IPCPLAY.H\
	.\IPCCOMM.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\IPCMSG.HPP\
	.\CHATBAR.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\UNIT.H\
	.\ICONS.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/IPCCHAT.OBJ :  $(SOURCE)  $(DEP_IPCCH) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/IPCCHAT.OBJ :  $(SOURCE)  $(DEP_IPCCH) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/IPCCHAT.OBJ :  $(SOURCE)  $(DEP_IPCCH) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/IPCCHAT.OBJ :  $(SOURCE)  $(DEP_IPCCH) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\LASTPLNT.CPP
DEP_LASTP=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\CREATSIN.H\
	.\JOIN.H\
	.\SCENARIO.H\
	.\RACEDATA.H\
	.\SPRITE.H\
	.\OPTIONS.H\
	.\ERROR.H\
	.\AI.H\
	.\BMBUTTON.H\
	.\BITMAPS.H\
	.\SFX.H\
	.\DLGFLIC.H\
	.\TERRAIN.INL\
	.\CREATMUL.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\CREATMUL.H\
	.\BASE.H\
	.\PLAYER.H\
	.\NETAPI.H\
	.\MSGS.H\
	F:\windward\include\FLCCTRL.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\RESEARCH.H\
	F:\windward\include\FLCANIM.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/LASTPLNT.OBJ :  $(SOURCE)  $(DEP_LASTP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/LASTPLNT.OBJ :  $(SOURCE)  $(DEP_LASTP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/LASTPLNT.OBJ :  $(SOURCE)  $(DEP_LASTP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/LASTPLNT.OBJ :  $(SOURCE)  $(DEP_LASTP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\LASTPLNT.DEF
# End Source File
################################################################################
# Begin Source File

SOURCE=.\LASTPLNT.RC
DEP_LASTPL=\
	.\RES\CURSOR1.CUR\
	.\RES\CUR00001.CUR\
	.\RES\CUR00002.CUR\
	.\RES\CUR00003.CUR\
	.\RES\CUR00004.CUR\
	.\RES\CUR00005.CUR\
	.\RES\GOTO1.CUR\
	.\RES\CUR00006.CUR\
	.\RES\GOTO3.CUR\
	.\RES\BUILD4.CUR\
	.\RES\TARGET1.CUR\
	.\RES\TARGET2.CUR\
	.\RES\TARGET3.CUR\
	.\RES\SELECT1.CUR\
	.\RES\SELECT2.CUR\
	.\RES\SELECT3.CUR\
	.\RES\CUR00007.CUR\
	.\RES\CUR00008.CUR\
	.\RES\CUR00009.CUR\
	.\RES\CUR00010.CUR\
	.\RES\CUR00011.CUR\
	.\RES\REPAIR1.CUR\
	.\RES\CLOSED.DIB\
	.\RES\TOOLBAR.BMP\
	.\RES\BITMAP1.BMP\
	.\RES\CHAT_MSG.BMP\
	.\RES\EMAIL_MS.BMP\
	.\RES\EMAIL_RE.BMP\
	.\RES\NO_SELEC.BMP\
	.\RES\PLYR_SEL.BMP\
	.\RES\VOC_MSG.BMP\
	".\RES\D&C1.ICO"\
	.\RES\VERSION.RC\
	F:\windward\lib\Windward.RC\
	.\VERSION.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/LASTPLNT.res :  $(SOURCE)  $(DEP_LASTPL) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/LASTPLNT.res :  $(SOURCE)  $(DEP_LASTPL) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/LASTPLNT.res :  $(SOURCE)  $(DEP_LASTPL) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/LASTPLNT.res :  $(SOURCE)  $(DEP_LASTPL) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPCPLAY.CPP
DEP_IPCPL=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\IPCPLAY.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/IPCPLAY.OBJ :  $(SOURCE)  $(DEP_IPCPL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/IPCPLAY.OBJ :  $(SOURCE)  $(DEP_IPCPL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/IPCPLAY.OBJ :  $(SOURCE)  $(DEP_IPCPL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/IPCPLAY.OBJ :  $(SOURCE)  $(DEP_IPCPL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EVENT.CPP
DEP_EVENT=\
	.\STDAFX.H\
	.\EVENT.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\SFX.H\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\UNIT.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\SPRITE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/EVENT.OBJ :  $(SOURCE)  $(DEP_EVENT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/EVENT.OBJ :  $(SOURCE)  $(DEP_EVENT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/EVENT.OBJ :  $(SOURCE)  $(DEP_EVENT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/EVENT.OBJ :  $(SOURCE)  $(DEP_EVENT) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MINERALS.CPP
DEP_MINER=\
	.\STDAFX.H\
	.\BASE.H\
	.\MINERALS.INL\
	.\TERRAIN.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\MINERALS.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\UNIT.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/MINERALS.OBJ :  $(SOURCE)  $(DEP_MINER) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/MINERALS.OBJ :  $(SOURCE)  $(DEP_MINER) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/MINERALS.OBJ :  $(SOURCE)  $(DEP_MINER) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/MINERALS.OBJ :  $(SOURCE)  $(DEP_MINER) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIUNIT.CPP
DEP_CAIUN=\
	.\STDAFX.H\
	.\CAIUNIT.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\TERRAIN.INL\
	.\NETAPI.H\
	.\PLAYER.H\
	.\CAICOPY.HPP\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETCMD.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\BASE.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\ICONS.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIUNIT.OBJ :  $(SOURCE)  $(DEP_CAIUN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIUNIT.OBJ :  $(SOURCE)  $(DEP_CAIUN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIUNIT.OBJ :  $(SOURCE)  $(DEP_CAIUN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIUNIT.OBJ :  $(SOURCE)  $(DEP_CAIUN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIMGR.CPP
DEP_CAIMG=\
	.\STDAFX.H\
	.\CAIDATA.HPP\
	.\CAISAVLD.HPP\
	.\CAIMGR.HPP\
	.\CAIINIT.HPP\
	.\CAISTART.HPP\
	.\CPATHMAP.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	.\CAIGOAL.HPP\
	.\CAITASK.HPP\
	.\CAITMGR.HPP\
	.\CAIROUTE.HPP\
	.\CAIMAP.HPP\
	.\CPATHMGR.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	.\CAIGMGR.HPP\
	.\CAIMSG.HPP\
	.\CAIMAPUT.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\CAIOPFOR.HPP\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIMGR.OBJ :  $(SOURCE)  $(DEP_CAIMG) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIMGR.OBJ :  $(SOURCE)  $(DEP_CAIMG) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIMGR.OBJ :  $(SOURCE)  $(DEP_CAIMG) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIMGR.OBJ :  $(SOURCE)  $(DEP_CAIMG) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIDATA.CPP
DEP_CAIDA=\
	.\STDAFX.H\
	.\CAIDATA.HPP\
	.\CPATHMAP.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	.\CPATHMGR.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIDATA.OBJ :  $(SOURCE)  $(DEP_CAIDA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIDATA.OBJ :  $(SOURCE)  $(DEP_CAIDA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIDATA.OBJ :  $(SOURCE)  $(DEP_CAIDA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIDATA.OBJ :  $(SOURCE)  $(DEP_CAIDA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIOPFOR.CPP
DEP_CAIOP=\
	.\STDAFX.H\
	.\CAIOPFOR.HPP\
	.\CAIDATA.HPP\
	.\CAI.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIMSG.HPP\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	.\RACEDATA.H\
	.\ERROR.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIOPFOR.OBJ :  $(SOURCE)  $(DEP_CAIOP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIOPFOR.OBJ :  $(SOURCE)  $(DEP_CAIOP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIOPFOR.OBJ :  $(SOURCE)  $(DEP_CAIOP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIOPFOR.OBJ :  $(SOURCE)  $(DEP_CAIOP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIMSG.CPP
DEP_CAIMS=\
	.\STDAFX.H\
	.\CAIMSG.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\NETAPI.H\
	.\NETCMD.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\RACEDATA.H\
	.\UNIT.H\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIMSG.OBJ :  $(SOURCE)  $(DEP_CAIMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIMSG.OBJ :  $(SOURCE)  $(DEP_CAIMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIMSG.OBJ :  $(SOURCE)  $(DEP_CAIMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIMSG.OBJ :  $(SOURCE)  $(DEP_CAIMS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIINIT.CPP
DEP_CAIIN=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\CAIINIT.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\CAIMAP.HPP\
	.\CAIUNIT.HPP\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAIMSG.HPP\
	.\CAIMAPUT.HPP\
	.\CAICOPY.HPP\
	.\MINERALS.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIINIT.OBJ :  $(SOURCE)  $(DEP_CAIIN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIINIT.OBJ :  $(SOURCE)  $(DEP_CAIIN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIINIT.OBJ :  $(SOURCE)  $(DEP_CAIIN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIINIT.OBJ :  $(SOURCE)  $(DEP_CAIIN) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIROUTE.CPP
DEP_CAIRO=\
	.\STDAFX.H\
	.\CAIROUTE.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIMSG.HPP\
	.\CAIMAP.HPP\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\CAIMAPUT.HPP\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIROUTE.OBJ :  $(SOURCE)  $(DEP_CAIRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIROUTE.OBJ :  $(SOURCE)  $(DEP_CAIRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIROUTE.OBJ :  $(SOURCE)  $(DEP_CAIRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIROUTE.OBJ :  $(SOURCE)  $(DEP_CAIRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAICOPY.CPP
DEP_CAICO=\
	.\STDAFX.H\
	.\CAICOPY.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAI.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\RACEDATA.H\
	.\ERROR.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAICOPY.OBJ :  $(SOURCE)  $(DEP_CAICO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAICOPY.OBJ :  $(SOURCE)  $(DEP_CAICO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAICOPY.OBJ :  $(SOURCE)  $(DEP_CAICO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAICOPY.OBJ :  $(SOURCE)  $(DEP_CAICO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAITASK.CPP
DEP_CAITA=\
	.\STDAFX.H\
	.\CAI.H\
	.\CAITASK.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\RACEDATA.H\
	.\ERROR.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAITASK.OBJ :  $(SOURCE)  $(DEP_CAITA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAITASK.OBJ :  $(SOURCE)  $(DEP_CAITA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAITASK.OBJ :  $(SOURCE)  $(DEP_CAITA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAITASK.OBJ :  $(SOURCE)  $(DEP_CAITA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIMAP.CPP
DEP_CAIMA=\
	.\STDAFX.H\
	.\NETAPI.H\
	.\CAIMAP.HPP\
	.\CAIDATA.HPP\
	.\CPATHMAP.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIMSG.HPP\
	.\CAIMAPUT.HPP\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	.\CPATHMGR.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIMAP.OBJ :  $(SOURCE)  $(DEP_CAIMA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIMAP.OBJ :  $(SOURCE)  $(DEP_CAIMA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIMAP.OBJ :  $(SOURCE)  $(DEP_CAIMA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIMAP.OBJ :  $(SOURCE)  $(DEP_CAIMA) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIMAPUT.CPP
DEP_CAIMAP=\
	.\STDAFX.H\
	.\CAIMAPUT.HPP\
	.\CPATHMAP.H\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\CPATHMGR.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\CAICOPY.HPP\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIMAPUT.OBJ :  $(SOURCE)  $(DEP_CAIMAP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIMAPUT.OBJ :  $(SOURCE)  $(DEP_CAIMAP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIMAPUT.OBJ :  $(SOURCE)  $(DEP_CAIMAP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIMAPUT.OBJ :  $(SOURCE)  $(DEP_CAIMAP) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAISAVLD.CPP
DEP_CAISA=\
	.\STDAFX.H\
	.\CAI.H\
	.\CAISAVLD.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\RACEDATA.H\
	.\ERROR.H\
	.\CAIGOAL.HPP\
	.\CAITASK.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAISAVLD.OBJ :  $(SOURCE)  $(DEP_CAISA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAISAVLD.OBJ :  $(SOURCE)  $(DEP_CAISA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAISAVLD.OBJ :  $(SOURCE)  $(DEP_CAISA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAISAVLD.OBJ :  $(SOURCE)  $(DEP_CAISA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIGOAL.CPP
DEP_CAIGO=\
	.\STDAFX.H\
	.\CAI.H\
	.\CAIGOAL.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\RACEDATA.H\
	.\ERROR.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIGOAL.OBJ :  $(SOURCE)  $(DEP_CAIGO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIGOAL.OBJ :  $(SOURCE)  $(DEP_CAIGO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIGOAL.OBJ :  $(SOURCE)  $(DEP_CAIGO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIGOAL.OBJ :  $(SOURCE)  $(DEP_CAIGO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAISTART.CPP
DEP_CAIST=\
	.\STDAFX.H\
	.\CAISTART.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIDATA.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAISTART.OBJ :  $(SOURCE)  $(DEP_CAIST) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAISTART.OBJ :  $(SOURCE)  $(DEP_CAIST) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAISTART.OBJ :  $(SOURCE)  $(DEP_CAIST) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAISTART.OBJ :  $(SOURCE)  $(DEP_CAIST) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIGMGR.CPP
DEP_CAIGM=\
	.\STDAFX.H\
	.\CAIGMGR.HPP\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIGOAL.HPP\
	.\CAITASK.HPP\
	.\CAIMAP.HPP\
	.\CAIUNIT.HPP\
	.\CAIOPFOR.HPP\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\CAIMSG.HPP\
	.\CAIMAPUT.HPP\
	.\CAICOPY.HPP\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIGMGR.OBJ :  $(SOURCE)  $(DEP_CAIGM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIGMGR.OBJ :  $(SOURCE)  $(DEP_CAIGM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIGMGR.OBJ :  $(SOURCE)  $(DEP_CAIGM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIGMGR.OBJ :  $(SOURCE)  $(DEP_CAIGM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAITMGR.CPP
DEP_CAITM=\
	.\STDAFX.H\
	.\CAITMGR.HPP\
	.\CPATHMGR.H\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIGMGR.HPP\
	.\UNIT.INL\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAI.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\CAIGOAL.HPP\
	.\CAITASK.HPP\
	.\CAIMAP.HPP\
	.\CAIOPFOR.HPP\
	.\UNIT.H\
	.\TERRAIN.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\MINERALS.H\
	.\RACEDATA.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\ERROR.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\CAIMSG.HPP\
	.\CAIMAPUT.HPP\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAITMGR.OBJ :  $(SOURCE)  $(DEP_CAITM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAITMGR.OBJ :  $(SOURCE)  $(DEP_CAITM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAITMGR.OBJ :  $(SOURCE)  $(DEP_CAITM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAITMGR.OBJ :  $(SOURCE)  $(DEP_CAITM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAIHEX.CPP
DEP_CAIHE=\
	.\STDAFX.H\
	.\CAIHEX.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAIHEX.OBJ :  $(SOURCE)  $(DEP_CAIHE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAIHEX.OBJ :  $(SOURCE)  $(DEP_CAIHE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAIHEX.OBJ :  $(SOURCE)  $(DEP_CAIHE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAIHEX.OBJ :  $(SOURCE)  $(DEP_CAIHE) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NETCMD.CPP
DEP_NETCM=\
	.\STDAFX.H\
	.\NETCMD.H\
	.\PLAYER.H\
	.\LASTPLNT.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\RACEDATA.H\
	.\UNIT.H\
	.\BASE.H\
	.\RESEARCH.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\ICONS.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/NETCMD.OBJ :  $(SOURCE)  $(DEP_NETCM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/NETCMD.OBJ :  $(SOURCE)  $(DEP_NETCM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/NETCMD.OBJ :  $(SOURCE)  $(DEP_NETCM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/NETCMD.OBJ :  $(SOURCE)  $(DEP_NETCM) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SCENARIO.CPP
DEP_SCENA=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\SCENARIO.H\
	.\HELP.H\
	.\CPATHMAP.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\CPATHMGR.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\BASE.H\
	.\SPRITE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/SCENARIO.OBJ :  $(SOURCE)  $(DEP_SCENA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/SCENARIO.OBJ :  $(SOURCE)  $(DEP_SCENA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/SCENARIO.OBJ :  $(SOURCE)  $(DEP_SCENA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/SCENARIO.OBJ :  $(SOURCE)  $(DEP_SCENA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CREATSIN.CPP
DEP_CREAT=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\CREATSIN.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CREATSIN.OBJ :  $(SOURCE)  $(DEP_CREAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CREATSIN.OBJ :  $(SOURCE)  $(DEP_CREAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CREATSIN.OBJ :  $(SOURCE)  $(DEP_CREAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CREATSIN.OBJ :  $(SOURCE)  $(DEP_CREAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PICKRACE.CPP
DEP_PICKR=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\PICKRACE.H\
	.\SCENARIO.H\
	.\CUTSCENE.H\
	.\CREATMUL.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\CREATMUL.H\
	.\JOIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/PICKRACE.OBJ :  $(SOURCE)  $(DEP_PICKR) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/PICKRACE.OBJ :  $(SOURCE)  $(DEP_PICKR) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/PICKRACE.OBJ :  $(SOURCE)  $(DEP_PICKR) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/PICKRACE.OBJ :  $(SOURCE)  $(DEP_PICKR) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RACEDATA.CPP
DEP_RACED=\
	.\STDAFX.H\
	.\RACEDATA.H\
	.\ERROR.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/RACEDATA.OBJ :  $(SOURCE)  $(DEP_RACED) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/RACEDATA.OBJ :  $(SOURCE)  $(DEP_RACED) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/RACEDATA.OBJ :  $(SOURCE)  $(DEP_RACED) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/RACEDATA.OBJ :  $(SOURCE)  $(DEP_RACED) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CREATMUL.CPP
DEP_CREATM=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\HELP.H\
	.\ERROR.H\
	.\CREATMUL.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\CREATMUL.H\
	.\JOIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CREATMUL.OBJ :  $(SOURCE)  $(DEP_CREATM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CREATMUL.OBJ :  $(SOURCE)  $(DEP_CREATM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CREATMUL.OBJ :  $(SOURCE)  $(DEP_CREATM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CREATMUL.OBJ :  $(SOURCE)  $(DEP_CREATM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CPATHMAP.CPP
DEP_CPATHM=\
	.\STDAFX.H\
	.\CAI.H\
	.\CPATHMAP.H\
	.\CAIDATA.HPP\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\RACEDATA.H\
	.\ERROR.H\
	.\CPATHMGR.H\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	.\UNIT.INL\
	.\NETAPI.H\
	.\NETCMD.H\
	.\PLAYER.H\
	F:\windward\include\THREADS.H\
	.\CAIHEX.HPP\
	.\CAIUNIT.HPP\
	.\CAILOG.HPP\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TERRAIN.H\
	.\MINERALS.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\BASE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	.\CAICOPY.HPP\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CPATHMAP.OBJ :  $(SOURCE)  $(DEP_CPATHM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CPATHMAP.OBJ :  $(SOURCE)  $(DEP_CPATHM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CPATHMAP.OBJ :  $(SOURCE)  $(DEP_CPATHM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CPATHMAP.OBJ :  $(SOURCE)  $(DEP_CPATHM) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RELATION.CPP
DEP_RELAT=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\RELATION.H\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\UNIT.H\
	.\TERRAIN.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\BASE.H\
	.\SPRITE.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/RELATION.OBJ :  $(SOURCE)  $(DEP_RELAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/RELATION.OBJ :  $(SOURCE)  $(DEP_RELAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/RELATION.OBJ :  $(SOURCE)  $(DEP_RELAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/RELATION.OBJ :  $(SOURCE)  $(DEP_RELAT) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UNIT_WND.CPP
DEP_UNIT_W=\
	.\STDAFX.H\
	.\UNIT_WND.H\
	.\LASTPLNT.H\
	.\PLAYER.H\
	.\ERROR.H\
	.\EVENT.H\
	.\CHPROUTE.HPP\
	.\BITMAPS.H\
	.\TERRAIN.INL\
	.\UI.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\TERRAIN.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\MSGS.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAICOPY.HPP\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\CHATBAR.H\
	.\CAI.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/UNIT_WND.OBJ :  $(SOURCE)  $(DEP_UNIT_W) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/UNIT_WND.OBJ :  $(SOURCE)  $(DEP_UNIT_W) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/UNIT_WND.OBJ :  $(SOURCE)  $(DEP_UNIT_W) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/UNIT_WND.OBJ :  $(SOURCE)  $(DEP_UNIT_W) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VEHMOVE.CPP
DEP_VEHMO=\
	.\STDAFX.H\
	.\EVENT.H\
	.\LASTPLNT.H\
	.\CPATHMGR.H\
	.\PLAYER.H\
	.\SPRITE.H\
	.\AI.H\
	.\CHPROUTE.HPP\
	.\TERRAIN.INL\
	.\MINERALS.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\UNIT.INL\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\NETAPI.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\TERRAIN.H\
	.\MINERALS.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\CAICOPY.HPP\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H\
	.\CAI.H\
	.\ERROR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/VEHMOVE.OBJ :  $(SOURCE)  $(DEP_VEHMO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/VEHMOVE.OBJ :  $(SOURCE)  $(DEP_VEHMO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/VEHMOVE.OBJ :  $(SOURCE)  $(DEP_VEHMO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/VEHMOVE.OBJ :  $(SOURCE)  $(DEP_VEHMO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CAILOG.CPP
DEP_CAILO=\
	.\STDAFX.H\
	.\CAILOG.HPP\
	.\ERROR.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CAILOG.OBJ :  $(SOURCE)  $(DEP_CAILO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CAILOG.OBJ :  $(SOURCE)  $(DEP_CAILO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CAILOG.OBJ :  $(SOURCE)  $(DEP_CAILO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CAILOG.OBJ :  $(SOURCE)  $(DEP_CAILO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RESEARCH.CPP
DEP_RESEA=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\RESEARCH.H\
	.\ICONS.H\
	.\BITMAPS.H\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\UNIT.H\
	.\TERRAIN.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\BASE.H\
	.\SPRITE.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/RESEARCH.OBJ :  $(SOURCE)  $(DEP_RESEA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/RESEARCH.OBJ :  $(SOURCE)  $(DEP_RESEA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/RESEARCH.OBJ :  $(SOURCE)  $(DEP_RESEA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/RESEARCH.OBJ :  $(SOURCE)  $(DEP_RESEA) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\OPTIONS.CPP
DEP_OPTIO=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\OPTIONS.H\
	.\PLAYER.H\
	.\HELP.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/OPTIONS.OBJ :  $(SOURCE)  $(DEP_OPTIO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/OPTIONS.OBJ :  $(SOURCE)  $(DEP_OPTIO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/OPTIONS.OBJ :  $(SOURCE)  $(DEP_OPTIO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/OPTIONS.OBJ :  $(SOURCE)  $(DEP_OPTIO) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CHAT.CPP
DEP_CHAT_=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\CHAT.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CHAT.OBJ :  $(SOURCE)  $(DEP_CHAT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CHAT.OBJ :  $(SOURCE)  $(DEP_CHAT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CHAT.OBJ :  $(SOURCE)  $(DEP_CHAT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CHAT.OBJ :  $(SOURCE)  $(DEP_CHAT_) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PROJBASE.CPP
DEP_PROJB=\
	.\STDAFX.H\
	.\BASE.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	.\PLAYER.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/PROJBASE.OBJ :  $(SOURCE)  $(DEP_PROJB) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/PROJBASE.OBJ :  $(SOURCE)  $(DEP_PROJB) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/PROJBASE.OBJ :  $(SOURCE)  $(DEP_PROJB) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/PROJBASE.OBJ :  $(SOURCE)  $(DEP_PROJB) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CHPROUTE.CPP
DEP_CHPRO=\
	.\STDAFX.H\
	.\CHPROUTE.HPP\
	.\CAILOG.HPP\
	.\CPATHMAP.H\
	.\NETCMD.H\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\CAIMSG.HPP\
	.\CAIUNIT.HPP\
	.\CAIHEX.HPP\
	.\CPATHMGR.H\
	.\PLAYER.H\
	.\RACEDATA.H\
	.\UNIT.H\
	.\TERRAIN.H\
	.\UI.H\
	.\UNIT_WND.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\NETAPI.H\
	.\TERRAIN.INL\
	.\CAICOPY.HPP\
	.\BASE.H\
	.\RESEARCH.H\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\BMBUTTON.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\CAI.H\
	.\ICONS.H\
	.\TOOLBAR.H\
	.\MSGS.H\
	.\ERROR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CHPROUTE.OBJ :  $(SOURCE)  $(DEP_CHPRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CHPROUTE.OBJ :  $(SOURCE)  $(DEP_CHPRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CHPROUTE.OBJ :  $(SOURCE)  $(DEP_CHPRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CHPROUTE.OBJ :  $(SOURCE)  $(DEP_CHPRO) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CITIZEN.CPP
DEP_CITIZ=\
	.\STDAFX.H\
	.\PLAYER.H\
	.\MINERALS.H\
	.\TERRAIN.INL\
	.\UNIT.INL\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\UI.H\
	.\UNIT_WND.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\ICONS.H\
	.\SPRITE.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\BMBUTTON.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\TOOLBAR.H\
	.\MSGS.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CITIZEN.OBJ :  $(SOURCE)  $(DEP_CITIZ) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CITIZEN.OBJ :  $(SOURCE)  $(DEP_CITIZ) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CITIZEN.OBJ :  $(SOURCE)  $(DEP_CITIZ) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CITIZEN.OBJ :  $(SOURCE)  $(DEP_CITIZ) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NEW_GAME.CPP
DEP_NEW_G=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\NEW_GAME.H\
	.\ERROR.H\
	.\HELP.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\IPCCOMM.H\
	.\NETAPI.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	.\PLAYER.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/NEW_GAME.OBJ :  $(SOURCE)  $(DEP_NEW_G) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/NEW_GAME.OBJ :  $(SOURCE)  $(DEP_NEW_G) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/NEW_GAME.OBJ :  $(SOURCE)  $(DEP_NEW_G) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/NEW_GAME.OBJ :  $(SOURCE)  $(DEP_NEW_G) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CUTSCENE.CPP
DEP_CUTSC=\
	.\STDAFX.H\
	.\CUTSCENE.H\
	.\PLAYER.H\
	.\LASTPLNT.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BASE.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\RESEARCH.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\UNIT.H\
	.\ICONS.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\SPRITE.H\
	.\MSGS.H\
	.\CHATBAR.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CUTSCENE.OBJ :  $(SOURCE)  $(DEP_CUTSC) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CUTSCENE.OBJ :  $(SOURCE)  $(DEP_CUTSC) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/CUTSCENE.OBJ :  $(SOURCE)  $(DEP_CUTSC) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/CUTSCENE.OBJ :  $(SOURCE)  $(DEP_CUTSC) $(INTDIR)\
 $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ICONS.CPP
DEP_ICONS=\
	.\STDAFX.H\
	.\ICONS.H\
	.\SPRITE.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BASE.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/ICONS.OBJ :  $(SOURCE)  $(DEP_ICONS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/ICONS.OBJ :  $(SOURCE)  $(DEP_ICONS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/ICONS.OBJ :  $(SOURCE)  $(DEP_ICONS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/ICONS.OBJ :  $(SOURCE)  $(DEP_ICONS) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TOOLBAR.CPP
DEP_TOOLB=\
	.\STDAFX.H\
	.\TOOLBAR.H\
	.\BITMAPS.H\
	.\LASTPLNT.H\
	.\RELATION.H\
	.\EVENT.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\BMBUTTON.H\
	.\ICONS.H\
	.\UI.H\
	.\VERSION.H\
	.\AREA.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\SPRITE.H\
	.\MSGS.H\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\UNIT.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/TOOLBAR.OBJ :  $(SOURCE)  $(DEP_TOOLB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/TOOLBAR.OBJ :  $(SOURCE)  $(DEP_TOOLB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/TOOLBAR.OBJ :  $(SOURCE)  $(DEP_TOOLB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/TOOLBAR.OBJ :  $(SOURCE)  $(DEP_TOOLB) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DLGFLIC.CPP
DEP_DLGFL=\
	.\STDAFX.H\
	.\LASTPLNT.H\
	.\DLGFLIC.H\
	D:\tools\INCLUDE\ddraw.h\
	D:\tools\INCLUDE\dsound.h\
	D:\tools\INCLUDE\WING.H\
	D:\tools\INCLUDE\WAIL.H\
	F:\windward\include\WINDWARD.H\
	.\VDMPLAY.H\
	.\VERSION.H\
	.\AREA.H\
	.\UI.H\
	.\WORLD.H\
	.\UNIT_WND.H\
	.\TOOLBAR.H\
	.\NEW_GAME.H\
	.\IPCCOMM.H\
	F:\windward\include\FLCCTRL.H\
	F:\windward\include\THIELEN.H\
	F:\windward\include\_ERROR.H\
	F:\windward\include\_HELP.H\
	F:\windward\include\DATAFILE.H\
	F:\windward\include\FIXPOINT.H\
	F:\windward\include\GAMES.H\
	F:\windward\include\INIT.H\
	F:\windward\include\MMIO.H\
	F:\windward\include\music.h\
	F:\windward\include\PTR.H\
	F:\windward\include\RAND.H\
	F:\windward\include\SCANLIST.H\
	F:\windward\include\THREADS.H\
	F:\windward\include\BLT.H\
	F:\windward\include\DIB.H\
	F:\windward\include\DIBWND.H\
	F:\windward\include\wndbase.h\
	F:\windward\include\_msgs.h\
	F:\windward\include\WINDWARD.INL\
	.\TO_WNDRD.H\
	.\NETAPI.H\
	.\TERRAIN.H\
	.\BMBUTTON.H\
	.\UNIT.H\
	.\ICONS.H\
	.\NETCMD.H\
	.\RACEDATA.H\
	.\PICKRACE.H\
	.\CHAT.H\
	.\IPCMSG.HPP\
	.\IPCCHAT.H\
	.\IPCREAD.H\
	.\IPCSEND.H\
	F:\windward\include\FLCANIM.H\
	F:\windward\include\DAVE32UT.H\
	F:\windward\include\THIELEN.INL\
	F:\windward\include\mmio.inl\
	.\BASE.H\
	.\SPRITE.H\
	.\MSGS.H\
	.\PLAYER.H\
	.\CHATBAR.H\
	.\RESEARCH.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/DLGFLIC.OBJ :  $(SOURCE)  $(DEP_DLGFL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/DLGFLIC.OBJ :  $(SOURCE)  $(DEP_DLGFL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Profile"

$(INTDIR)/DLGFLIC.OBJ :  $(SOURCE)  $(DEP_DLGFL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ELSEIF  "$(CFG)" == "Release No Inline"

$(INTDIR)/DLGFLIC.OBJ :  $(SOURCE)  $(DEP_DLGFL) $(INTDIR) $(INTDIR)/STDAFX.OBJ

!ENDIF 

# End Source File
# End Group
# End Project
################################################################################
