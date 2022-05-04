# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "VDMPLAY.MAK" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : $(OUTDIR)/VDMPLAY.dll $(OUTDIR)/VDMPLAY.map $(OUTDIR)/VDMPLAY.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /YX /O2 /I "." /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "VPSYSTEM" /D "_USRDLL" /D "_MBCS" /D "_WINDLL" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /YX /O2 /I "." /D "NDEBUG" /D "STRICT" /D "WIN32"\
 /D "_WINDOWS" /D "VPSYSTEM" /D "_USRDLL" /D "_MBCS" /D "_WINDLL" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"VDMPLAY.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "WIN32"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"VDMPLAY.res" /d "NDEBUG" /d "WIN32" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"VDMPLAY.bsc" 
BSC32_SBRS= \
	$(INTDIR)/TCPNET.SBR \
	$(INTDIR)/SMAP.SBR \
	$(INTDIR)/VDMPLAY.SBR \
	$(INTDIR)/VPENGINE.SBR \
	$(INTDIR)/STDAFX.SBR \
	$(INTDIR)/WSIPXNET.SBR \
	$(INTDIR)/NBNET.SBR \
	$(INTDIR)/VPWINSK.SBR \
	$(INTDIR)/W32COMM.SBR \
	$(INTDIR)/COMMNET.SBR \
	$(INTDIR)/DATALINK.SBR \
	$(INTDIR)/MDMDRVR.SBR \
	$(INTDIR)/DIALER.SBR \
	$(INTDIR)/astring.sbr \
	$(INTDIR)/MDMNET.SBR \
	$(INTDIR)/DATALOG.SBR \
	$(INTDIR)/vpnet.sbr \
	$(INTDIR)/advdlg.sbr \
	$(INTDIR)/comstatd.sbr

$(OUTDIR)/VDMPLAY.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 mtlmt.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MAP /MACHINE:I386
LINK32_FLAGS=mtlmt.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL\
 /INCREMENTAL:no /PDB:$(OUTDIR)/"VDMPLAY.pdb" /MAP:$(INTDIR)/"VDMPLAY.map"\
 /MACHINE:I386 /DEF:".\VDMPLAY.DEF" /OUT:$(OUTDIR)/"VDMPLAY.dll"\
 /IMPLIB:$(OUTDIR)/"VDMPLAY.lib" 
DEF_FILE=.\VDMPLAY.DEF
LINK32_OBJS= \
	$(INTDIR)/TCPNET.OBJ \
	$(INTDIR)/SMAP.OBJ \
	$(INTDIR)/VDMPLAY.OBJ \
	$(INTDIR)/VPENGINE.OBJ \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/WSIPXNET.OBJ \
	$(INTDIR)/NBNET.OBJ \
	$(INTDIR)/VPWINSK.OBJ \
	$(INTDIR)/W32COMM.OBJ \
	$(INTDIR)/COMMNET.OBJ \
	$(INTDIR)/DATALINK.OBJ \
	$(INTDIR)/MDMDRVR.OBJ \
	$(INTDIR)/DIALER.OBJ \
	$(INTDIR)/astring.obj \
	$(INTDIR)/MDMNET.OBJ \
	$(INTDIR)/VDMPLAY.res \
	$(INTDIR)/DATALOG.OBJ \
	$(INTDIR)/vpnet.obj \
	$(INTDIR)/advdlg.obj \
	$(INTDIR)/comstatd.obj

$(OUTDIR)/VDMPLAY.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/VDMPLAY.dll $(OUTDIR)/VDMPLAY.map $(OUTDIR)/VDMPLAY.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /Zi /YX /Od /I "." /D "_DEBUG" /D "RWDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "VPSYSTEM" /D "_USRDLL" /D "_MBCS" /D "_WINDLL" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /Zi /YX /Od /I "." /D "_DEBUG" /D "RWDEBUG" /D\
 "STRICT" /D "WIN32" /D "_WINDOWS" /D "VPSYSTEM" /D "_USRDLL" /D "_MBCS" /D\
 "_WINDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"VDMPLAY.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"VDMPLAY.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "WIN32"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"VDMPLAY.res" /d "_DEBUG" /d "WIN32" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"VDMPLAY.bsc" 
BSC32_SBRS= \
	$(INTDIR)/TCPNET.SBR \
	$(INTDIR)/SMAP.SBR \
	$(INTDIR)/VDMPLAY.SBR \
	$(INTDIR)/VPENGINE.SBR \
	$(INTDIR)/STDAFX.SBR \
	$(INTDIR)/WSIPXNET.SBR \
	$(INTDIR)/NBNET.SBR \
	$(INTDIR)/VPWINSK.SBR \
	$(INTDIR)/W32COMM.SBR \
	$(INTDIR)/COMMNET.SBR \
	$(INTDIR)/DATALINK.SBR \
	$(INTDIR)/MDMDRVR.SBR \
	$(INTDIR)/DIALER.SBR \
	$(INTDIR)/astring.sbr \
	$(INTDIR)/MDMNET.SBR \
	$(INTDIR)/DATALOG.SBR \
	$(INTDIR)/vpnet.sbr \
	$(INTDIR)/advdlg.sbr \
	$(INTDIR)/comstatd.sbr

$(OUTDIR)/VDMPLAY.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 nafxdwd.lib mtlgmt.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MAP /DEBUG /MACHINE:I386
LINK32_FLAGS=nafxdwd.lib mtlgmt.lib netapi32.lib /NOLOGO /SUBSYSTEM:windows\
 /DLL /INCREMENTAL:yes /PDB:$(OUTDIR)/"VDMPLAY.pdb" /MAP:$(INTDIR)/"VDMPLAY.map"\
 /DEBUG /MACHINE:I386 /DEF:".\VDMPLAY.DEF" /OUT:$(OUTDIR)/"VDMPLAY.dll"\
 /IMPLIB:$(OUTDIR)/"VDMPLAY.lib" 
DEF_FILE=.\VDMPLAY.DEF
LINK32_OBJS= \
	$(INTDIR)/TCPNET.OBJ \
	$(INTDIR)/SMAP.OBJ \
	$(INTDIR)/VDMPLAY.OBJ \
	$(INTDIR)/VPENGINE.OBJ \
	$(INTDIR)/STDAFX.OBJ \
	$(INTDIR)/WSIPXNET.OBJ \
	$(INTDIR)/NBNET.OBJ \
	$(INTDIR)/VPWINSK.OBJ \
	$(INTDIR)/W32COMM.OBJ \
	$(INTDIR)/COMMNET.OBJ \
	$(INTDIR)/DATALINK.OBJ \
	$(INTDIR)/MDMDRVR.OBJ \
	$(INTDIR)/DIALER.OBJ \
	$(INTDIR)/astring.obj \
	$(INTDIR)/MDMNET.OBJ \
	$(INTDIR)/VDMPLAY.res \
	$(INTDIR)/DATALOG.OBJ \
	$(INTDIR)/vpnet.obj \
	$(INTDIR)/advdlg.obj \
	$(INTDIR)/comstatd.obj

$(OUTDIR)/VDMPLAY.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

SOURCE=.\TCPNET.CPP
DEP_TCPNE=\
	.\STDAFX.H\
	.\VPWINSK.H\
	.\DATAGRAM.H\
	.\TCPNET.H\
	.\tdlog.h\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/TCPNET.OBJ :  $(SOURCE)  $(DEP_TCPNE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SMAP.CPP
DEP_SMAP_=\
	.\STDAFX.H\
	.\VPENGINE.H\
	.\VPLIST.H\
	.\SMAP.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/SMAP.OBJ :  $(SOURCE)  $(DEP_SMAP_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPLAY.CPP
DEP_VDMPL=\
	.\STDAFX.H\
	.\VPENGINE.H\
	.\WNOTQUE.H\
	.\VPWINSK.H\
	.\datalog.h\
	.\tdlog.h\
	.\BASE.H\
	.\W32SUT.H\
	.\DATAGRAM.H\
	.\TCPNET.H\
	.\WSIPXNET.H\
	.\IPX16NET.H\
	.\NBNET.H\
	.\COMMPORT.H\
	.\COMMNET.H\
	.\MDMNET.H\
	.\W32COMM.H\
	.\W16COMM.H\
	.\SMAP.H\
	.\VERSION.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/VDMPLAY.OBJ :  $(SOURCE)  $(DEP_VDMPL) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VPENGINE.CPP
DEP_VPENG=\
	.\STDAFX.H\
	.\VPENGINE.H\
	.\tdlog.h\
	.\SMAP.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/VPENGINE.OBJ :  $(SOURCE)  $(DEP_VPENG) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\STDAFX.CPP
DEP_STDAF=\
	.\STDAFX.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/STDAFX.OBJ :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WSIPXNET.CPP
DEP_WSIPX=\
	.\STDAFX.H\
	.\VPWINSK.H\
	.\DATAGRAM.H\
	.\WSIPXNET.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/WSIPXNET.OBJ :  $(SOURCE)  $(DEP_WSIPX) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPLAY.DEF
# End Source File
################################################################################
# Begin Source File

SOURCE=.\NBNET.CPP
DEP_NBNET=\
	.\STDAFX.H\
	.\DATAGRAM.H\
	.\NBNET.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/NBNET.OBJ :  $(SOURCE)  $(DEP_NBNET) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VPWINSK.CPP
DEP_VPWIN=\
	.\STDAFX.H\
	.\VPUTIL.H\
	.\VPWINSK.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/VPWINSK.OBJ :  $(SOURCE)  $(DEP_VPWIN) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\W32COMM.CPP
DEP_W32CO=\
	.\STDAFX.H\
	.\VPNET.H\
	.\DATAGRAM.H\
	.\COMMPORT.H\
	.\W32COMM.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/W32COMM.OBJ :  $(SOURCE)  $(DEP_W32CO) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\COMMNET.CPP
DEP_COMMN=\
	.\STDAFX.H\
	.\VPNET.H\
	.\COMMPORT.H\
	.\DATAGRAM.H\
	.\DATALINK.H\
	.\datalog.h\
	.\COMMNET.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/COMMNET.OBJ :  $(SOURCE)  $(DEP_COMMN) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DATALINK.CPP
DEP_DATAL=\
	.\STDAFX.H\
	.\tdlog.h\
	.\DATAGRAM.H\
	.\datalog.h\
	.\DATALINK.H\
	.\COMMPORT.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/DATALINK.OBJ :  $(SOURCE)  $(DEP_DATAL) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MDMDRVR.CPP
DEP_MDMDR=\
	.\STDAFX.H\
	.\ASTRING.H\
	.\ASTRLIST.H\
	.\MDMDRVR.H\
	.\COMMPORT.H\
	.\datalog.h\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\CSTRING.H\
	M:\ROGUE.61\RW\TVSLIST.H\
	M:\ROGUE.61\RW\GSLIST.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\TOOLDEFS.H\
	M:\ROGUE.61\RW\REF.H\
	M:\ROGUE.61\RW\TISLIST.H\
	M:\ROGUE.61\RW\TVSLIST.CC\
	M:\ROGUE.61\RW\SLIST.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\BACKWARD.H\
	M:\ROGUE.61\RW\ISLIST.H\
	M:\ROGUE.61\RW\TISLIST.CC\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H\
	M:\ROGUE.61\RW\MEMPOOL.H

$(INTDIR)/MDMDRVR.OBJ :  $(SOURCE)  $(DEP_MDMDR) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DIALER.CPP
DEP_DIALE=\
	.\STDAFX.H\
	.\ASTRING.H\
	.\ASTRLIST.H\
	.\MDMDRVR.H\
	.\DIALER.H\
	.\COMMPORT.H\
	.\datalog.h\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\CSTRING.H\
	M:\ROGUE.61\RW\TVSLIST.H\
	M:\ROGUE.61\RW\GSLIST.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\TOOLDEFS.H\
	M:\ROGUE.61\RW\REF.H\
	M:\ROGUE.61\RW\TISLIST.H\
	M:\ROGUE.61\RW\TVSLIST.CC\
	M:\ROGUE.61\RW\SLIST.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\BACKWARD.H\
	M:\ROGUE.61\RW\ISLIST.H\
	M:\ROGUE.61\RW\TISLIST.CC\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H\
	M:\ROGUE.61\RW\MEMPOOL.H

$(INTDIR)/DIALER.OBJ :  $(SOURCE)  $(DEP_DIALE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\astring.cpp
DEP_ASTRI=\
	.\ASTRING.H\
	M:\ROGUE.61\RW\CSTRING.H\
	M:\ROGUE.61\RW\TOOLDEFS.H\
	M:\ROGUE.61\RW\REF.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\BACKWARD.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H\
	M:\ROGUE.61\RW\GENERIC.H\
	.\GENERIC.H

$(INTDIR)/astring.obj :  $(SOURCE)  $(DEP_ASTRI) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MDMNET.CPP
DEP_MDMNE=\
	.\STDAFX.H\
	.\VPNET.H\
	.\COMMPORT.H\
	.\DATAGRAM.H\
	.\DATALINK.H\
	.\COMMNET.H\
	.\MDMNET.H\
	.\datalog.h\
	.\ASTRING.H\
	.\ASTRLIST.H\
	.\MDMDRVR.H\
	.\DIALER.H\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	M:\ROGUE.61\RW\CSTRING.H\
	M:\ROGUE.61\RW\TVSLIST.H\
	M:\ROGUE.61\RW\GSLIST.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\TOOLDEFS.H\
	M:\ROGUE.61\RW\REF.H\
	M:\ROGUE.61\RW\TISLIST.H\
	M:\ROGUE.61\RW\TVSLIST.CC\
	M:\ROGUE.61\RW\SLIST.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\BACKWARD.H\
	M:\ROGUE.61\RW\ISLIST.H\
	M:\ROGUE.61\RW\TISLIST.CC\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H\
	M:\ROGUE.61\RW\MEMPOOL.H

$(INTDIR)/MDMNET.OBJ :  $(SOURCE)  $(DEP_MDMNE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPLAY.RC
DEP_VDMPLA=\
	.\ICON1.ICO\
	.\VERSION.RC\
	.\VER32.RC\
	.\VERSION.H

$(INTDIR)/VDMPLAY.res :  $(SOURCE)  $(DEP_VDMPLA) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DATALOG.CPP
DEP_DATALO=\
	.\comstatd.h\
	.\STDAFX.H\
	.\HELP.H\
	.\datalog.h\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/DATALOG.OBJ :  $(SOURCE)  $(DEP_DATALO) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpnet.cpp
DEP_VPNET=\
	.\STDAFX.H\
	.\VPNET.H\
	.\datalog.h\
	.\GENERIC.H\
	.\VDMPLAY.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/vpnet.obj :  $(SOURCE)  $(DEP_VPNET) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\advdlg.cpp
DEP_ADVDL=\
	.\STDAFX.H\
	.\VDMPLAY.H\
	.\advdlg.h\
	.\BASE.H\
	.\HELP.H\
	.\GENERIC.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/advdlg.obj :  $(SOURCE)  $(DEP_ADVDL) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\comstatd.cpp
DEP_COMST=\
	.\STDAFX.H\
	.\VDMPLAY.H\
	.\datalog.h\
	.\comstatd.h\
	.\HELP.H\
	.\GENERIC.H\
	.\VPUTIL.H\
	.\VPLIST.H\
	.\tdlog.h\
	.\VPNET.H\
	M:\ROGUE.61\RW\GENERIC.H\
	M:\ROGUE.61\RW\DEFS.H\
	M:\ROGUE.61\RW\COMPILER.H\
	M:\ROGUE.61\RW\RWWIND.H

$(INTDIR)/comstatd.obj :  $(SOURCE)  $(DEP_COMST) $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
