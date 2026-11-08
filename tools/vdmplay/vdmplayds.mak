# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Vdmplay - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Vdmplay - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Vdmplay - Win32 Release" && "$(CFG)" !=\
 "Vdmplay - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "vdmplayds.MAK" CFG="Vdmplay - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vdmplay - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Vdmplay - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "Vdmplay - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

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

ALL : "$(OUTDIR)\vdmplay.dll" "$(OUTDIR)\vdmplayds.bsc"

CLEAN : 
	-@erase ".\WinRel\vc40.pdb"
	-@erase ".\WinRel\vdmplayds.bsc"
	-@erase ".\WinRel\TCPNET.SBR"
	-@erase ".\WinRel\VPENGINE.SBR"
	-@erase ".\WinRel\frcoder.sbr"
	-@erase ".\WinRel\vpnet.sbr"
	-@erase ".\WinRel\comstatd.sbr"
	-@erase ".\WinRel\tapinet.sbr"
	-@erase ".\WinRel\dpnet.sbr"
	-@erase ".\WinRel\vputil.sbr"
	-@erase ".\WinRel\WSIPXNET.SBR"
	-@erase ".\WinRel\SMAP.SBR"
	-@erase ".\WinRel\VDMPLAY.SBR"
	-@erase ".\WinRel\COMMNET.SBR"
	-@erase ".\WinRel\MDMNET.SBR"
	-@erase ".\WinRel\STDAFX.SBR"
	-@erase ".\WinRel\tapiutil.sbr"
	-@erase ".\WinRel\astring.sbr"
	-@erase ".\WinRel\MDMDRVR.SBR"
	-@erase ".\WinRel\DATALOG.SBR"
	-@erase ".\WinRel\NBNET.SBR"
	-@erase ".\WinRel\DPENGINE.SBR"
	-@erase ".\WinRel\W32COMM.SBR"
	-@erase ".\WinRel\Advanced.sbr"
	-@erase ".\WinRel\VPWINSK.SBR"
	-@erase ".\WinRel\DATALINK.SBR"
	-@erase ".\WinRel\DIALER.SBR"
	-@erase ".\WinRel\vdmplay.dll"
	-@erase ".\WinRel\Advanced.obj"
	-@erase ".\WinRel\VPWINSK.OBJ"
	-@erase ".\WinRel\DATALINK.OBJ"
	-@erase ".\WinRel\DIALER.OBJ"
	-@erase ".\WinRel\TCPNET.OBJ"
	-@erase ".\WinRel\VPENGINE.OBJ"
	-@erase ".\WinRel\frcoder.obj"
	-@erase ".\WinRel\vpnet.obj"
	-@erase ".\WinRel\comstatd.obj"
	-@erase ".\WinRel\tapinet.obj"
	-@erase ".\WinRel\dpnet.obj"
	-@erase ".\WinRel\vputil.obj"
	-@erase ".\WinRel\WSIPXNET.OBJ"
	-@erase ".\WinRel\SMAP.OBJ"
	-@erase ".\WinRel\VDMPLAY.OBJ"
	-@erase ".\WinRel\COMMNET.OBJ"
	-@erase ".\WinRel\MDMNET.OBJ"
	-@erase ".\WinRel\STDAFX.OBJ"
	-@erase ".\WinRel\tapiutil.obj"
	-@erase ".\WinRel\astring.obj"
	-@erase ".\WinRel\MDMDRVR.OBJ"
	-@erase ".\WinRel\DATALOG.OBJ"
	-@erase ".\WinRel\NBNET.OBJ"
	-@erase ".\WinRel\DPENGINE.OBJ"
	-@erase ".\WinRel\W32COMM.OBJ"
	-@erase ".\WinRel\VDMPLAY.res"
	-@erase ".\WinRel\vdmplay.lib"
	-@erase ".\WinRel\vdmplay.exp"
	-@erase ".\WinRel\vdmplay.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "." /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "VPSYSTEM" /D "_MBCS" /D "NOMFC" /D "USE_VPMEM" /Fr /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /Zi /O2 /I "." /D "NDEBUG" /D "STRICT" /D "WIN32"\
 /D "_WINDOWS" /D "VPSYSTEM" /D "_MBCS" /D "NOMFC" /D "USE_VPMEM"\
 /Fr"$(INTDIR)/" /Fp"$(INTDIR)/vdmplayds.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "WIN32"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/VDMPLAY.res" /d "NDEBUG" /d "WIN32" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vdmplayds.bsc" 
BSC32_SBRS= \
	".\WinRel\TCPNET.SBR" \
	".\WinRel\VPENGINE.SBR" \
	".\WinRel\frcoder.sbr" \
	".\WinRel\vpnet.sbr" \
	".\WinRel\comstatd.sbr" \
	".\WinRel\tapinet.sbr" \
	".\WinRel\dpnet.sbr" \
	".\WinRel\vputil.sbr" \
	".\WinRel\WSIPXNET.SBR" \
	".\WinRel\SMAP.SBR" \
	".\WinRel\VDMPLAY.SBR" \
	".\WinRel\COMMNET.SBR" \
	".\WinRel\MDMNET.SBR" \
	".\WinRel\STDAFX.SBR" \
	".\WinRel\tapiutil.sbr" \
	".\WinRel\astring.sbr" \
	".\WinRel\MDMDRVR.SBR" \
	".\WinRel\DATALOG.SBR" \
	".\WinRel\NBNET.SBR" \
	".\WinRel\DPENGINE.SBR" \
	".\WinRel\W32COMM.SBR" \
	".\WinRel\Advanced.sbr" \
	".\WinRel\VPWINSK.SBR" \
	".\WinRel\DATALINK.SBR" \
	".\WinRel\DIALER.SBR"

"$(OUTDIR)\vdmplayds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 mtlmt.lib netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"WinRel\vdmplay.dll"
# SUBTRACT LINK32 /map
LINK32_FLAGS=mtlmt.lib netapi32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)/vdmplay.pdb" /debug /machine:I386\
 /def:".\VDMPLAY.DEF" /out:"$(OUTDIR)/vdmplay.dll"\
 /implib:"$(OUTDIR)/vdmplay.lib" 
DEF_FILE= \
	".\VDMPLAY.DEF"
LINK32_OBJS= \
	".\WinRel\Advanced.obj" \
	".\WinRel\VPWINSK.OBJ" \
	".\WinRel\DATALINK.OBJ" \
	".\WinRel\DIALER.OBJ" \
	".\WinRel\TCPNET.OBJ" \
	".\WinRel\VPENGINE.OBJ" \
	".\WinRel\frcoder.obj" \
	".\WinRel\vpnet.obj" \
	".\WinRel\comstatd.obj" \
	".\WinRel\tapinet.obj" \
	".\WinRel\dpnet.obj" \
	".\WinRel\vputil.obj" \
	".\WinRel\WSIPXNET.OBJ" \
	".\WinRel\SMAP.OBJ" \
	".\WinRel\VDMPLAY.OBJ" \
	".\WinRel\COMMNET.OBJ" \
	".\WinRel\MDMNET.OBJ" \
	".\WinRel\STDAFX.OBJ" \
	".\WinRel\tapiutil.obj" \
	".\WinRel\astring.obj" \
	".\WinRel\MDMDRVR.OBJ" \
	".\WinRel\DATALOG.OBJ" \
	".\WinRel\NBNET.OBJ" \
	".\WinRel\DPENGINE.OBJ" \
	".\WinRel\W32COMM.OBJ" \
	".\WinRel\VDMPLAY.res"

"$(OUTDIR)\vdmplay.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

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

ALL : "$(OUTDIR)\vdmplay.dll" "$(OUTDIR)\vdmplayds.bsc"

CLEAN : 
	-@erase ".\WinDebug\vc40.pdb"
	-@erase ".\WinDebug\vc40.idb"
	-@erase ".\WinDebug\vdmplayds.bsc"
	-@erase ".\WinDebug\SMAP.SBR"
	-@erase ".\WinDebug\VDMPLAY.SBR"
	-@erase ".\WinDebug\WSIPXNET.SBR"
	-@erase ".\WinDebug\MDMNET.SBR"
	-@erase ".\WinDebug\DPENGINE.SBR"
	-@erase ".\WinDebug\Advanced.sbr"
	-@erase ".\WinDebug\COMMNET.SBR"
	-@erase ".\WinDebug\astring.sbr"
	-@erase ".\WinDebug\DATALINK.SBR"
	-@erase ".\WinDebug\tapiutil.sbr"
	-@erase ".\WinDebug\MDMDRVR.SBR"
	-@erase ".\WinDebug\DATALOG.SBR"
	-@erase ".\WinDebug\tapinet.sbr"
	-@erase ".\WinDebug\W32COMM.SBR"
	-@erase ".\WinDebug\DIALER.SBR"
	-@erase ".\WinDebug\VPWINSK.SBR"
	-@erase ".\WinDebug\NBNET.SBR"
	-@erase ".\WinDebug\vpnet.sbr"
	-@erase ".\WinDebug\TCPNET.SBR"
	-@erase ".\WinDebug\STDAFX.SBR"
	-@erase ".\WinDebug\dpnet.sbr"
	-@erase ".\WinDebug\vputil.sbr"
	-@erase ".\WinDebug\frcoder.sbr"
	-@erase ".\WinDebug\VPENGINE.SBR"
	-@erase ".\WinDebug\comstatd.sbr"
	-@erase ".\WinDebug\vdmplay.dll"
	-@erase ".\WinDebug\vputil.obj"
	-@erase ".\WinDebug\frcoder.obj"
	-@erase ".\WinDebug\VPENGINE.OBJ"
	-@erase ".\WinDebug\comstatd.obj"
	-@erase ".\WinDebug\SMAP.OBJ"
	-@erase ".\WinDebug\VDMPLAY.OBJ"
	-@erase ".\WinDebug\WSIPXNET.OBJ"
	-@erase ".\WinDebug\MDMNET.OBJ"
	-@erase ".\WinDebug\DPENGINE.OBJ"
	-@erase ".\WinDebug\Advanced.obj"
	-@erase ".\WinDebug\COMMNET.OBJ"
	-@erase ".\WinDebug\astring.obj"
	-@erase ".\WinDebug\DATALINK.OBJ"
	-@erase ".\WinDebug\tapiutil.obj"
	-@erase ".\WinDebug\MDMDRVR.OBJ"
	-@erase ".\WinDebug\DATALOG.OBJ"
	-@erase ".\WinDebug\tapinet.obj"
	-@erase ".\WinDebug\W32COMM.OBJ"
	-@erase ".\WinDebug\DIALER.OBJ"
	-@erase ".\WinDebug\VPWINSK.OBJ"
	-@erase ".\WinDebug\NBNET.OBJ"
	-@erase ".\WinDebug\vpnet.obj"
	-@erase ".\WinDebug\TCPNET.OBJ"
	-@erase ".\WinDebug\STDAFX.OBJ"
	-@erase ".\WinDebug\dpnet.obj"
	-@erase ".\WinDebug\VDMPLAY.res"
	-@erase ".\WinDebug\vdmplay.ilk"
	-@erase ".\WinDebug\vdmplay.lib"
	-@erase ".\WinDebug\vdmplay.exp"
	-@erase ".\WinDebug\vdmplay.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "RWDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D "VPSYSTEM" /D "_MBCS" /D "NOMFC" /D "USE_VPMEM" /Fr /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "RWDEBUG" /D\
 "STRICT" /D "WIN32" /D "_WINDOWS" /D "VPSYSTEM" /D "_MBCS" /D "NOMFC" /D\
 "USE_VPMEM" /Fr"$(INTDIR)/" /Fp"$(INTDIR)/vdmplayds.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "WIN32"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/VDMPLAY.res" /d "_DEBUG" /d "WIN32" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vdmplayds.bsc" 
BSC32_SBRS= \
	".\WinDebug\SMAP.SBR" \
	".\WinDebug\VDMPLAY.SBR" \
	".\WinDebug\WSIPXNET.SBR" \
	".\WinDebug\MDMNET.SBR" \
	".\WinDebug\DPENGINE.SBR" \
	".\WinDebug\Advanced.sbr" \
	".\WinDebug\COMMNET.SBR" \
	".\WinDebug\astring.sbr" \
	".\WinDebug\DATALINK.SBR" \
	".\WinDebug\tapiutil.sbr" \
	".\WinDebug\MDMDRVR.SBR" \
	".\WinDebug\DATALOG.SBR" \
	".\WinDebug\tapinet.sbr" \
	".\WinDebug\W32COMM.SBR" \
	".\WinDebug\DIALER.SBR" \
	".\WinDebug\VPWINSK.SBR" \
	".\WinDebug\NBNET.SBR" \
	".\WinDebug\vpnet.sbr" \
	".\WinDebug\TCPNET.SBR" \
	".\WinDebug\STDAFX.SBR" \
	".\WinDebug\dpnet.sbr" \
	".\WinDebug\vputil.sbr" \
	".\WinDebug\frcoder.sbr" \
	".\WinDebug\VPENGINE.SBR" \
	".\WinDebug\comstatd.sbr"

"$(OUTDIR)\vdmplayds.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 mtlgmt.lib netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"WinDebug\vdmplay.dll"
# SUBTRACT LINK32 /map
LINK32_FLAGS=mtlgmt.lib netapi32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)/vdmplay.pdb" /debug /machine:I386\
 /def:".\VDMPLAY.DEF" /out:"$(OUTDIR)/vdmplay.dll"\
 /implib:"$(OUTDIR)/vdmplay.lib" 
DEF_FILE= \
	".\VDMPLAY.DEF"
LINK32_OBJS= \
	".\WinDebug\vputil.obj" \
	".\WinDebug\frcoder.obj" \
	".\WinDebug\VPENGINE.OBJ" \
	".\WinDebug\comstatd.obj" \
	".\WinDebug\SMAP.OBJ" \
	".\WinDebug\VDMPLAY.OBJ" \
	".\WinDebug\WSIPXNET.OBJ" \
	".\WinDebug\MDMNET.OBJ" \
	".\WinDebug\DPENGINE.OBJ" \
	".\WinDebug\Advanced.obj" \
	".\WinDebug\COMMNET.OBJ" \
	".\WinDebug\astring.obj" \
	".\WinDebug\DATALINK.OBJ" \
	".\WinDebug\tapiutil.obj" \
	".\WinDebug\MDMDRVR.OBJ" \
	".\WinDebug\DATALOG.OBJ" \
	".\WinDebug\tapinet.obj" \
	".\WinDebug\W32COMM.OBJ" \
	".\WinDebug\DIALER.OBJ" \
	".\WinDebug\VPWINSK.OBJ" \
	".\WinDebug\NBNET.OBJ" \
	".\WinDebug\vpnet.obj" \
	".\WinDebug\TCPNET.OBJ" \
	".\WinDebug\STDAFX.OBJ" \
	".\WinDebug\dpnet.obj" \
	".\WinDebug\VDMPLAY.res"

"$(OUTDIR)\vdmplay.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "Vdmplay - Win32 Release"
# Name "Vdmplay - Win32 Debug"

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\TCPNET.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_TCPNE=\
	".\Stdafx.h"\
	".\Vpwinsk.h"\
	".\Datagram.h"\
	".\Tcpnet.h"\
	".\Tdlog.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\TCPNET.OBJ" : $(SOURCE) $(DEP_CPP_TCPNE) "$(INTDIR)"

"$(INTDIR)\TCPNET.SBR" : $(SOURCE) $(DEP_CPP_TCPNE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_TCPNE=\
	".\Stdafx.h"\
	".\Vpwinsk.h"\
	".\Datagram.h"\
	".\Tcpnet.h"\
	".\Tdlog.h"\
	".\nb30.h"\
	

"$(INTDIR)\TCPNET.OBJ" : $(SOURCE) $(DEP_CPP_TCPNE) "$(INTDIR)"

"$(INTDIR)\TCPNET.SBR" : $(SOURCE) $(DEP_CPP_TCPNE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SMAP.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_SMAP_=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Vplist.h"\
	".\Smap.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\SMAP.OBJ" : $(SOURCE) $(DEP_CPP_SMAP_) "$(INTDIR)"

"$(INTDIR)\SMAP.SBR" : $(SOURCE) $(DEP_CPP_SMAP_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_SMAP_=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Vplist.h"\
	".\Smap.h"\
	".\nb30.h"\
	

"$(INTDIR)\SMAP.OBJ" : $(SOURCE) $(DEP_CPP_SMAP_) "$(INTDIR)"

"$(INTDIR)\SMAP.SBR" : $(SOURCE) $(DEP_CPP_SMAP_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPLAY.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_VDMPL=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Wnotque.h"\
	".\Vpwinsk.h"\
	".\Datalog.h"\
	".\Tdlog.h"\
	".\Base.h"\
	".\nb30.h"\
	".\W32sut.h"\
	".\Datagram.h"\
	".\Tcpnet.h"\
	".\Wsipxnet.h"\
	".\Ipx16net.h"\
	".\Nbnet.h"\
	".\Commport.h"\
	".\Commnet.h"\
	".\Mdmnet.h"\
	".\W32comm.h"\
	".\W16comm.h"\
	".\Smap.h"\
	".\Version.h"\
	".\Vpint.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\VDMPLAY.OBJ" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"

"$(INTDIR)\VDMPLAY.SBR" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_VDMPL=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Wnotque.h"\
	".\Vpwinsk.h"\
	".\Datalog.h"\
	".\Tdlog.h"\
	".\Base.h"\
	".\nb30.h"\
	".\W32sut.h"\
	".\Datagram.h"\
	".\Tcpnet.h"\
	".\Wsipxnet.h"\
	".\Ipx16net.h"\
	".\Nbnet.h"\
	".\Commport.h"\
	".\Commnet.h"\
	".\Mdmnet.h"\
	".\W32comm.h"\
	".\W16comm.h"\
	".\Smap.h"\
	".\Version.h"\
	".\Vpint.h"\
	

"$(INTDIR)\VDMPLAY.OBJ" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"

"$(INTDIR)\VDMPLAY.SBR" : $(SOURCE) $(DEP_CPP_VDMPL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VPENGINE.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_VPENG=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Tdlog.h"\
	".\Smap.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\VPENGINE.OBJ" : $(SOURCE) $(DEP_CPP_VPENG) "$(INTDIR)"

"$(INTDIR)\VPENGINE.SBR" : $(SOURCE) $(DEP_CPP_VPENG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_VPENG=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Tdlog.h"\
	".\Smap.h"\
	".\nb30.h"\
	

"$(INTDIR)\VPENGINE.OBJ" : $(SOURCE) $(DEP_CPP_VPENG) "$(INTDIR)"

"$(INTDIR)\VPENGINE.SBR" : $(SOURCE) $(DEP_CPP_VPENG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\STDAFX.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_STDAF=\
	".\Stdafx.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\STDAFX.OBJ" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

"$(INTDIR)\STDAFX.SBR" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_STDAF=\
	".\Stdafx.h"\
	".\nb30.h"\
	

"$(INTDIR)\STDAFX.OBJ" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

"$(INTDIR)\STDAFX.SBR" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WSIPXNET.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_WSIPX=\
	".\Stdafx.h"\
	".\Vpwinsk.h"\
	".\Datagram.h"\
	".\Wsipxnet.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\WSIPXNET.OBJ" : $(SOURCE) $(DEP_CPP_WSIPX) "$(INTDIR)"

"$(INTDIR)\WSIPXNET.SBR" : $(SOURCE) $(DEP_CPP_WSIPX) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_WSIPX=\
	".\Stdafx.h"\
	".\Vpwinsk.h"\
	".\Datagram.h"\
	".\Wsipxnet.h"\
	".\nb30.h"\
	

"$(INTDIR)\WSIPXNET.OBJ" : $(SOURCE) $(DEP_CPP_WSIPX) "$(INTDIR)"

"$(INTDIR)\WSIPXNET.SBR" : $(SOURCE) $(DEP_CPP_WSIPX) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPLAY.DEF

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NBNET.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_NBNET=\
	".\Stdafx.h"\
	".\nb30.h"\
	".\Datagram.h"\
	".\Nbnet.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\NBNET.OBJ" : $(SOURCE) $(DEP_CPP_NBNET) "$(INTDIR)"

"$(INTDIR)\NBNET.SBR" : $(SOURCE) $(DEP_CPP_NBNET) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_NBNET=\
	".\Stdafx.h"\
	".\nb30.h"\
	".\Datagram.h"\
	".\Nbnet.h"\
	

"$(INTDIR)\NBNET.OBJ" : $(SOURCE) $(DEP_CPP_NBNET) "$(INTDIR)"

"$(INTDIR)\NBNET.SBR" : $(SOURCE) $(DEP_CPP_NBNET) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VPWINSK.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_VPWIN=\
	".\Stdafx.h"\
	".\Vputil.h"\
	".\Vpwinsk.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\VPWINSK.OBJ" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"

"$(INTDIR)\VPWINSK.SBR" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_VPWIN=\
	".\Stdafx.h"\
	".\Vputil.h"\
	".\Vpwinsk.h"\
	".\nb30.h"\
	

"$(INTDIR)\VPWINSK.OBJ" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"

"$(INTDIR)\VPWINSK.SBR" : $(SOURCE) $(DEP_CPP_VPWIN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\W32COMM.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_W32CO=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Datagram.h"\
	".\Commport.h"\
	".\W32comm.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\W32COMM.OBJ" : $(SOURCE) $(DEP_CPP_W32CO) "$(INTDIR)"

"$(INTDIR)\W32COMM.SBR" : $(SOURCE) $(DEP_CPP_W32CO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_W32CO=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Datagram.h"\
	".\Commport.h"\
	".\W32comm.h"\
	".\nb30.h"\
	

"$(INTDIR)\W32COMM.OBJ" : $(SOURCE) $(DEP_CPP_W32CO) "$(INTDIR)"

"$(INTDIR)\W32COMM.SBR" : $(SOURCE) $(DEP_CPP_W32CO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\COMMNET.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_COMMN=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Commport.h"\
	".\Datagram.h"\
	".\Datalink.h"\
	".\Datalog.h"\
	".\Commnet.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\COMMNET.OBJ" : $(SOURCE) $(DEP_CPP_COMMN) "$(INTDIR)"

"$(INTDIR)\COMMNET.SBR" : $(SOURCE) $(DEP_CPP_COMMN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_COMMN=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Commport.h"\
	".\Datagram.h"\
	".\Datalink.h"\
	".\Datalog.h"\
	".\Commnet.h"\
	".\nb30.h"\
	

"$(INTDIR)\COMMNET.OBJ" : $(SOURCE) $(DEP_CPP_COMMN) "$(INTDIR)"

"$(INTDIR)\COMMNET.SBR" : $(SOURCE) $(DEP_CPP_COMMN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DATALINK.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_DATAL=\
	".\Stdafx.h"\
	".\Tdlog.h"\
	".\Datagram.h"\
	".\Datalog.h"\
	".\Datalink.h"\
	".\Commport.h"\
	".\Frcoder.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\DATALINK.OBJ" : $(SOURCE) $(DEP_CPP_DATAL) "$(INTDIR)"

"$(INTDIR)\DATALINK.SBR" : $(SOURCE) $(DEP_CPP_DATAL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_DATAL=\
	".\Stdafx.h"\
	".\Tdlog.h"\
	".\Datagram.h"\
	".\Datalog.h"\
	".\Datalink.h"\
	".\Commport.h"\
	".\Frcoder.h"\
	".\nb30.h"\
	

"$(INTDIR)\DATALINK.OBJ" : $(SOURCE) $(DEP_CPP_DATAL) "$(INTDIR)"

"$(INTDIR)\DATALINK.SBR" : $(SOURCE) $(DEP_CPP_DATAL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MDMDRVR.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_MDMDR=\
	".\Stdafx.h"\
	".\Astring.h"\
	".\Astrlist.h"\
	".\Mdmdrvr.h"\
	".\Commport.h"\
	".\Datalog.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	{$(INCLUDE)}"\rw\gslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\slist.h"\
	

"$(INTDIR)\MDMDRVR.OBJ" : $(SOURCE) $(DEP_CPP_MDMDR) "$(INTDIR)"

"$(INTDIR)\MDMDRVR.SBR" : $(SOURCE) $(DEP_CPP_MDMDR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_MDMDR=\
	".\Stdafx.h"\
	".\Astring.h"\
	".\Astrlist.h"\
	".\Mdmdrvr.h"\
	".\Commport.h"\
	".\Datalog.h"\
	".\nb30.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	".\generic.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	{$(INCLUDE)}"\rw\gslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\slist.h"\
	

"$(INTDIR)\MDMDRVR.OBJ" : $(SOURCE) $(DEP_CPP_MDMDR) "$(INTDIR)"

"$(INTDIR)\MDMDRVR.SBR" : $(SOURCE) $(DEP_CPP_MDMDR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DIALER.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_DIALE=\
	".\Stdafx.h"\
	".\Astring.h"\
	".\Astrlist.h"\
	".\Mdmdrvr.h"\
	".\Dialer.h"\
	".\Commport.h"\
	".\Datalog.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	{$(INCLUDE)}"\rw\gslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\slist.h"\
	

"$(INTDIR)\DIALER.OBJ" : $(SOURCE) $(DEP_CPP_DIALE) "$(INTDIR)"

"$(INTDIR)\DIALER.SBR" : $(SOURCE) $(DEP_CPP_DIALE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_DIALE=\
	".\Stdafx.h"\
	".\Astring.h"\
	".\Astrlist.h"\
	".\Mdmdrvr.h"\
	".\Dialer.h"\
	".\Commport.h"\
	".\Datalog.h"\
	".\nb30.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	".\generic.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	{$(INCLUDE)}"\rw\gslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\slist.h"\
	

"$(INTDIR)\DIALER.OBJ" : $(SOURCE) $(DEP_CPP_DIALE) "$(INTDIR)"

"$(INTDIR)\DIALER.SBR" : $(SOURCE) $(DEP_CPP_DIALE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\astring.cpp
DEP_CPP_ASTRI=\
	".\Astring.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	".\generic.h"\
	

"$(INTDIR)\astring.obj" : $(SOURCE) $(DEP_CPP_ASTRI) "$(INTDIR)"

"$(INTDIR)\astring.sbr" : $(SOURCE) $(DEP_CPP_ASTRI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MDMNET.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_MDMNE=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Commport.h"\
	".\Datagram.h"\
	".\Datalink.h"\
	".\Commnet.h"\
	".\Mdmnet.h"\
	".\Datalog.h"\
	".\Astring.h"\
	".\Astrlist.h"\
	".\Mdmdrvr.h"\
	".\Dialer.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	{$(INCLUDE)}"\rw\gslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\slist.h"\
	

"$(INTDIR)\MDMNET.OBJ" : $(SOURCE) $(DEP_CPP_MDMNE) "$(INTDIR)"

"$(INTDIR)\MDMNET.SBR" : $(SOURCE) $(DEP_CPP_MDMNE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_MDMNE=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Commport.h"\
	".\Datagram.h"\
	".\Datalink.h"\
	".\Commnet.h"\
	".\Mdmnet.h"\
	".\Datalog.h"\
	".\Astring.h"\
	".\Astrlist.h"\
	".\Mdmdrvr.h"\
	".\Dialer.h"\
	".\nb30.h"\
	{$(INCLUDE)}"\Rw\Cstring.h"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\ref.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	".\generic.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	{$(INCLUDE)}"\rw\gslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\slist.h"\
	

"$(INTDIR)\MDMNET.OBJ" : $(SOURCE) $(DEP_CPP_MDMNE) "$(INTDIR)"

"$(INTDIR)\MDMNET.SBR" : $(SOURCE) $(DEP_CPP_MDMNE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VDMPLAY.RC
DEP_RSC_VDMPLA=\
	".\ICON1.ICO"\
	".\bitmap1.bmp"\
	

"$(INTDIR)\VDMPLAY.res" : $(SOURCE) $(DEP_RSC_VDMPLA) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DATALOG.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_DATALO=\
	".\Stdafx.h"\
	".\Help.h"\
	".\Datalog.h"\
	".\Comstatd.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\DATALOG.OBJ" : $(SOURCE) $(DEP_CPP_DATALO) "$(INTDIR)"

"$(INTDIR)\DATALOG.SBR" : $(SOURCE) $(DEP_CPP_DATALO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_DATALO=\
	".\Stdafx.h"\
	".\Help.h"\
	".\Datalog.h"\
	".\Comstatd.h"\
	".\nb30.h"\
	

"$(INTDIR)\DATALOG.OBJ" : $(SOURCE) $(DEP_CPP_DATALO) "$(INTDIR)"

"$(INTDIR)\DATALOG.SBR" : $(SOURCE) $(DEP_CPP_DATALO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vpnet.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_VPNET=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Datalog.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\vpnet.obj" : $(SOURCE) $(DEP_CPP_VPNET) "$(INTDIR)"

"$(INTDIR)\vpnet.sbr" : $(SOURCE) $(DEP_CPP_VPNET) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_VPNET=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Datalog.h"\
	".\nb30.h"\
	

"$(INTDIR)\vpnet.obj" : $(SOURCE) $(DEP_CPP_VPNET) "$(INTDIR)"

"$(INTDIR)\vpnet.sbr" : $(SOURCE) $(DEP_CPP_VPNET) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\comstatd.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_COMST=\
	".\Stdafx.h"\
	".\Vdmplay.h"\
	".\Datalog.h"\
	".\Comstatd.h"\
	".\Help.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\comstatd.obj" : $(SOURCE) $(DEP_CPP_COMST) "$(INTDIR)"

"$(INTDIR)\comstatd.sbr" : $(SOURCE) $(DEP_CPP_COMST) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_COMST=\
	".\Stdafx.h"\
	".\Vdmplay.h"\
	".\Datalog.h"\
	".\Comstatd.h"\
	".\Help.h"\
	".\nb30.h"\
	

"$(INTDIR)\comstatd.obj" : $(SOURCE) $(DEP_CPP_COMST) "$(INTDIR)"

"$(INTDIR)\comstatd.sbr" : $(SOURCE) $(DEP_CPP_COMST) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Advanced.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_ADVAN=\
	".\Stdafx.h"\
	".\Base.h"\
	".\Help.h"\
	".\Version.h"\
	".\nb30.h"\
	".\Tapiutil.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	
NODEP_CPP_ADVAN=\
	".\dplay.h"\
	

"$(INTDIR)\Advanced.obj" : $(SOURCE) $(DEP_CPP_ADVAN) "$(INTDIR)"

"$(INTDIR)\Advanced.sbr" : $(SOURCE) $(DEP_CPP_ADVAN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_ADVAN=\
	".\Stdafx.h"\
	".\Base.h"\
	".\Help.h"\
	".\Version.h"\
	".\nb30.h"\
	".\Tapiutil.h"\
	
NODEP_CPP_ADVAN=\
	".\dplay.h"\
	

"$(INTDIR)\Advanced.obj" : $(SOURCE) $(DEP_CPP_ADVAN) "$(INTDIR)"

"$(INTDIR)\Advanced.sbr" : $(SOURCE) $(DEP_CPP_ADVAN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tapiutil.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_TAPIU=\
	".\Stdafx.h"\
	".\Tapiutil.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\tapiutil.obj" : $(SOURCE) $(DEP_CPP_TAPIU) "$(INTDIR)"

"$(INTDIR)\tapiutil.sbr" : $(SOURCE) $(DEP_CPP_TAPIU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_TAPIU=\
	".\Stdafx.h"\
	".\Tapiutil.h"\
	".\nb30.h"\
	

"$(INTDIR)\tapiutil.obj" : $(SOURCE) $(DEP_CPP_TAPIU) "$(INTDIR)"

"$(INTDIR)\tapiutil.sbr" : $(SOURCE) $(DEP_CPP_TAPIU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tapinet.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_TAPIN=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Commport.h"\
	".\Datagram.h"\
	".\Datalink.h"\
	".\Commnet.h"\
	".\Tapiutil.h"\
	".\Tapinet.h"\
	".\Datalog.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\tapinet.obj" : $(SOURCE) $(DEP_CPP_TAPIN) "$(INTDIR)"

"$(INTDIR)\tapinet.sbr" : $(SOURCE) $(DEP_CPP_TAPIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_TAPIN=\
	".\Stdafx.h"\
	".\Vpnet.h"\
	".\Commport.h"\
	".\Datagram.h"\
	".\Datalink.h"\
	".\Commnet.h"\
	".\Tapiutil.h"\
	".\Tapinet.h"\
	".\Datalog.h"\
	".\nb30.h"\
	

"$(INTDIR)\tapinet.obj" : $(SOURCE) $(DEP_CPP_TAPIN) "$(INTDIR)"

"$(INTDIR)\tapinet.sbr" : $(SOURCE) $(DEP_CPP_TAPIN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dpnet.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_DPNET=\
	".\Stdafx.h"\
	".\Dpnet.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\Rw\Tpslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\Rw\Tpslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	

"$(INTDIR)\dpnet.obj" : $(SOURCE) $(DEP_CPP_DPNET) "$(INTDIR)"

"$(INTDIR)\dpnet.sbr" : $(SOURCE) $(DEP_CPP_DPNET) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_DPNET=\
	".\Stdafx.h"\
	".\Dpnet.h"\
	".\nb30.h"\
	{$(INCLUDE)}"\Rw\Tpslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\Rw\Tpslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	".\generic.h"\
	

"$(INTDIR)\dpnet.obj" : $(SOURCE) $(DEP_CPP_DPNET) "$(INTDIR)"

"$(INTDIR)\dpnet.sbr" : $(SOURCE) $(DEP_CPP_DPNET) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DPENGINE.CPP

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_DPENG=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Vpnet.h"\
	".\Wnotque.h"\
	".\Vpint.h"\
	".\Dpnet.h"\
	".\Dpengine.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\Rw\Tpslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\Rw\Tpslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	

"$(INTDIR)\DPENGINE.OBJ" : $(SOURCE) $(DEP_CPP_DPENG) "$(INTDIR)"

"$(INTDIR)\DPENGINE.SBR" : $(SOURCE) $(DEP_CPP_DPENG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_DPENG=\
	".\Stdafx.h"\
	".\Vpparam.h"\
	".\Vpengine.h"\
	".\Vpnet.h"\
	".\Wnotque.h"\
	".\Vpint.h"\
	".\Dpnet.h"\
	".\Dpengine.h"\
	{$(INCLUDE)}"\Rw\Tvslist.h"\
	".\nb30.h"\
	{$(INCLUDE)}"\Rw\Tpslist.h"\
	{$(INCLUDE)}"\Rw\Tislist.h"\
	{$(INCLUDE)}"\Rw\Tpslist.cc"\
	{$(INCLUDE)}"\rw\islist.h"\
	{$(INCLUDE)}"\rw\tislist.cc"\
	{$(INCLUDE)}"\rw\tooldefs.h"\
	{$(INCLUDE)}"\rw\mempool.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\backward.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	".\generic.h"\
	{$(INCLUDE)}"\rw\tvslist.cc"\
	

"$(INTDIR)\DPENGINE.OBJ" : $(SOURCE) $(DEP_CPP_DPENG) "$(INTDIR)"

"$(INTDIR)\DPENGINE.SBR" : $(SOURCE) $(DEP_CPP_DPENG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vputil.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_VPUTI=\
	".\Stdafx.h"\
	".\Vputil.h"\
	".\Vdmplay.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\vputil.obj" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"

"$(INTDIR)\vputil.sbr" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_VPUTI=\
	".\Stdafx.h"\
	".\Vputil.h"\
	".\Vdmplay.h"\
	".\nb30.h"\
	

"$(INTDIR)\vputil.obj" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"

"$(INTDIR)\vputil.sbr" : $(SOURCE) $(DEP_CPP_VPUTI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\frcoder.cpp

!IF  "$(CFG)" == "Vdmplay - Win32 Release"

DEP_CPP_FRCOD=\
	".\Stdafx.h"\
	".\Commport.h"\
	".\Frcoder.h"\
	".\nb30.h"\
	".\generic.h"\
	".\Vdmplay.h"\
	".\Vputil.h"\
	".\Vplist.h"\
	".\Tdlog.h"\
	".\Vpnet.h"\
	{$(INCLUDE)}"\rw\generic.h"\
	{$(INCLUDE)}"\rw\defs.h"\
	{$(INCLUDE)}"\rw\compiler.h"\
	{$(INCLUDE)}"\rw\rwwind.h"\
	

"$(INTDIR)\frcoder.obj" : $(SOURCE) $(DEP_CPP_FRCOD) "$(INTDIR)"

"$(INTDIR)\frcoder.sbr" : $(SOURCE) $(DEP_CPP_FRCOD) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Vdmplay - Win32 Debug"

DEP_CPP_FRCOD=\
	".\Stdafx.h"\
	".\Commport.h"\
	".\Frcoder.h"\
	".\nb30.h"\
	

"$(INTDIR)\frcoder.obj" : $(SOURCE) $(DEP_CPP_FRCOD) "$(INTDIR)"

"$(INTDIR)\frcoder.sbr" : $(SOURCE) $(DEP_CPP_FRCOD) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
