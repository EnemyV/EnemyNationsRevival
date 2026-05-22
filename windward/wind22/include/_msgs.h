#ifndef ___MSGS_H__
#define ___MSGS_H__


//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------

// Phase 1g step 2: WM_USER comes from windows.h under the stub gates;
// afxwin.h was only included here for that single macro.
#if defined(ENATIONS_USE_STUB_WND) && defined(ENATIONS_USE_STUB_APP)
#include <windows.h>
#else
#include <afxwin.h>
#endif

enum Msg {
 WM_FIRST_LIB = WM_USER + 0x1002,
 WM_BUTTONMOUSEMOVE,
 WM_ICONMOUSEMOVE,
 WM_MUSIC_EOB,

 WM_FIRST_APP
 };

#endif
