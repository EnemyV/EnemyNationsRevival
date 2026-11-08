//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __IPCCOMM_H__
#define __IPCCOMM_H__

////////////////////////////////////////////////////////////////////////////
//
//  IPCComm.h : IPC Main Window — CWndComm STUB (Phase 2d-cont, 2026-05-11)
//
//  Original CWndComm was the MFC chat/e-mail compound window (CWndBase
//  inheritance). The SDL2 path never instantiates it (m_gameWindow is always
//  non-null in the live game), so the entire ipccomm/ipcchat/ipcread/ipcsend
//  /chatbar implementation .cpp files are excluded from the build.
//
//  This header is kept to preserve the type used by `CConquerApp::m_wndChat`
//  and the call surface from live code (`m_hWnd`, `PostMessage`, `Create`,
//  `DestroyWindow`, `KillAiChatWnd`, `IncomingMessage`, `EnableWindow`,
//  `UpdateMail`). All methods are inline no-ops; m_hWnd stays NULL forever,
//  so every `if (m_hWnd != NULL)` guard short-circuits correctly.
//
////////////////////////////////////////////////////////////////////////////

#include <windows.h>

class CMsgIPC;
class CPlayer;

class CWndComm
{
public:
    HWND m_hWnd = NULL;  // always NULL; guards in live code rely on this

    CWndComm() {}
    ~CWndComm() {}

    // External call surface from live code (netapi.cpp, newworld.cpp,
    // main.cpp, player.cpp, SDL2GameDialogs.cpp). All no-ops:
    void Create() {}
    void DestroyWindow() {}
    void EnableWindow( BOOL ) {}
    void KillAiChatWnd( CPlayer const * ) {}
    void IncomingMessage( CMsgIPC * ) {}
    LRESULT PostMessage( UINT, WPARAM, LPARAM ) { return 0; }

    // Static used in player.cpp:490
    static void UpdateMail() {}
};

#endif // __IPCCOMM_H__
