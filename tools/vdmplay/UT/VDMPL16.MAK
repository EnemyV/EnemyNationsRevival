# Microsoft Visual C++ generated build script - Do not modify

PROJ = VDMPL16
DEBUG = 1
PROGTYPE = 1
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\PROJECTS\VDMPLAY\UT\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = VPUT16.CPP  
RC = rc
CFLAGS_D_WDLL = /nologo /Gs /G2 /Zp1 /W3 /Zi /ALw /Od /D "_DEBUG" /D "RWDEBUG" /D "NOMFC" /I "." /I ".." /FR /GD /GEe /Fd"VDMPL16.PDB"
CFLAGS_R_WDLL = /nologo /Zp1 /W3 /ALw /O1 /D "NDEBUG" /D "NOMFC" /I "." /I ".." /FR /GD 
LFLAGS_D_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /CO /MAP:FULL
LFLAGS_R_WDLL = /NOLOGO /NOD /NOE /PACKC:61440 /ALIGN:16 /ONERROR:NOEXE /MAP:FULL
LIBS_D_WDLL = mtllwg oldnames libw ldllcew commdlg.lib mmsystem.lib olecli.lib olesvr.lib shell.lib 
LIBS_R_WDLL = mtllw oldnames libw ldllcew commdlg.lib mmsystem.lib olecli.lib olesvr.lib shell.lib 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = VDMPL16.DEF
OBJS_EXT = ..\NBPOST.OBJ ..\ECBPOST.OBJ 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WDLL)
LFLAGS = $(LFLAGS_D_WDLL)
LIBS = $(LIBS_D_WDLL)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WDLL)
LFLAGS = $(LFLAGS_R_WDLL)
LIBS = $(LIBS_R_WDLL)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = VPUT16.SBR \
		VDMPLAY.SBR \
		VPENGINE.SBR \
		SMAP.SBR \
		TCPNET.SBR \
		NBNET.SBR \
		IPX16NET.SBR \
		VPWINSK.SBR \
		IPX16STB.SBR \
		COMMNET.SBR \
		DATALINK.SBR \
		W16COMM.SBR \
		MDMDRVR.SBR \
		MDMNET.SBR \
		DIALER.SBR \
		DATALOG.SBR \
		VPNET.SBR \
		COMSTATD.SBR \
		ADVANCED.SBR \
		VPUTIL.SBR \
		FRCODER.SBR


NBPOST_DEP = 

ECBPOST_DEP = 

VPUT16_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\ut\w32sut.h \
	c:\projects\vdmplay\ut\vdmplut.h


VDMPLAY_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\vpparam.h \
	c:\projects\vdmplay\vpengine.h \
	c:\projects\vdmplay\wnotque.h \
	c:\projects\vdmplay\vpwinsk.h \
	c:\msvc\include\winsock.h \
	c:\projects\vdmplay\datalog.h \
	c:\projects\vdmplay\base.h \
	c:\projects\vdmplay\w32sut.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\tcpnet.h \
	c:\projects\vdmplay\wsipxnet.h \
	c:\sdkcd6\client\include\nwipxspx.h \
	c:\sdkcd6\client\include\nxtw.h \
	c:\sdkcd6\client\include\nwsap.h \
	c:\sdkcd6\client\include\nwdiag.h \
	c:\sdkcd6\client\include\nxtd.h \
	c:\sdkcd6\client\include\nwcaldef.h \
	c:\sdkcd6\client\include\sap.h \
	c:\sdkcd6\client\include\diag.h \
	c:\projects\vdmplay\ipx16net.h \
	c:\projects\vdmplay\nbnet.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\commnet.h \
	c:\projects\vdmplay\mdmnet.h \
	c:\projects\vdmplay\w32comm.h \
	c:\projects\vdmplay\w16comm.h \
	c:\projects\vdmplay\smap.h \
	c:\projects\vdmplay\version.h \
	c:\projects\vdmplay\vpint.h


VPENGINE_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\vpparam.h \
	c:\projects\vdmplay\vpengine.h \
	c:\projects\vdmplay\smap.h


SMAP_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\vpparam.h \
	c:\projects\vdmplay\vpengine.h \
	c:\projects\vdmplay\smap.h


TCPNET_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\vpwinsk.h \
	c:\msvc\include\winsock.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\tcpnet.h


NBNET_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\nb30.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\nbnet.h


IPX16NET_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\sdkcd6\client\include\nwipxspx.h \
	c:\sdkcd6\client\include\nxtw.h \
	c:\sdkcd6\client\include\nwsap.h \
	c:\sdkcd6\client\include\nwdiag.h \
	c:\sdkcd6\client\include\nxtd.h \
	c:\sdkcd6\client\include\nwcaldef.h \
	c:\sdkcd6\client\include\sap.h \
	c:\sdkcd6\client\include\diag.h \
	c:\projects\vdmplay\ipx16stb.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\ipx16net.h


VPWINSK_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\vpwinsk.h \
	c:\msvc\include\winsock.h


IPX16STB_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\sdkcd6\client\include\nwipxspx.h \
	c:\sdkcd6\client\include\nxtw.h \
	c:\sdkcd6\client\include\nwsap.h \
	c:\sdkcd6\client\include\nwdiag.h \
	c:\sdkcd6\client\include\nxtd.h \
	c:\sdkcd6\client\include\nwcaldef.h \
	c:\sdkcd6\client\include\sap.h \
	c:\sdkcd6\client\include\diag.h \
	c:\projects\vdmplay\ipx16stb.h


COMMNET_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\datalink.h \
	c:\projects\vdmplay\datalog.h \
	c:\projects\vdmplay\commnet.h


VER16_RCDEP = c:\projects\vdmplay\ut\icon1.ico \
	c:\projects\vdmplay\ut\ver32.rc


DATALINK_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\datalog.h \
	c:\projects\vdmplay\datalink.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\frcoder.h


W16COMM_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\w16comm.h


MDMDRVR_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\astring.h \
	c:\util\rogue.61\rw/cstring.h \
	c:\util\rogue.61\rw/tooldefs.h \
	c:\util\rogue.61\rw/defs.h \
	c:\util\rogue.61\rw/compiler.h \
	c:\util\rogue.61\rw/rwwind.h \
	c:\util\rogue.61\rw/generic.h \
	c:\util\rogue.61\rw/backward.h \
	c:\util\rogue.61\rw/ref.h \
	c:\projects\vdmplay\astrlist.h \
	c:\util\rogue.61\rw/tvslist.h \
	c:\util\rogue.61\rw/tislist.h \
	c:\util\rogue.61\rw/islist.h \
	c:\util\rogue.61\rw/mempool.h \
	c:\util\rogue.61\rw/tislist.cc \
	c:\util\rogue.61\rw/tvslist.cc \
	c:\util\rogue.61\rw/gslist.h \
	c:\util\rogue.61\rw/slist.h \
	c:\projects\vdmplay\mdmdrvr.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\datalog.h


MDMNET_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\datagram.h \
	c:\projects\vdmplay\datalink.h \
	c:\projects\vdmplay\commnet.h \
	c:\projects\vdmplay\mdmnet.h \
	c:\projects\vdmplay\datalog.h \
	c:\projects\vdmplay\astring.h \
	c:\util\rogue.61\rw/cstring.h \
	c:\util\rogue.61\rw/tooldefs.h \
	c:\util\rogue.61\rw/defs.h \
	c:\util\rogue.61\rw/compiler.h \
	c:\util\rogue.61\rw/rwwind.h \
	c:\util\rogue.61\rw/generic.h \
	c:\util\rogue.61\rw/backward.h \
	c:\util\rogue.61\rw/ref.h \
	c:\projects\vdmplay\astrlist.h \
	c:\util\rogue.61\rw/tvslist.h \
	c:\util\rogue.61\rw/tislist.h \
	c:\util\rogue.61\rw/islist.h \
	c:\util\rogue.61\rw/mempool.h \
	c:\util\rogue.61\rw/tislist.cc \
	c:\util\rogue.61\rw/tvslist.cc \
	c:\util\rogue.61\rw/gslist.h \
	c:\util\rogue.61\rw/slist.h \
	c:\projects\vdmplay\mdmdrvr.h \
	c:\projects\vdmplay\dialer.h


DIALER_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\astring.h \
	c:\util\rogue.61\rw/cstring.h \
	c:\util\rogue.61\rw/tooldefs.h \
	c:\util\rogue.61\rw/defs.h \
	c:\util\rogue.61\rw/compiler.h \
	c:\util\rogue.61\rw/rwwind.h \
	c:\util\rogue.61\rw/generic.h \
	c:\util\rogue.61\rw/backward.h \
	c:\util\rogue.61\rw/ref.h \
	c:\projects\vdmplay\astrlist.h \
	c:\util\rogue.61\rw/tvslist.h \
	c:\util\rogue.61\rw/tislist.h \
	c:\util\rogue.61\rw/islist.h \
	c:\util\rogue.61\rw/mempool.h \
	c:\util\rogue.61\rw/tislist.cc \
	c:\util\rogue.61\rw/tvslist.cc \
	c:\util\rogue.61\rw/gslist.h \
	c:\util\rogue.61\rw/slist.h \
	c:\projects\vdmplay\mdmdrvr.h \
	c:\projects\vdmplay\dialer.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\datalog.h


DATALOG_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\help.h \
	c:\projects\vdmplay\datalog.h \
	c:\projects\vdmplay\comstatd.h


VPNET_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\datalog.h


COMSTATD_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\datalog.h \
	c:\projects\vdmplay\comstatd.h \
	c:\projects\vdmplay\help.h


ADVANCED_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\base.h \
	c:\projects\vdmplay\help.h \
	c:\projects\vdmplay\version.h \
	c:\projects\vdmplay\nb30.h \
	c:\projects\vdmplay\tapiutil.h


VPUTIL_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h


FRCODER_DEP = c:\projects\vdmplay\stdafx.h \
	c:\projects\vdmplay\vdmplay.h \
	c:\projects\vdmplay\vputil.h \
	c:\projects\vdmplay\vplist.h \
	c:\projects\vdmplay\tdlog.h \
	c:\projects\vdmplay\vpnet.h \
	c:\projects\vdmplay\commport.h \
	c:\projects\vdmplay\frcoder.h


all:	$(PROJ).DLL $(PROJ).BSC

VPUT16.OBJ:	VPUT16.CPP $(VPUT16_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c VPUT16.CPP

VDMPLAY.OBJ:	..\VDMPLAY.CPP $(VDMPLAY_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\VDMPLAY.CPP

VPENGINE.OBJ:	..\VPENGINE.CPP $(VPENGINE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\VPENGINE.CPP

SMAP.OBJ:	..\SMAP.CPP $(SMAP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\SMAP.CPP

TCPNET.OBJ:	..\TCPNET.CPP $(TCPNET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\TCPNET.CPP

NBNET.OBJ:	..\NBNET.CPP $(NBNET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\NBNET.CPP

IPX16NET.OBJ:	..\IPX16NET.CPP $(IPX16NET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\IPX16NET.CPP

VPWINSK.OBJ:	..\VPWINSK.CPP $(VPWINSK_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\VPWINSK.CPP

IPX16STB.OBJ:	..\IPX16STB.CPP $(IPX16STB_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\IPX16STB.CPP

COMMNET.OBJ:	..\COMMNET.CPP $(COMMNET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\COMMNET.CPP

VER16.RES:	VER16.RC $(VER16_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r VER16.RC

DATALINK.OBJ:	..\DATALINK.CPP $(DATALINK_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\DATALINK.CPP

W16COMM.OBJ:	..\W16COMM.CPP $(W16COMM_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\W16COMM.CPP

MDMDRVR.OBJ:	..\MDMDRVR.CPP $(MDMDRVR_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\MDMDRVR.CPP

MDMNET.OBJ:	..\MDMNET.CPP $(MDMNET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\MDMNET.CPP

DIALER.OBJ:	..\DIALER.CPP $(DIALER_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\DIALER.CPP

DATALOG.OBJ:	..\DATALOG.CPP $(DATALOG_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\DATALOG.CPP

VPNET.OBJ:	..\VPNET.CPP $(VPNET_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\VPNET.CPP

COMSTATD.OBJ:	..\COMSTATD.CPP $(COMSTATD_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\COMSTATD.CPP

ADVANCED.OBJ:	..\ADVANCED.CPP $(ADVANCED_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\ADVANCED.CPP

VPUTIL.OBJ:	..\VPUTIL.CPP $(VPUTIL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\VPUTIL.CPP

FRCODER.OBJ:	..\FRCODER.CPP $(FRCODER_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\FRCODER.CPP


$(PROJ).DLL::	VER16.RES

$(PROJ).DLL::	VPUT16.OBJ VDMPLAY.OBJ VPENGINE.OBJ SMAP.OBJ TCPNET.OBJ NBNET.OBJ IPX16NET.OBJ \
	VPWINSK.OBJ IPX16STB.OBJ COMMNET.OBJ DATALINK.OBJ W16COMM.OBJ MDMDRVR.OBJ MDMNET.OBJ \
	DIALER.OBJ DATALOG.OBJ VPNET.OBJ COMSTATD.OBJ ADVANCED.OBJ VPUTIL.OBJ FRCODER.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
VPUT16.OBJ +
VDMPLAY.OBJ +
VPENGINE.OBJ +
SMAP.OBJ +
TCPNET.OBJ +
NBNET.OBJ +
IPX16NET.OBJ +
VPWINSK.OBJ +
IPX16STB.OBJ +
COMMNET.OBJ +
DATALINK.OBJ +
W16COMM.OBJ +
MDMDRVR.OBJ +
MDMNET.OBJ +
DIALER.OBJ +
DATALOG.OBJ +
VPNET.OBJ +
COMSTATD.OBJ +
ADVANCED.OBJ +
VPUTIL.OBJ +
FRCODER.OBJ +
$(OBJS_EXT)
$(PROJ).DLL
$(MAPFILE)
c:\msvc\lib\+
c:\msvc\mfc\lib\+
c:\util\rogue.61\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) VER16.RES $@
	@copy $(PROJ).CRF MSVC.BND
	implib /nowep $(PROJ).LIB $(PROJ).DLL

$(PROJ).DLL::	VER16.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) VER16.RES $@

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
