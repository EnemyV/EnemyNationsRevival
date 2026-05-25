//---------------------------------------------------------------------------
//
// winappstub.h — Non-MFC replacement for MFC's CWinApp.
//
// CConquerApp inherits from CWinAppStub (Phase 4c, landed 2026-05-18).
// CWinAppStub provides the exact surface CConquerApp uses from CWinApp:
//
//   Members exposed:
//     CWnd*      m_pMainWnd     (CWndStub*; nullptr default)
//     HINSTANCE  m_hInstance    (set from WinMain, defaults to GetModuleHandle(NULL))
//     LPTSTR     m_pszAppName   (owned C-string)
//     MSG        m_msgCur       (current message in pump)
//     HACCEL     m_hAccel       (already owned by CConquerApp itself)
//
//   Methods exposed:
//     virtual BOOL InitInstance()
//     virtual int  ExitInstance()
//     virtual int  Run()
//     virtual BOOL PreTranslateMessage(MSG* pMsg)
//     BOOL SetRegistryKey(LPCTSTR pszCompany)        — no-op (EnSettings handles registry path)
//
// MFC's auto-generated WinMain is replaced by our own (WinMain.cpp) that
// calls theApp.InitInstance() / Run() / ExitInstance(). CMAKE_MFC_FLAG is
// commented out in CMakeLists.txt to suppress the auto-WinMain.
//
//---------------------------------------------------------------------------

#ifndef __WINAPPSTUB_H__
#define __WINAPPSTUB_H__

#include <windows.h>

// Forward-declare CWnd so the m_pMainWnd type works without forcing afx*.h
// into translation units that include winappstub.h. Translation units that
// actually USE m_pMainWnd will need afxwin.h via stdafx.h (or our CWndStub
// when MFC is gone entirely).
class CWnd;


class CWinAppStub
{
public:
    CWinAppStub();
    virtual ~CWinAppStub();

    // ----- Members exposed publicly (CConquerApp uses these by name) -----
    // CWinApp members:
    CWnd*      m_pMainWnd;    // current main window pointer (legacy MFC name)
    HINSTANCE  m_hInstance;   // module handle; set from WinMain entry
    LPSTR      m_pszAppName;  // owned heap-allocated C-string (free in dtor)
    MSG        m_msgCur;      // current message being pumped
    // CWinThread members CConquerApp also references:
    DWORD      m_nThreadID;   // thread id of the main UI thread

    // ----- App lifecycle (virtuals — CConquerApp overrides) -----
    virtual BOOL InitInstance()                          { return TRUE; }
    virtual int  ExitInstance()                          { return 0; }
    virtual int  Run();           // default: standard message pump
    virtual BOOL PreTranslateMessage( MSG* pMsg )        { return FALSE; }
    virtual BOOL IsIdleMessage( MSG* pMsg )              { return TRUE; }
    virtual BOOL OnIdle( LONG lCount )                   { return FALSE; }
    virtual void AssertValid() const                     {}

    // ----- CWinApp compat shims -----
    // SetRegistryKey is a no-op — EnSettings already handles the registry
    // namespace. Both string and int-resource overloads supported.
    void SetRegistryKey( LPCSTR pszRegKey )              { (void)pszRegKey; }
    void SetRegistryKey( UINT nIDRegKey )                { (void)nIDRegKey; }

    // WinHelp — game uses this for Help button callbacks. Forwards to Win32
    // ::WinHelpA against the main window handle if known. CConquerApp's
    // call sites (SDL2Options, SDL2FileDialog, lastplnt, main) pass HELP_*
    // commands; for the SDL2 migration we just no-op since there's no .hlp
    // file shipped with the modern build.
    virtual void WinHelp( DWORD_PTR dwData, UINT nCmd = 0x0001 /*HELP_CONTEXT*/ )
    {
        (void)dwData; (void)nCmd;
        // No-op: legacy WinHelp .hlp files aren't shipped with the SDL2 port.
    }

    // ----- CWinThread compat shims -----
    // The game tweaks main-thread priority in a few spots. Forward to the
    // Win32 thread handle of the main UI thread.
    int  GetThreadPriority()                             { return ::GetThreadPriority( ::GetCurrentThread() ); }
    BOOL SetThreadPriority( int nPriority )              { return ::SetThreadPriority( ::GetCurrentThread(), nPriority ); }

    // CWinApp::LoadStdString / LoadIcon / LoadCursor / LoadStandardCursor —
    // CConquerApp already has its own shadow versions of these (added in
    // Phase 4c prep, commits b97bc0d and 47910c1). No need to forward here.
};

#endif // __WINAPPSTUB_H__
