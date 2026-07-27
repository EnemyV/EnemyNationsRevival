//---------------------------------------------------------------------------
// vdmplay_stubs.cpp — Linux-only stubs for the vdmplay (VP) networking/session
// API. vdmplay.dll is not built on Linux (single-player bring-up; multiplayer
// is out of scope), but the game links against the vp* entry points. These
// stubs return failure/empty so the binary links and single-player runs.
// Excluded from the MSVC build (which links the real vdmplay).
//---------------------------------------------------------------------------

#ifdef _WIN32
#error "vdmplay_stubs.cpp is Linux-only"
#endif

#include "stdafx.h"
#include "vdmplay.h"

extern "C" {

// InitInstance requires GetVersion() >= 0x01000021 (v1.0.0.33). Report a
// passing version so single-player init proceeds (multiplayer is stubbed off).
DWORD VPAPI vpGetVersion( ) { return 0x01000021; }
DWORD VPAPI vpSupportedTransports( ) { return 0; }

VPHANDLE VPAPI vpStartup( DWORD, LPCVPGUID, DWORD, DWORD, UINT, LPCVOID ) { return NULL; }
BOOL     VPAPI vpCleanup( VPHANDLE ) { return TRUE; }

BOOL VPAPI vpEnumSessions( VPHANDLE, HWND, BOOL, LPCVOID ) { return FALSE; }
BOOL VPAPI vpStopEnumSessions( VPHANDLE ) { return FALSE; }

VPSESSIONHANDLE VPAPI vpCreateSession( VPHANDLE, HWND, LPCSTR, DWORD, LPCVOID ) { return NULL; }
VPSESSIONHANDLE VPAPI vpJoinSession( VPHANDLE, HWND, LPCVPSESSIONID, LPCSTR, DWORD, LPCVOID ) { return NULL; }
BOOL VPAPI vpSetSessionVisibility( VPSESSIONHANDLE, BOOL ) { return FALSE; }
BOOL VPAPI vpUpdateSessionData( VPSESSIONHANDLE, LPCVOID ) { return FALSE; }
BOOL VPAPI vpCloseSession( VPSESSIONHANDLE, LPCVOID ) { return TRUE; }

BOOL VPAPI vpAddPlayer( VPSESSIONHANDLE, LPCSTR, DWORD, LPCVOID, LPVPPLAYERID ) { return FALSE; }
BOOL VPAPI vpKillPlayer( VPSESSIONHANDLE, VPPLAYERID ) { return FALSE; }

BOOL VPAPI vpSendData( VPSESSIONHANDLE, VPPLAYERID, VPPLAYERID, LPCVOID, DWORD, DWORD, LPCVOID ) { return FALSE; }
BOOL VPAPI vpAcknowledge( VPHANDLE, LPCVPMESSAGE ) { return FALSE; }

DWORD VPAPI vpGetAddressString( VPHANDLE, LPCVPNETADDRESS, LPSTR buf, DWORD bufSize ) { if (buf && bufSize) buf[0] = 0; return 0; }
BOOL  VPAPI vpGetAddress( VPHANDLE, LPVPNETADDRESS ) { return FALSE; }
BOOL  VPAPI vpGetServerAddress( VPHANDLE, LPVPNETADDRESS ) { return FALSE; }

} // extern "C"
