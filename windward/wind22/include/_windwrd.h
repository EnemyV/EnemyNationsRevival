#ifndef ___WINDWRD_H__
#define ___WINDWRD_H__


//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


// this is always defined in windward so it's there for the apps
// #define _LOGOUT

#include <windward.h>

// needed by some — type is gate-conditional. The single user (w22_settings.cpp
// GetMainHWND fallback) only reads m_pMainWnd, so both CWinApp and CWinAppStub
// satisfy the contract since both expose `CWnd* m_pMainWnd`.
#ifdef ENATIONS_USE_STUB_APP
class CWinAppStub;
extern CWinAppStub * ptheApp;
#else
extern CWinApp * ptheApp;
#endif



#endif
