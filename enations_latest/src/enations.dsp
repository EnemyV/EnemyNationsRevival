# Microsoft Developer Studio Project File - Name="enations" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=enations - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "enations.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "enations.mak" CFG="enations - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "enations - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "enations - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "enations - Win32 Profile" (based on "Win32 (x86) Application")
!MESSAGE "enations - Win32 TRAPs" (based on "Win32 (x86) Application")
!MESSAGE "enations - Win32 LOGOUT  TRAP" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "enations - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\WinRel"
# PROP Intermediate_Dir ".\WinRel"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "\\zazoo\e\windward\lib" /d "NDEBUG" /d "_WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 shmfc4m.lib shlw32m.lib wind40.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib version.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "enations - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\WinDebug"
# PROP Intermediate_Dir ".\WinDebug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_LOGOUT" /D "_MBCS" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "\\zazoo\e\windward\lib" /d "_DEBUG" /d "_WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 hamfc4m.lib haw32m.lib wind40d.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib version.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "enations - Win32 Profile"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\enations"
# PROP BASE Intermediate_Dir ".\enations"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\profile"
# PROP Intermediate_Dir ".\profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_CHEAT" /D "_LOGOUT" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /Z7 /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_CHEAT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "e:\windward\lib" /d "NDEBUG" /d "_WIN32"
# ADD RSC /l 0x409 /i "\\zazoo\e\windward\lib" /d "NDEBUG" /d "_WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shmfc4m.lib shlw32m.lib wind40.lib wail32.lib vdmplay.lib winmm.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 shmfc4m.lib shlw32m.lib wind40.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib version.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386
# SUBTRACT LINK32 /map /nodefaultlib

!ELSEIF  "$(CFG)" == "enations - Win32 TRAPs"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\enation0"
# PROP BASE Intermediate_Dir ".\enation0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\trap"
# PROP Intermediate_Dir ".\trap"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /Ob0 /D "NDEBUG" /D "_TRAP" /D "WIN32" /D "_WINDOWS" /D "_CHEAT" /D "_MBCS" /Yu"stdafx.h" /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /Ob0 /D "NDEBUG" /D "_TRAP" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_CHEAT" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "e:\windward\lib" /d "NDEBUG" /d "_WIN32"
# ADD RSC /l 0x409 /i "\\zazoo\e\windward\lib" /d "NDEBUG" /d "_WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shmfc4m.lib shlw32m.lib wind40.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 shmfc4m.lib shlw32m.lib wind40.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "enations - Win32 LOGOUT  TRAP"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\enation1"
# PROP BASE Intermediate_Dir ".\enation1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\logout"
# PROP Intermediate_Dir ".\logout"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /Ob0 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_CHEAT" /D "_MBCS" /D "_LOGOUT" /D "TRAP" /Yu"stdafx.h" /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /Ob0 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LOGOUT" /D "_TRAP" /D "_WINDOWS" /D "_CHEAT" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "e:\windward\lib" /d "NDEBUG" /d "_WIN32"
# ADD RSC /l 0x409 /i "\\zazoo\e\windward\lib" /d "NDEBUG" /d "_WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shmfc4m.lib shlw32m.lib wind40.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 shmfc4m.lib shlw32m.lib wind40.lib mss32.lib vdmplay.lib vfw32.lib winmm.lib version.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "enations - Win32 Release"
# Name "enations - Win32 Debug"
# Name "enations - Win32 Profile"
# Name "enations - Win32 TRAPs"
# Name "enations - Win32 LOGOUT  TRAP"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\Ai.cpp
# End Source File
# Begin Source File

SOURCE=.\Area.cpp
# End Source File
# Begin Source File

SOURCE=.\Bmbutton.cpp
# End Source File
# Begin Source File

SOURCE=.\bridge.cpp
# End Source File
# Begin Source File

SOURCE=.\Caicopy.cpp
# End Source File
# Begin Source File

SOURCE=.\Caidata.cpp
# End Source File
# Begin Source File

SOURCE=.\Caigmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\Caigoal.cpp
# End Source File
# Begin Source File

SOURCE=.\Caihex.cpp
# End Source File
# Begin Source File

SOURCE=.\Caiinit.cpp
# End Source File
# Begin Source File

SOURCE=.\Caimap.cpp
# End Source File
# Begin Source File

SOURCE=.\Caimaput.cpp
# End Source File
# Begin Source File

SOURCE=.\Caimgr.cpp
# End Source File
# Begin Source File

SOURCE=.\Caimsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Caiopfor.cpp
# End Source File
# Begin Source File

SOURCE=.\Cairoute.cpp
# End Source File
# Begin Source File

SOURCE=.\Caisavld.cpp
# End Source File
# Begin Source File

SOURCE=.\Caistart.cpp
# End Source File
# Begin Source File

SOURCE=.\Caitask.cpp
# End Source File
# Begin Source File

SOURCE=.\Caitmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\Caiunit.cpp
# End Source File
# Begin Source File

SOURCE=.\CdLoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Chat.cpp
# End Source File
# Begin Source File

SOURCE=.\Chatbar.cpp
# End Source File
# Begin Source File

SOURCE=.\Chproute.cpp
# End Source File
# Begin Source File

SOURCE=.\Cpathmap.cpp
# End Source File
# Begin Source File

SOURCE=.\Cpathmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\Creatmul.cpp
# End Source File
# Begin Source File

SOURCE=.\Creatsin.cpp
# End Source File
# Begin Source File

SOURCE=.\credits.cpp
# End Source File
# Begin Source File

SOURCE=.\Cutscene.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgflic.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgReg.cpp
# End Source File
# Begin Source File

SOURCE=.\DXFER.CPP
# End Source File
# Begin Source File

SOURCE=.\Event.cpp
# End Source File
# Begin Source File

SOURCE=.\Icons.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipcchat.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipccomm.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipcmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipcplay.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipcread.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipcsend.cpp
# End Source File
# Begin Source File

SOURCE=.\Join.cpp
# End Source File
# Begin Source File

SOURCE=.\lastplnt.cpp
# End Source File
# Begin Source File

SOURCE=.\Lastplnt.rc
# End Source File
# Begin Source File

SOURCE=.\License.cpp
# End Source File
# Begin Source File

SOURCE=.\loadtruk.cpp
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\Mainloop.cpp
# End Source File
# Begin Source File

SOURCE=.\Minerals.cpp
# End Source File
# Begin Source File

SOURCE=.\movie.cpp
# End Source File
# Begin Source File

SOURCE=.\Netapi.cpp
# End Source File
# Begin Source File

SOURCE=.\Netcmd.cpp
# End Source File
# Begin Source File

SOURCE=.\New_game.cpp
# End Source File
# Begin Source File

SOURCE=.\New_unit.cpp
# End Source File
# Begin Source File

SOURCE=.\Newworld.cpp
# End Source File
# Begin Source File

SOURCE=.\Options.cpp
# End Source File
# Begin Source File

SOURCE=.\Pickrace.cpp
# End Source File
# Begin Source File

SOURCE=.\Player.cpp
# End Source File
# Begin Source File

SOURCE=.\PlyrList.cpp
# End Source File
# Begin Source File

SOURCE=.\Projbase.cpp
# End Source File
# Begin Source File

SOURCE=.\Racedata.cpp
# End Source File
# Begin Source File

SOURCE=.\Relation.cpp
# End Source File
# Begin Source File

SOURCE=.\Research.cpp
# End Source File
# Begin Source File

SOURCE=.\Scenario.cpp
# End Source File
# Begin Source File

SOURCE=.\Sprite.cpp
# End Source File
# Begin Source File

SOURCE=.\Sprtinit.cpp
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Terrain.cpp
# End Source File
# Begin Source File

SOURCE=.\Toolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\tstsnds.cpp
# End Source File
# Begin Source File

SOURCE=.\Unit.cpp
# End Source File
# Begin Source File

SOURCE=.\Unit_wnd.cpp
# End Source File
# Begin Source File

SOURCE=.\vehicle.cpp
# End Source File
# Begin Source File

SOURCE=.\Vehmove.cpp
# End Source File
# Begin Source File

SOURCE=.\vehoppo.cpp
# End Source File
# Begin Source File

SOURCE=.\VPXFER.CPP
# End Source File
# Begin Source File

SOURCE=.\World.cpp
# End Source File
# Begin Source File

SOURCE=.\Wrldinit.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ai.h
# End Source File
# Begin Source File

SOURCE=.\area.h
# End Source File
# Begin Source File

SOURCE=.\base.h
# End Source File
# Begin Source File

SOURCE=.\bitmaps.h
# End Source File
# Begin Source File

SOURCE=.\bmbutton.h
# End Source File
# Begin Source File

SOURCE=.\bridge.h
# End Source File
# Begin Source File

SOURCE=.\building.h
# End Source File
# Begin Source File

SOURCE=.\building.inl
# End Source File
# Begin Source File

SOURCE=.\cai.h
# End Source File
# Begin Source File

SOURCE=.\caicopy.hpp
# End Source File
# Begin Source File

SOURCE=.\caidata.hpp
# End Source File
# Begin Source File

SOURCE=.\caigmgr.hpp
# End Source File
# Begin Source File

SOURCE=.\caigoal.hpp
# End Source File
# Begin Source File

SOURCE=.\caihex.hpp
# End Source File
# Begin Source File

SOURCE=.\caiinit.hpp
# End Source File
# Begin Source File

SOURCE=.\caimap.hpp
# End Source File
# Begin Source File

SOURCE=.\caimaput.hpp
# End Source File
# Begin Source File

SOURCE=.\caimgr.hpp
# End Source File
# Begin Source File

SOURCE=.\caimsg.hpp
# End Source File
# Begin Source File

SOURCE=.\caiopfor.hpp
# End Source File
# Begin Source File

SOURCE=.\cairoute.hpp
# End Source File
# Begin Source File

SOURCE=.\caisavld.hpp
# End Source File
# Begin Source File

SOURCE=.\caistart.hpp
# End Source File
# Begin Source File

SOURCE=.\caitargt.h
# End Source File
# Begin Source File

SOURCE=.\caitask.hpp
# End Source File
# Begin Source File

SOURCE=.\caitmgr.hpp
# End Source File
# Begin Source File

SOURCE=.\caiunit.hpp
# End Source File
# Begin Source File

SOURCE=.\CdLoc.h
# End Source File
# Begin Source File

SOURCE=.\chat.h
# End Source File
# Begin Source File

SOURCE=.\chatbar.h
# End Source File
# Begin Source File

SOURCE=.\chproute.hpp
# End Source File
# Begin Source File

SOURCE=.\cpathmap.h
# End Source File
# Begin Source File

SOURCE=.\cpathmgr.h
# End Source File
# Begin Source File

SOURCE=.\creatmul.h
# End Source File
# Begin Source File

SOURCE=.\creatmul.inl
# End Source File
# Begin Source File

SOURCE=.\creatsin.h
# End Source File
# Begin Source File

SOURCE=.\credits.h
# End Source File
# Begin Source File

SOURCE=.\cutscene.h
# End Source File
# Begin Source File

SOURCE=.\dlgflic.h
# End Source File
# Begin Source File

SOURCE=.\dlgmsg.h
# End Source File
# Begin Source File

SOURCE=.\DlgReg.h
# End Source File
# Begin Source File

SOURCE=.\Dxfer.h
# End Source File
# Begin Source File

SOURCE=.\error.h
# End Source File
# Begin Source File

SOURCE=.\event.h
# End Source File
# Begin Source File

SOURCE=.\help.h
# End Source File
# Begin Source File

SOURCE=.\icons.h
# End Source File
# Begin Source File

SOURCE=.\ipcchat.h
# End Source File
# Begin Source File

SOURCE=.\ipccomm.h
# End Source File
# Begin Source File

SOURCE=.\ipcmsg.hpp
# End Source File
# Begin Source File

SOURCE=.\ipcplay.h
# End Source File
# Begin Source File

SOURCE=.\ipcread.h
# End Source File
# Begin Source File

SOURCE=.\ipcsend.h
# End Source File
# Begin Source File

SOURCE=.\join.h
# End Source File
# Begin Source File

SOURCE=.\lastplnt.h
# End Source File
# Begin Source File

SOURCE=.\license.h
# End Source File
# Begin Source File

SOURCE=.\loadtruk.h
# End Source File
# Begin Source File

SOURCE=.\minerals.h
# End Source File
# Begin Source File

SOURCE=.\minerals.inl
# End Source File
# Begin Source File

SOURCE=.\movie.h
# End Source File
# Begin Source File

SOURCE=.\msgs.h
# End Source File
# Begin Source File

SOURCE=.\netapi.h
# End Source File
# Begin Source File

SOURCE=.\netcmd.h
# End Source File
# Begin Source File

SOURCE=.\new_game.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\ourlog.h
# End Source File
# Begin Source File

SOURCE=.\pickrace.h
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\PlyrList.h
# End Source File
# Begin Source File

SOURCE=.\racedata.h
# End Source File
# Begin Source File

SOURCE=.\relation.h
# End Source File
# Begin Source File

SOURCE=.\research.h
# End Source File
# Begin Source File

SOURCE=.\scenario.h
# End Source File
# Begin Source File

SOURCE=.\sfx.h
# End Source File
# Begin Source File

SOURCE=.\sprite.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\terrain.h
# End Source File
# Begin Source File

SOURCE=.\terrain.inl
# End Source File
# Begin Source File

SOURCE=.\to_wndrd.h
# End Source File
# Begin Source File

SOURCE=.\toolbar.h
# End Source File
# Begin Source File

SOURCE=.\tstsnds.h
# End Source File
# Begin Source File

SOURCE=.\ui.h
# End Source File
# Begin Source File

SOURCE=.\ui.inl
# End Source File
# Begin Source File

SOURCE=.\unit.h
# End Source File
# Begin Source File

SOURCE=.\unit.inl
# End Source File
# Begin Source File

SOURCE=.\unit_wnd.h
# End Source File
# Begin Source File

SOURCE=.\us_eng.h
# End Source File
# Begin Source File

SOURCE=.\vdmplay.h
# End Source File
# Begin Source File

SOURCE=.\vehicle.h
# End Source File
# Begin Source File

SOURCE=.\vehicle.inl
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\Vpxfer.h
# End Source File
# Begin Source File

SOURCE=.\world.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RES\BITMAP1.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\CHAT_MSG.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\CLOSED.DIB
# End Source File
# Begin Source File

SOURCE=.\res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00002.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00003.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00004.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00005.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00006.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00007.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00008.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00010.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\CUR00011.CUR
# End Source File
# Begin Source File

SOURCE=.\res\cur00012.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00013.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00014.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00015.cur
# End Source File
# Begin Source File

SOURCE=.\RES\CURSOR1.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\EMAIL_MS.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\EMAIL_RE.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\GOTO1.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\GOTO3.CUR
# End Source File
# Begin Source File

SOURCE=.\res\load1.cur
# End Source File
# Begin Source File

SOURCE=.\res\load2.cur
# End Source File
# Begin Source File

SOURCE=.\res\load3.cur
# End Source File
# Begin Source File

SOURCE=.\res\load4.cur
# End Source File
# Begin Source File

SOURCE=.\res\main.ico
# End Source File
# Begin Source File

SOURCE=.\res\move1.cur
# End Source File
# Begin Source File

SOURCE=.\res\move2.cur
# End Source File
# Begin Source File

SOURCE=.\res\move3.cur
# End Source File
# Begin Source File

SOURCE=.\res\move4.cur
# End Source File
# Begin Source File

SOURCE=.\res\move5.cur
# End Source File
# Begin Source File

SOURCE=.\res\move6.cur
# End Source File
# Begin Source File

SOURCE=.\res\move7.cur
# End Source File
# Begin Source File

SOURCE=.\RES\NO_SELEC.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\PLYR_SEL.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\REPAIR1.CUR
# End Source File
# Begin Source File

SOURCE=.\res\road_beg.cur
# End Source File
# Begin Source File

SOURCE=.\res\road_set.cur
# End Source File
# Begin Source File

SOURCE=.\RES\SELECT1.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\SELECT2.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\SELECT3.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\TARGET1.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\TARGET2.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\TARGET3.CUR
# End Source File
# Begin Source File

SOURCE=.\RES\TOOLBAR.BMP
# End Source File
# Begin Source File

SOURCE=.\res\unload1.cur
# End Source File
# Begin Source File

SOURCE=.\res\unload2.cur
# End Source File
# Begin Source File

SOURCE=.\res\unload3.cur
# End Source File
# Begin Source File

SOURCE=.\RES\VERSION.RC
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\RES\VOC_MSG.BMP
# End Source File
# End Group
# End Target
# End Project
