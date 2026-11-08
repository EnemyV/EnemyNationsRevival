# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Sprite - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Sprite - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Sprite - Win32 Debug" && "$(CFG)" != "Sprite - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "SPRITE.MAK" CFG="Sprite - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sprite - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Sprite - Win32 Release" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "Sprite - Win32 Release"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "Sprite - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\SPRITE.exe" "$(OUTDIR)\SPRITE.pch" "$(OUTDIR)\SPRITE.bsc"

CLEAN : 
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\vc40.idb"
	-@erase ".\WinDebug\SPRITE.pch"
	-@erase ".\WinDebug\SPRITE.bsc"
	-@erase ".\WinDebug\spritvw.sbr"
	-@erase ".\WinDebug\sprd.sbr"
	-@erase ".\WinDebug\medcut.sbr"
	-@erase ".\WinDebug\NamngDlg.sbr"
	-@erase ".\WinDebug\drawdib.sbr"
	-@erase ".\WinDebug\StatusDlg.sbr"
	-@erase ".\WinDebug\cdirdlg.sbr"
	-@erase ".\WinDebug\vd.sbr"
	-@erase ".\WinDebug\splitwnd.sbr"
	-@erase ".\WinDebug\mainfrm.sbr"
	-@erase ".\WinDebug\stucki.sbr"
	-@erase ".\WinDebug\multprog.sbr"
	-@erase ".\WinDebug\rgbcolor.sbr"
	-@erase ".\WinDebug\Huffmanc.sbr"
	-@erase ".\WinDebug\sp.sbr"
	-@erase ".\WinDebug\varcut.sbr"
	-@erase ".\WinDebug\Bpecodec.sbr"
	-@erase ".\WinDebug\rbuild.sbr"
	-@erase ".\WinDebug\bmpfile.sbr"
	-@erase ".\WinDebug\sptrctrl.sbr"
	-@erase ".\WinDebug\sprite.sbr"
	-@erase ".\WinDebug\Sprcnvrt.sbr"
	-@erase ".\WinDebug\tgafile.sbr"
	-@erase ".\WinDebug\PROGDLG.SBR"
	-@erase ".\WinDebug\PALVIEW.SBR"
	-@erase ".\WinDebug\SPProp.sbr"
	-@erase ".\WinDebug\Bitbuffe.sbr"
	-@erase ".\WinDebug\v.sbr"
	-@erase ".\WinDebug\td.sbr"
	-@erase ".\WinDebug\bd.sbr"
	-@erase ".\WinDebug\terrain.sbr"
	-@erase ".\WinDebug\HoldTime.sbr"
	-@erase ".\WinDebug\image.sbr"
	-@erase ".\WinDebug\boxsplit.sbr"
	-@erase ".\WinDebug\PALDOC.SBR"
	-@erase ".\WinDebug\Codec.sbr"
	-@erase ".\WinDebug\Lzwcodec.sbr"
	-@erase ".\WinDebug\treeview.sbr"
	-@erase ".\WinDebug\Lzsscode.sbr"
	-@erase ".\WinDebug\stdafx.sbr"
	-@erase ".\WinDebug\SPRITE.exe"
	-@erase ".\WinDebug\bd.obj"
	-@erase ".\WinDebug\terrain.obj"
	-@erase ".\WinDebug\HoldTime.obj"
	-@erase ".\WinDebug\image.obj"
	-@erase ".\WinDebug\boxsplit.obj"
	-@erase ".\WinDebug\PALDOC.OBJ"
	-@erase ".\WinDebug\Codec.obj"
	-@erase ".\WinDebug\Lzwcodec.obj"
	-@erase ".\WinDebug\treeview.obj"
	-@erase ".\WinDebug\Lzsscode.obj"
	-@erase ".\WinDebug\stdafx.obj"
	-@erase ".\WinDebug\spritvw.obj"
	-@erase ".\WinDebug\sprd.obj"
	-@erase ".\WinDebug\medcut.obj"
	-@erase ".\WinDebug\NamngDlg.obj"
	-@erase ".\WinDebug\drawdib.obj"
	-@erase ".\WinDebug\StatusDlg.obj"
	-@erase ".\WinDebug\cdirdlg.obj"
	-@erase ".\WinDebug\vd.obj"
	-@erase ".\WinDebug\splitwnd.obj"
	-@erase ".\WinDebug\mainfrm.obj"
	-@erase ".\WinDebug\stucki.obj"
	-@erase ".\WinDebug\multprog.obj"
	-@erase ".\WinDebug\rgbcolor.obj"
	-@erase ".\WinDebug\Huffmanc.obj"
	-@erase ".\WinDebug\sp.obj"
	-@erase ".\WinDebug\varcut.obj"
	-@erase ".\WinDebug\Bpecodec.obj"
	-@erase ".\WinDebug\rbuild.obj"
	-@erase ".\WinDebug\bmpfile.obj"
	-@erase ".\WinDebug\sptrctrl.obj"
	-@erase ".\WinDebug\sprite.obj"
	-@erase ".\WinDebug\Sprcnvrt.obj"
	-@erase ".\WinDebug\tgafile.obj"
	-@erase ".\WinDebug\PROGDLG.OBJ"
	-@erase ".\WinDebug\PALVIEW.OBJ"
	-@erase ".\WinDebug\SPProp.obj"
	-@erase ".\WinDebug\Bitbuffe.obj"
	-@erase ".\WinDebug\v.obj"
	-@erase ".\WinDebug\td.obj"
	-@erase ".\WinDebug\sprite.res"
	-@erase ".\WinDebug\SPRITE.ilk"
	-@erase ".\WinDebug\SPRITE.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /c
# SUBTRACT CPP /Yu
CPP_PROJ=/nologo /MDd /W4 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/sprite.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/SPRITE.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/spritvw.sbr" \
	"$(INTDIR)/sprd.sbr" \
	"$(INTDIR)/medcut.sbr" \
	"$(INTDIR)/NamngDlg.sbr" \
	"$(INTDIR)/drawdib.sbr" \
	"$(INTDIR)/StatusDlg.sbr" \
	"$(INTDIR)/cdirdlg.sbr" \
	"$(INTDIR)/vd.sbr" \
	"$(INTDIR)/splitwnd.sbr" \
	"$(INTDIR)/mainfrm.sbr" \
	"$(INTDIR)/stucki.sbr" \
	"$(INTDIR)/multprog.sbr" \
	"$(INTDIR)/rgbcolor.sbr" \
	"$(INTDIR)/Huffmanc.sbr" \
	"$(INTDIR)/sp.sbr" \
	"$(INTDIR)/varcut.sbr" \
	"$(INTDIR)/Bpecodec.sbr" \
	"$(INTDIR)/rbuild.sbr" \
	"$(INTDIR)/bmpfile.sbr" \
	"$(INTDIR)/sptrctrl.sbr" \
	"$(INTDIR)/sprite.sbr" \
	"$(INTDIR)/Sprcnvrt.sbr" \
	"$(INTDIR)/tgafile.sbr" \
	"$(INTDIR)/PROGDLG.SBR" \
	"$(INTDIR)/PALVIEW.SBR" \
	"$(INTDIR)/SPProp.sbr" \
	"$(INTDIR)/Bitbuffe.sbr" \
	"$(INTDIR)/v.sbr" \
	"$(INTDIR)/td.sbr" \
	"$(INTDIR)/bd.sbr" \
	"$(INTDIR)/terrain.sbr" \
	"$(INTDIR)/HoldTime.sbr" \
	"$(INTDIR)/image.sbr" \
	"$(INTDIR)/boxsplit.sbr" \
	"$(INTDIR)/PALDOC.SBR" \
	"$(INTDIR)/Codec.sbr" \
	"$(INTDIR)/Lzwcodec.sbr" \
	"$(INTDIR)/treeview.sbr" \
	"$(INTDIR)/Lzsscode.sbr" \
	"$(INTDIR)/stdafx.sbr"

"$(OUTDIR)\SPRITE.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=winmm.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/SPRITE.pdb" /debug /machine:I386 /out:"$(OUTDIR)/SPRITE.exe" 
LINK32_OBJS= \
	"$(INTDIR)/bd.obj" \
	"$(INTDIR)/terrain.obj" \
	"$(INTDIR)/HoldTime.obj" \
	"$(INTDIR)/image.obj" \
	"$(INTDIR)/boxsplit.obj" \
	"$(INTDIR)/PALDOC.OBJ" \
	"$(INTDIR)/Codec.obj" \
	"$(INTDIR)/Lzwcodec.obj" \
	"$(INTDIR)/treeview.obj" \
	"$(INTDIR)/Lzsscode.obj" \
	"$(INTDIR)/stdafx.obj" \
	"$(INTDIR)/spritvw.obj" \
	"$(INTDIR)/sprd.obj" \
	"$(INTDIR)/medcut.obj" \
	"$(INTDIR)/NamngDlg.obj" \
	"$(INTDIR)/drawdib.obj" \
	"$(INTDIR)/StatusDlg.obj" \
	"$(INTDIR)/cdirdlg.obj" \
	"$(INTDIR)/vd.obj" \
	"$(INTDIR)/splitwnd.obj" \
	"$(INTDIR)/mainfrm.obj" \
	"$(INTDIR)/stucki.obj" \
	"$(INTDIR)/multprog.obj" \
	"$(INTDIR)/rgbcolor.obj" \
	"$(INTDIR)/Huffmanc.obj" \
	"$(INTDIR)/sp.obj" \
	"$(INTDIR)/varcut.obj" \
	"$(INTDIR)/Bpecodec.obj" \
	"$(INTDIR)/rbuild.obj" \
	"$(INTDIR)/bmpfile.obj" \
	"$(INTDIR)/sptrctrl.obj" \
	"$(INTDIR)/sprite.obj" \
	"$(INTDIR)/Sprcnvrt.obj" \
	"$(INTDIR)/tgafile.obj" \
	"$(INTDIR)/PROGDLG.OBJ" \
	"$(INTDIR)/PALVIEW.OBJ" \
	"$(INTDIR)/SPProp.obj" \
	"$(INTDIR)/Bitbuffe.obj" \
	"$(INTDIR)/v.obj" \
	"$(INTDIR)/td.obj" \
	"$(INTDIR)/sprite.res"

"$(OUTDIR)\SPRITE.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : "$(OUTDIR)\SPRITE.exe" "$(OUTDIR)\SPRITE.pch" "$(OUTDIR)\SPRITE.bsc"

CLEAN : 
	-@erase ".\WinRel\vc40.pdb"
	-@erase ".\WinRel\SPRITE.pch"
	-@erase ".\WinRel\SPRITE.bsc"
	-@erase ".\WinRel\Lzsscode.sbr"
	-@erase ".\WinRel\mainfrm.sbr"
	-@erase ".\WinRel\NamngDlg.sbr"
	-@erase ".\WinRel\stucki.sbr"
	-@erase ".\WinRel\vd.sbr"
	-@erase ".\WinRel\splitwnd.sbr"
	-@erase ".\WinRel\spritvw.sbr"
	-@erase ".\WinRel\rgbcolor.sbr"
	-@erase ".\WinRel\varcut.sbr"
	-@erase ".\WinRel\stdafx.sbr"
	-@erase ".\WinRel\Huffmanc.sbr"
	-@erase ".\WinRel\sp.sbr"
	-@erase ".\WinRel\drawdib.sbr"
	-@erase ".\WinRel\Bpecodec.sbr"
	-@erase ".\WinRel\medcut.sbr"
	-@erase ".\WinRel\sprite.sbr"
	-@erase ".\WinRel\cdirdlg.sbr"
	-@erase ".\WinRel\sptrctrl.sbr"
	-@erase ".\WinRel\SPProp.sbr"
	-@erase ".\WinRel\Sprcnvrt.sbr"
	-@erase ".\WinRel\StatusDlg.sbr"
	-@erase ".\WinRel\Bitbuffe.sbr"
	-@erase ".\WinRel\terrain.sbr"
	-@erase ".\WinRel\image.sbr"
	-@erase ".\WinRel\multprog.sbr"
	-@erase ".\WinRel\Codec.sbr"
	-@erase ".\WinRel\sprd.sbr"
	-@erase ".\WinRel\bmpfile.sbr"
	-@erase ".\WinRel\td.sbr"
	-@erase ".\WinRel\rbuild.sbr"
	-@erase ".\WinRel\v.sbr"
	-@erase ".\WinRel\PALDOC.SBR"
	-@erase ".\WinRel\bd.sbr"
	-@erase ".\WinRel\tgafile.sbr"
	-@erase ".\WinRel\PROGDLG.SBR"
	-@erase ".\WinRel\HoldTime.sbr"
	-@erase ".\WinRel\PALVIEW.SBR"
	-@erase ".\WinRel\boxsplit.sbr"
	-@erase ".\WinRel\Lzwcodec.sbr"
	-@erase ".\WinRel\treeview.sbr"
	-@erase ".\WinRel\SPRITE.exe"
	-@erase ".\WinRel\v.obj"
	-@erase ".\WinRel\PALDOC.OBJ"
	-@erase ".\WinRel\bd.obj"
	-@erase ".\WinRel\tgafile.obj"
	-@erase ".\WinRel\PROGDLG.OBJ"
	-@erase ".\WinRel\HoldTime.obj"
	-@erase ".\WinRel\PALVIEW.OBJ"
	-@erase ".\WinRel\boxsplit.obj"
	-@erase ".\WinRel\Lzwcodec.obj"
	-@erase ".\WinRel\treeview.obj"
	-@erase ".\WinRel\Lzsscode.obj"
	-@erase ".\WinRel\mainfrm.obj"
	-@erase ".\WinRel\NamngDlg.obj"
	-@erase ".\WinRel\stucki.obj"
	-@erase ".\WinRel\vd.obj"
	-@erase ".\WinRel\splitwnd.obj"
	-@erase ".\WinRel\spritvw.obj"
	-@erase ".\WinRel\rgbcolor.obj"
	-@erase ".\WinRel\varcut.obj"
	-@erase ".\WinRel\stdafx.obj"
	-@erase ".\WinRel\Huffmanc.obj"
	-@erase ".\WinRel\sp.obj"
	-@erase ".\WinRel\drawdib.obj"
	-@erase ".\WinRel\Bpecodec.obj"
	-@erase ".\WinRel\medcut.obj"
	-@erase ".\WinRel\sprite.obj"
	-@erase ".\WinRel\cdirdlg.obj"
	-@erase ".\WinRel\sptrctrl.obj"
	-@erase ".\WinRel\SPProp.obj"
	-@erase ".\WinRel\Sprcnvrt.obj"
	-@erase ".\WinRel\StatusDlg.obj"
	-@erase ".\WinRel\Bitbuffe.obj"
	-@erase ".\WinRel\terrain.obj"
	-@erase ".\WinRel\image.obj"
	-@erase ".\WinRel\multprog.obj"
	-@erase ".\WinRel\Codec.obj"
	-@erase ".\WinRel\sprd.obj"
	-@erase ".\WinRel\bmpfile.obj"
	-@erase ".\WinRel\td.obj"
	-@erase ".\WinRel\rbuild.obj"
	-@erase ".\WinRel\sprite.res"
	-@erase ".\WinRel\SPRITE.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W4 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /c
# SUBTRACT CPP /Yu
CPP_PROJ=/nologo /MD /W4 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/sprite.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/SPRITE.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/Lzsscode.sbr" \
	"$(INTDIR)/mainfrm.sbr" \
	"$(INTDIR)/NamngDlg.sbr" \
	"$(INTDIR)/stucki.sbr" \
	"$(INTDIR)/vd.sbr" \
	"$(INTDIR)/splitwnd.sbr" \
	"$(INTDIR)/spritvw.sbr" \
	"$(INTDIR)/rgbcolor.sbr" \
	"$(INTDIR)/varcut.sbr" \
	"$(INTDIR)/stdafx.sbr" \
	"$(INTDIR)/Huffmanc.sbr" \
	"$(INTDIR)/sp.sbr" \
	"$(INTDIR)/drawdib.sbr" \
	"$(INTDIR)/Bpecodec.sbr" \
	"$(INTDIR)/medcut.sbr" \
	"$(INTDIR)/sprite.sbr" \
	"$(INTDIR)/cdirdlg.sbr" \
	"$(INTDIR)/sptrctrl.sbr" \
	"$(INTDIR)/SPProp.sbr" \
	"$(INTDIR)/Sprcnvrt.sbr" \
	"$(INTDIR)/StatusDlg.sbr" \
	"$(INTDIR)/Bitbuffe.sbr" \
	"$(INTDIR)/terrain.sbr" \
	"$(INTDIR)/image.sbr" \
	"$(INTDIR)/multprog.sbr" \
	"$(INTDIR)/Codec.sbr" \
	"$(INTDIR)/sprd.sbr" \
	"$(INTDIR)/bmpfile.sbr" \
	"$(INTDIR)/td.sbr" \
	"$(INTDIR)/rbuild.sbr" \
	"$(INTDIR)/v.sbr" \
	"$(INTDIR)/PALDOC.SBR" \
	"$(INTDIR)/bd.sbr" \
	"$(INTDIR)/tgafile.sbr" \
	"$(INTDIR)/PROGDLG.SBR" \
	"$(INTDIR)/HoldTime.sbr" \
	"$(INTDIR)/PALVIEW.SBR" \
	"$(INTDIR)/boxsplit.sbr" \
	"$(INTDIR)/Lzwcodec.sbr" \
	"$(INTDIR)/treeview.sbr"

"$(OUTDIR)\SPRITE.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none /nodefaultlib
LINK32_FLAGS=winmm.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/SPRITE.pdb" /debug /machine:I386 /out:"$(OUTDIR)/SPRITE.exe" 
LINK32_OBJS= \
	"$(INTDIR)/v.obj" \
	"$(INTDIR)/PALDOC.OBJ" \
	"$(INTDIR)/bd.obj" \
	"$(INTDIR)/tgafile.obj" \
	"$(INTDIR)/PROGDLG.OBJ" \
	"$(INTDIR)/HoldTime.obj" \
	"$(INTDIR)/PALVIEW.OBJ" \
	"$(INTDIR)/boxsplit.obj" \
	"$(INTDIR)/Lzwcodec.obj" \
	"$(INTDIR)/treeview.obj" \
	"$(INTDIR)/Lzsscode.obj" \
	"$(INTDIR)/mainfrm.obj" \
	"$(INTDIR)/NamngDlg.obj" \
	"$(INTDIR)/stucki.obj" \
	"$(INTDIR)/vd.obj" \
	"$(INTDIR)/splitwnd.obj" \
	"$(INTDIR)/spritvw.obj" \
	"$(INTDIR)/rgbcolor.obj" \
	"$(INTDIR)/varcut.obj" \
	"$(INTDIR)/stdafx.obj" \
	"$(INTDIR)/Huffmanc.obj" \
	"$(INTDIR)/sp.obj" \
	"$(INTDIR)/drawdib.obj" \
	"$(INTDIR)/Bpecodec.obj" \
	"$(INTDIR)/medcut.obj" \
	"$(INTDIR)/sprite.obj" \
	"$(INTDIR)/cdirdlg.obj" \
	"$(INTDIR)/sptrctrl.obj" \
	"$(INTDIR)/SPProp.obj" \
	"$(INTDIR)/Sprcnvrt.obj" \
	"$(INTDIR)/StatusDlg.obj" \
	"$(INTDIR)/Bitbuffe.obj" \
	"$(INTDIR)/terrain.obj" \
	"$(INTDIR)/image.obj" \
	"$(INTDIR)/multprog.obj" \
	"$(INTDIR)/Codec.obj" \
	"$(INTDIR)/sprd.obj" \
	"$(INTDIR)/bmpfile.obj" \
	"$(INTDIR)/td.obj" \
	"$(INTDIR)/rbuild.obj" \
	"$(INTDIR)/sprite.res"

"$(OUTDIR)\SPRITE.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

MTL_PROJ=
################################################################################
# Begin Target

# Name "Sprite - Win32 Debug"
# Name "Sprite - Win32 Release"

!IF  "$(CFG)" == "Sprite - Win32 Debug"

!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\stdafx.cpp
DEP_CPP_STDAF=\
	".\stdafx.h"\
	

!IF  "$(CFG)" == "Sprite - Win32 Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W4 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/SPRITE.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\stdafx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\SPRITE.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W4 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/SPRITE.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\stdafx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\SPRITE.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sprite.cpp

!IF  "$(CFG)" == "Sprite - Win32 Debug"

DEP_CPP_SPRIT=\
	".\stdafx.h"\
	".\BD.H"\
	".\CDIRDLG.H"\
	".\SPRITE.H"\
	".\MAINFRM.H"\
	".\NAMNGDLG.H"\
	".\PALDOC.H"\
	".\PALVIEW.H"\
	".\RBUILD.H"\
	".\SPRITVW.H"\
	".\SPLITWND.H"\
	".\TD.H"\
	".\TERRAIN.H"\
	".\TREEVIEW.H"\
	".\VD.H"\
	".\statusdlg.H"\
	".\BMPFILE.H"\
	".\TGAFILE.H"\
	".\SPRD.H"\
	".\SP.H"\
	".\SPTYPES.H"\
	".\V.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\IMAGE.H"\
	".\SPTRCTRL.H"\
	".\RGBCOLOR.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\STUCKI.H"\
	".\DITHER.H"\
	".\QUANT.H"\
	".\DRAWDIB.H"\
	

"$(INTDIR)\sprite.obj" : $(SOURCE) $(DEP_CPP_SPRIT) "$(INTDIR)"

"$(INTDIR)\sprite.sbr" : $(SOURCE) $(DEP_CPP_SPRIT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

DEP_CPP_SPRIT=\
	".\stdafx.h"\
	".\BD.H"\
	".\CDIRDLG.H"\
	".\SPRITE.H"\
	".\MAINFRM.H"\
	".\NAMNGDLG.H"\
	".\PALDOC.H"\
	".\PALVIEW.H"\
	".\RBUILD.H"\
	".\SPRITVW.H"\
	".\SPLITWND.H"\
	".\TD.H"\
	".\TERRAIN.H"\
	".\TREEVIEW.H"\
	".\VD.H"\
	".\statusdlg.H"\
	".\BMPFILE.H"\
	".\TGAFILE.H"\
	".\SPRD.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\QUANT.H"\
	".\DRAWDIB.H"\
	".\IMAGE.H"\
	".\V.H"\
	".\SP.H"\
	".\SPTRCTRL.H"\
	".\DITHER.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\sprite.obj" : $(SOURCE) $(DEP_CPP_SPRIT) "$(INTDIR)"

"$(INTDIR)\sprite.sbr" : $(SOURCE) $(DEP_CPP_SPRIT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mainfrm.cpp
DEP_CPP_MAINF=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\MAINFRM.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\mainfrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"

"$(INTDIR)\mainfrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\spritvw.cpp
DEP_CPP_SPRITV=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\SPRD.H"\
	".\SPRITVW.H"\
	".\DRAWDIB.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\SP.H"\
	".\V.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\spritvw.obj" : $(SOURCE) $(DEP_CPP_SPRITV) "$(INTDIR)"

"$(INTDIR)\spritvw.sbr" : $(SOURCE) $(DEP_CPP_SPRITV) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sprite.rc
DEP_RSC_SPRITE=\
	".\res\toolbar.bmp"\
	".\res\idr_main.ico"\
	".\res\sprite.rc2"\
	

"$(INTDIR)\sprite.res" : $(SOURCE) $(DEP_RSC_SPRITE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\splitwnd.cpp

!IF  "$(CFG)" == "Sprite - Win32 Debug"

DEP_CPP_SPLIT=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\SPLITWND.H"\
	".\SPRITVW.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\SPRD.H"\
	".\DRAWDIB.H"\
	".\SP.H"\
	".\V.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\splitwnd.obj" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"

"$(INTDIR)\splitwnd.sbr" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

DEP_CPP_SPLIT=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\SPLITWND.H"\
	".\SPRITVW.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\SPRD.H"\
	".\DRAWDIB.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\SP.H"\
	".\DRAWSURF.H"\
	".\IMAGE.H"\
	".\V.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\splitwnd.obj" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"

"$(INTDIR)\splitwnd.sbr" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\treeview.cpp
DEP_CPP_TREEV=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\SPRD.H"\
	".\TREEVIEW.H"\
	".\V.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\SP.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\treeview.obj" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"

"$(INTDIR)\treeview.sbr" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PALVIEW.CPP
DEP_CPP_PALVI=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\PALVIEW.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\PALVIEW.OBJ" : $(SOURCE) $(DEP_CPP_PALVI) "$(INTDIR)"

"$(INTDIR)\PALVIEW.SBR" : $(SOURCE) $(DEP_CPP_PALVI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PALDOC.CPP
DEP_CPP_PALDO=\
	".\stdafx.h"\
	".\BOXSPLIT.H"\
	".\BMPFILE.H"\
	".\CDIRDLG.H"\
	".\MEDCUT.H"\
	".\varcut.h"\
	".\PALDOC.H"\
	".\SPRITE.H"\
	".\SPTYPES.H"\
	".\TERRAIN.H"\
	".\TGAFILE.H"\
	".\QUANT.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\STUCKI.H"\
	".\DITHER.H"\
	".\V.H"\
	".\SP.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\PALDOC.OBJ" : $(SOURCE) $(DEP_CPP_PALDO) "$(INTDIR)"

"$(INTDIR)\PALDOC.SBR" : $(SOURCE) $(DEP_CPP_PALDO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sprd.cpp
DEP_CPP_SPRD_=\
	".\stdafx.h"\
	".\CDIRDLG.H"\
	".\SPRITE.H"\
	".\SPRD.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\SP.H"\
	".\V.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\sprd.obj" : $(SOURCE) $(DEP_CPP_SPRD_) "$(INTDIR)"

"$(INTDIR)\sprd.sbr" : $(SOURCE) $(DEP_CPP_SPRD_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\v.cpp
DEP_CPP_V_CPP=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\BMPFILE.H"\
	".\TGAFILE.H"\
	".\V.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\v.obj" : $(SOURCE) $(DEP_CPP_V_CPP) "$(INTDIR)"

"$(INTDIR)\v.sbr" : $(SOURCE) $(DEP_CPP_V_CPP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sp.cpp
DEP_CPP_SP_CP=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\SP.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\V.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\sp.obj" : $(SOURCE) $(DEP_CPP_SP_CP) "$(INTDIR)"

"$(INTDIR)\sp.sbr" : $(SOURCE) $(DEP_CPP_SP_CP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\bd.cpp
DEP_CPP_BD_CP=\
	".\stdafx.h"\
	".\BD.H"\
	".\SPRD.H"\
	".\SP.H"\
	".\SPTYPES.H"\
	".\V.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\IMAGE.H"\
	".\SPTRCTRL.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\bd.obj" : $(SOURCE) $(DEP_CPP_BD_CP) "$(INTDIR)"

"$(INTDIR)\bd.sbr" : $(SOURCE) $(DEP_CPP_BD_CP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\td.cpp
DEP_CPP_TD_CP=\
	".\stdafx.h"\
	".\TD.H"\
	".\TERRAIN.H"\
	".\SPRD.H"\
	".\SP.H"\
	".\SPTYPES.H"\
	".\V.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\IMAGE.H"\
	".\SPTRCTRL.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\td.obj" : $(SOURCE) $(DEP_CPP_TD_CP) "$(INTDIR)"

"$(INTDIR)\td.sbr" : $(SOURCE) $(DEP_CPP_TD_CP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cdirdlg.cpp
DEP_CPP_CDIRD=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\CDIRDLG.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\cdirdlg.obj" : $(SOURCE) $(DEP_CPP_CDIRD) "$(INTDIR)"

"$(INTDIR)\cdirdlg.sbr" : $(SOURCE) $(DEP_CPP_CDIRD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\image.cpp
DEP_CPP_IMAGE=\
	".\stdafx.h"\
	".\BMPFILE.H"\
	".\TGAFILE.H"\
	".\IMAGE.H"\
	".\SPRITE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\DITHER.H"\
	

"$(INTDIR)\image.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"

"$(INTDIR)\image.sbr" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\drawdib.cpp
DEP_CPP_DRAWD=\
	".\stdafx.h"\
	".\DRAWDIB.H"\
	".\DRAWSURF.H"\
	".\COMMON.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\drawdib.obj" : $(SOURCE) $(DEP_CPP_DRAWD) "$(INTDIR)"

"$(INTDIR)\drawdib.sbr" : $(SOURCE) $(DEP_CPP_DRAWD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\tgafile.cpp
DEP_CPP_TGAFI=\
	".\stdafx.h"\
	".\TGAFILE.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\tgafile.obj" : $(SOURCE) $(DEP_CPP_TGAFI) "$(INTDIR)"

"$(INTDIR)\tgafile.sbr" : $(SOURCE) $(DEP_CPP_TGAFI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\bmpfile.cpp
DEP_CPP_BMPFI=\
	".\stdafx.h"\
	".\BMPFILE.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\bmpfile.obj" : $(SOURCE) $(DEP_CPP_BMPFI) "$(INTDIR)"

"$(INTDIR)\bmpfile.sbr" : $(SOURCE) $(DEP_CPP_BMPFI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\stucki.cpp
DEP_CPP_STUCK=\
	".\stdafx.h"\
	".\STUCKI.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\stucki.obj" : $(SOURCE) $(DEP_CPP_STUCK) "$(INTDIR)"

"$(INTDIR)\stucki.sbr" : $(SOURCE) $(DEP_CPP_STUCK) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\medcut.cpp
DEP_CPP_MEDCU=\
	".\stdafx.h"\
	".\MEDCUT.H"\
	".\QUANT.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\medcut.obj" : $(SOURCE) $(DEP_CPP_MEDCU) "$(INTDIR)"

"$(INTDIR)\medcut.sbr" : $(SOURCE) $(DEP_CPP_MEDCU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PROGDLG.CPP
DEP_CPP_PROGD=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\PROGDLG.OBJ" : $(SOURCE) $(DEP_CPP_PROGD) "$(INTDIR)"

"$(INTDIR)\PROGDLG.SBR" : $(SOURCE) $(DEP_CPP_PROGD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\rgbcolor.cpp
DEP_CPP_RGBCO=\
	".\RGBCOLOR.H"\
	".\COMMON.H"\
	

"$(INTDIR)\rgbcolor.obj" : $(SOURCE) $(DEP_CPP_RGBCO) "$(INTDIR)"

"$(INTDIR)\rgbcolor.sbr" : $(SOURCE) $(DEP_CPP_RGBCO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\vd.cpp
DEP_CPP_VD_CP=\
	".\stdafx.h"\
	".\VD.H"\
	".\SPRD.H"\
	".\SP.H"\
	".\SPTYPES.H"\
	".\V.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\IMAGE.H"\
	".\SPTRCTRL.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\vd.obj" : $(SOURCE) $(DEP_CPP_VD_CP) "$(INTDIR)"

"$(INTDIR)\vd.sbr" : $(SOURCE) $(DEP_CPP_VD_CP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Todo.txt

!IF  "$(CFG)" == "Sprite - Win32 Debug"

!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Formats.txt

!IF  "$(CFG)" == "Sprite - Win32 Debug"

!ELSEIF  "$(CFG)" == "Sprite - Win32 Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NamngDlg.cpp
DEP_CPP_NAMNG=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\NAMNGDLG.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\NamngDlg.obj" : $(SOURCE) $(DEP_CPP_NAMNG) "$(INTDIR)"

"$(INTDIR)\NamngDlg.sbr" : $(SOURCE) $(DEP_CPP_NAMNG) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sptrctrl.cpp
DEP_CPP_SPTRC=\
	".\stdafx.h"\
	".\Holdtime.h"\
	".\SPRITE.H"\
	".\Spprop.h"\
	".\SPTRCTRL.H"\
	".\V.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\sptrctrl.obj" : $(SOURCE) $(DEP_CPP_SPTRC) "$(INTDIR)"

"$(INTDIR)\sptrctrl.sbr" : $(SOURCE) $(DEP_CPP_SPTRC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\SPProp.cpp
DEP_CPP_SPPRO=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\Spprop.h"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\SPProp.obj" : $(SOURCE) $(DEP_CPP_SPPRO) "$(INTDIR)"

"$(INTDIR)\SPProp.sbr" : $(SOURCE) $(DEP_CPP_SPPRO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\terrain.cpp
DEP_CPP_TERRA=\
	".\stdafx.h"\
	".\BMPFILE.H"\
	".\TGAFILE.H"\
	".\SPRITE.H"\
	".\TERRAIN.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\DITHER.H"\
	".\V.H"\
	".\SP.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\terrain.obj" : $(SOURCE) $(DEP_CPP_TERRA) "$(INTDIR)"

"$(INTDIR)\terrain.sbr" : $(SOURCE) $(DEP_CPP_TERRA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\rbuild.cpp
DEP_CPP_RBUIL=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\SPTYPES.H"\
	".\SP.H"\
	".\TERRAIN.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	".\V.H"\
	".\SPTRCTRL.H"\
	

"$(INTDIR)\rbuild.obj" : $(SOURCE) $(DEP_CPP_RBUIL) "$(INTDIR)"

"$(INTDIR)\rbuild.sbr" : $(SOURCE) $(DEP_CPP_RBUIL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\multprog.cpp
DEP_CPP_MULTP=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\MULTPROG.H"\
	".\PROGDLG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\multprog.obj" : $(SOURCE) $(DEP_CPP_MULTP) "$(INTDIR)"

"$(INTDIR)\multprog.sbr" : $(SOURCE) $(DEP_CPP_MULTP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HoldTime.cpp
DEP_CPP_HOLDT=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\Holdtime.h"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\HoldTime.obj" : $(SOURCE) $(DEP_CPP_HOLDT) "$(INTDIR)"

"$(INTDIR)\HoldTime.sbr" : $(SOURCE) $(DEP_CPP_HOLDT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\boxsplit.cpp
DEP_CPP_BOXSP=\
	".\stdafx.h"\
	".\BOXSPLIT.H"\
	".\QUANT.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\boxsplit.obj" : $(SOURCE) $(DEP_CPP_BOXSP) "$(INTDIR)"

"$(INTDIR)\boxsplit.sbr" : $(SOURCE) $(DEP_CPP_BOXSP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StatusDlg.cpp
DEP_CPP_STATU=\
	".\stdafx.h"\
	".\SPRITE.H"\
	".\statusdlg.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\RGBCOLOR.H"\
	".\STUCKI.H"\
	".\SPTYPES.H"\
	".\COMMON.H"\
	".\DITHER.H"\
	".\IMAGE.H"\
	".\DRAWSURF.H"\
	

"$(INTDIR)\StatusDlg.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"

"$(INTDIR)\StatusDlg.sbr" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Sprcnvrt.cpp
DEP_CPP_SPRCN=\
	".\stdafx.h"\
	".\SP.H"\
	".\Codec.h"\
	".\SPRITE.H"\
	".\SPTYPES.H"\
	".\V.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\IMAGE.H"\
	".\SPTRCTRL.H"\
	".\RGBCOLOR.H"\
	".\PROGDLG.H"\
	".\MULTPROG.H"\
	".\STUCKI.H"\
	".\DITHER.H"\
	

"$(INTDIR)\Sprcnvrt.obj" : $(SOURCE) $(DEP_CPP_SPRCN) "$(INTDIR)"

"$(INTDIR)\Sprcnvrt.sbr" : $(SOURCE) $(DEP_CPP_SPRCN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Bpecodec.cpp
DEP_CPP_BPECO=\
	".\stdafx.h"\
	".\Bpecodec.h"\
	".\Codec.h"\
	

"$(INTDIR)\Bpecodec.obj" : $(SOURCE) $(DEP_CPP_BPECO) "$(INTDIR)"

"$(INTDIR)\Bpecodec.sbr" : $(SOURCE) $(DEP_CPP_BPECO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Codec.cpp
DEP_CPP_CODEC=\
	".\stdafx.h"\
	".\Codec.h"\
	".\Bpecodec.h"\
	".\lzsscode.h"\
	".\huffmanc.h"\
	".\lzwcodec.h"\
	".\bitbuffe.h"\
	

"$(INTDIR)\Codec.obj" : $(SOURCE) $(DEP_CPP_CODEC) "$(INTDIR)"

"$(INTDIR)\Codec.sbr" : $(SOURCE) $(DEP_CPP_CODEC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Huffmanc.cpp
DEP_CPP_HUFFM=\
	".\stdafx.h"\
	".\bitbuffe.h"\
	".\huffmanc.h"\
	".\Codec.h"\
	

"$(INTDIR)\Huffmanc.obj" : $(SOURCE) $(DEP_CPP_HUFFM) "$(INTDIR)"

"$(INTDIR)\Huffmanc.sbr" : $(SOURCE) $(DEP_CPP_HUFFM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Lzsscode.cpp
DEP_CPP_LZSSC=\
	".\stdafx.h"\
	".\bitbuffe.h"\
	".\lzsscode.h"\
	".\Codec.h"\
	

"$(INTDIR)\Lzsscode.obj" : $(SOURCE) $(DEP_CPP_LZSSC) "$(INTDIR)"

"$(INTDIR)\Lzsscode.sbr" : $(SOURCE) $(DEP_CPP_LZSSC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Lzwcodec.cpp
DEP_CPP_LZWCO=\
	".\stdafx.h"\
	".\lzwcodec.h"\
	".\bitbuffe.h"\
	".\Codec.h"\
	

"$(INTDIR)\Lzwcodec.obj" : $(SOURCE) $(DEP_CPP_LZWCO) "$(INTDIR)"

"$(INTDIR)\Lzwcodec.sbr" : $(SOURCE) $(DEP_CPP_LZWCO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Bitbuffe.cpp
DEP_CPP_BITBU=\
	".\stdafx.h"\
	".\bitbuffe.h"\
	

"$(INTDIR)\Bitbuffe.obj" : $(SOURCE) $(DEP_CPP_BITBU) "$(INTDIR)"

"$(INTDIR)\Bitbuffe.sbr" : $(SOURCE) $(DEP_CPP_BITBU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\varcut.CPP
DEP_CPP_VARCU=\
	".\stdafx.h"\
	".\varcut.h"\
	".\QUANT.H"\
	".\IMAGE.H"\
	".\COMMON.H"\
	".\DRAWSURF.H"\
	".\RGBCOLOR.H"\
	

"$(INTDIR)\varcut.obj" : $(SOURCE) $(DEP_CPP_VARCU) "$(INTDIR)"

"$(INTDIR)\varcut.sbr" : $(SOURCE) $(DEP_CPP_VARCU) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
