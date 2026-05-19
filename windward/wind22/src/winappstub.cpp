//---------------------------------------------------------------------------
//
// winappstub.cpp — Implementation of the non-MFC CWinApp replacement.
//
// See winappstub.h for the design notes. This file is a transitional
// scaffold; the runtime path through it doesn't activate until the
// ENATIONS_USE_STUB_APP gate is flipped on (and CConquerApp's base class
// is switched from CWinApp to CWinAppStub).
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "winappstub.h"


CWinAppStub::CWinAppStub()
    : m_pMainWnd( nullptr )
    , m_hInstance( ::GetModuleHandle( nullptr ) )
    , m_pszAppName( nullptr )
    , m_nThreadID( ::GetCurrentThreadId() )
{
    ZeroMemory( &m_msgCur, sizeof( m_msgCur ) );
}

CWinAppStub::~CWinAppStub()
{
    if ( m_pszAppName != nullptr )
    {
        // MFC docs say m_pszAppName must be heap-allocated by the app
        // (it gets freed at CWinApp destruction). Match that contract.
        free( m_pszAppName );
        m_pszAppName = nullptr;
    }
}


//-------------------------------------------------------------------------
// CWinAppStub::Run — standard message pump.
//
// Default Win32 GetMessage / TranslateMessage / DispatchMessage loop.
// Derived classes (CConquerApp) override this for the game pump in
// mainloop.cpp; the default here exists so callers without overrides
// still have a working pump.
//-------------------------------------------------------------------------
int CWinAppStub::Run()
{
    while ( ::GetMessage( &m_msgCur, nullptr, 0, 0 ) > 0 )
    {
        if ( PreTranslateMessage( &m_msgCur ) )
            continue;
        ::TranslateMessage( &m_msgCur );
        ::DispatchMessage( &m_msgCur );
    }
    return (int)m_msgCur.wParam;
}
