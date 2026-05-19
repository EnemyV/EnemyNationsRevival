//---------------------------------------------------------------------------
// WinMain.cpp — Phase 4c entry point replacing MFC's AfxWinMain.
//
// MFC's CMAKE_MFC_FLAG=2 normally pulls in an auto-generated WinMain via
// AfxWinMain in mfcsXX.lib. AfxWinMain calls AfxGetApp() which assumes a
// CWinApp-derived static instance; our gate-on CConquerApp inherits
// CWinAppStub (no MFC), so AfxGetApp returns garbage and crashes.
//
// Defining our own WinMain here makes the linker prefer this entry over
// the MFC one. We then directly drive the CConquerApp lifecycle:
//   theApp.m_hInstance = hInstance
//   theApp.InitInstance()
//   theApp.Run()
//   theApp.ExitInstance()
//
// Only compiles under ENATIONS_USE_STUB_APP. Under gate-off, the file is
// effectively empty and MFC's auto-WinMain wins.
//---------------------------------------------------------------------------

#include "stdafx.h"

#ifdef ENATIONS_USE_STUB_APP

#include "lastplnt.h"

extern CConquerApp theApp;

extern "C" int APIENTRY WinMain( HINSTANCE hInstance,
                                 HINSTANCE hPrevInstance,
                                 LPSTR     lpCmdLine,
                                 int       nCmdShow )
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Bootstrap CWinAppStub members the app expects.
    theApp.m_hInstance = hInstance;

    // App lifecycle.
    int exitCode = 0;
    if ( theApp.InitInstance() )
    {
        exitCode = theApp.Run();
    }
    theApp.ExitInstance();

    return exitCode;
}

#endif  // ENATIONS_USE_STUB_APP
