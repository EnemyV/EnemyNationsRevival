/**************************************************************************

    HALFTONE.C - A halftoning demo for WinG

 **************************************************************************/
/**************************************************************************

    (C) Copyright 1994 Microsoft Corp.  All rights reserved.

    You have a royalty-free right to use, modify, reproduce and 
    distribute the Sample Files (and/or any modified version) in 
    any way you find useful, provided that you agree that 
    Microsoft has no warranty obligations or liability for any 
    Sample Application Files which are modified. 

 **************************************************************************/


#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include <wing.h>

#include "halftone.h"
#include "..\utils\utils.h"

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
static  char       szAppName[]="WinG Halftoning Sample";
static  char       szAppFilter[]="Bitmaps\0*.bmp;*.dib\0";
static  HINSTANCE  hInstApp;
static  HWND       hwndApp;
static  HPALETTE   hpalApp;
static  BOOL       fAppActive;
static  BOOL       fCustomDither = TRUE;
static  PDIB       pdibOriginal = 0;
static  PDIB       pdibHalftone = 0;

#ifdef WIN32
    #define _export
#endif

/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

LONG FAR PASCAL _export  AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LONG  AppCommand (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

void  AppExit(void);
BOOL  AppIdle(void);
void  AppOpenFile(HWND hwnd, LPSTR szFileName);

PDIB  DibHalftoneDIB( PDIB pSource );

/*----------------------------------------------------------------------------*\
|   AppAbout( hDlg, uiMessage, wParam, lParam )                                |
|                                                                              |
|   Description:                                                               |
|       This function handles messages belonging to the "About" dialog box.    |
|       The only message that it looks for is WM_COMMAND, indicating the use   |
|       has pressed the "OK" button.  When this happens, it takes down         |
|       the dialog box.                                                        |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of about dialog window                   |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if message has been processed, else FALSE                         |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL FAR PASCAL _export AppAbout(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
          if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hwnd,TRUE);
            }
            break;

        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   AppInit( hInst, hPrev)                                                     |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into               |
|       memory.  It performs all initialization that doesn't need to be done   |
|       once per instance.                                                     |
|                                                                              |
|   Arguments:                                                                 |
|       hInstance       instance handle of current instance                    |
|       hPrev           instance handle of previous instance                   |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HINSTANCE hInst,HINSTANCE hPrev,int sw,LPSTR szCmdLine)
{
    WNDCLASS cls;
    int      dx,dy;
    HMENU hMenu;

    /* Save instance handle for DialogBoxes */
    hInstApp = hInst;

// Clear the System Palette so that WinG blting runs at full speed.
    ClearSystemPalette();

    if (!hPrev)
    {
        /*
         *  Register a class for the main application window
         */
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = LoadIcon(hInst,"AppIcon");
        cls.lpszMenuName   = "AppMenu";
        cls.lpszClassName  = szAppName;
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = (WNDPROC)AppWndProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        if (!RegisterClass(&cls))
            return FALSE;
    }

    dx = GetSystemMetrics (SM_CXSCREEN) / 2;
    dy = GetSystemMetrics (SM_CYSCREEN) / 2;

  hpalApp = WinGCreateHalftonePalette();

    hwndApp = CreateWindow (szAppName,              // Class name
                            szAppName,              // Caption
                            WS_OVERLAPPEDWINDOW,    // Style bits
                            CW_USEDEFAULT, 0,       // Position
                            dx,dy,                  // Size
                            (HWND)NULL,             // Parent window (no parent)
                            (HMENU)NULL,            // use class menu
                            hInst,                  // handle to window instance
                            (LPSTR)NULL             // no params to pass on
                           );
    ShowWindow(hwndApp,sw);

  //*** Check the default dither selection
  hMenu = GetMenu(hwndApp);
  CheckMenuItem(hMenu, MENU_WING, fCustomDither ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hMenu, MENU_GDI, fCustomDither ? MF_UNCHECKED : MF_CHECKED);

    if (*szCmdLine)
  AppOpenFile(hwndApp, szCmdLine);

    return TRUE;
}


/*----------------------------------------------------------------------------*\
|   AppExit()                                                                  |
|                                                                              |
|   Description:                                                               |
|     App is just about to exit, cleanup.                                      |
|                                                                              |
\*----------------------------------------------------------------------------*/
void AppExit()
{
  if (hpalApp)
    DeleteObject(hpalApp);
  if (pdibOriginal)
    DibFree(pdibOriginal);
  if (pdibHalftone)
    DibFree(pdibHalftone);
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )                              |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hInst           instance handle of this instance of the app            |
|       hPrev           instance handle of previous instance, NULL if first    |
|       szCmdLine       ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    MSG     msg;

    /* Call initialization procedure */
    if (!AppInit(hInst,hPrev,sw,szCmdLine))
      return FALSE;

    /*
     * Polling messages from event queue
     */
    for (;;)
    {
        if (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
  {
      if (AppIdle())
            WaitMessage();
        }
    }

    AppExit();
    return msg.wParam;
}

/*----------------------------------------------------------------------------*\
|   AppIdle()                                                                  |
|                                                                              |
|   Description:                                                               |
|     Place to do idle time stuff.                                             |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL AppIdle()
{
    if (fAppActive)
    {
  //
  // we are the foreground app.
  //
  return TRUE;      // nothing to do.
    }
    else
    {
  //
  // we are a background app.
  //
  return TRUE;      // nothing to do.
    }
}

/*----------------------------------------------------------------------------*\
|   AppOpenFile()                                                              |
|                                                                              |
|   Description:                                                               |
|     Open a file.                                                             |
|                                                                              |
\*----------------------------------------------------------------------------*/
void AppOpenFile(HWND hwnd, LPSTR szFileName)
{
  HCURSOR hCur = LoadCursor(NULL, IDC_WAIT);
  PDIB pdibNew;

  hCur = SetCursor(hCur);
  
  pdibNew = DibOpenFile(szFileName);

  if (pdibNew)
  {
    if (pdibOriginal)
      DibFree(pdibOriginal);
    if (pdibHalftone)
      DibFree(pdibHalftone);

    pdibOriginal = pdibNew;
    pdibHalftone = DibHalftoneDIB(pdibOriginal);

    if (DibBitCount(pdibOriginal) != 24)
    {
      MessageBox(hwnd, "Not a 24-bit DIB!", "Oops", MB_OK);
    }

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }

  hCur = SetCursor(hCur);
}

/*----------------------------------------------------------------------------*\
|   AppPaint(hwnd, hdc)                                                        |
|                                                                              |
|   Description:                                                               |
|       The paint function.                                                    |
|                                                                              |
|   Arguments:                                                                 |
|       hwnd             window painting into                                  |
|       hdc              display context to paint to                           |
|                                                                              |
\*----------------------------------------------------------------------------*/
AppPaint (HWND hwnd, HDC hdc)
{
  if (fCustomDither && pdibHalftone)
  {
    StretchDIBits(hdc,
      0, 0, DibWidth(pdibHalftone), DibHeight(pdibHalftone),
      0, 0, DibWidth(pdibHalftone), DibHeight(pdibHalftone),
      DibPtr(pdibHalftone), DibInfo(pdibHalftone),
      DIB_RGB_COLORS, SRCCOPY);
  }
  else if (pdibOriginal)
  {
    StretchDIBits(hdc,
      0, 0, DibWidth(pdibOriginal), DibHeight(pdibOriginal),
      0, 0, DibWidth(pdibOriginal), DibHeight(pdibOriginal),
      DibPtr(pdibOriginal), DibInfo(pdibOriginal),
      DIB_RGB_COLORS, SRCCOPY);
  }

  return TRUE;
}

/*----------------------------------------------------------------------------*\
|   AppWndProc( hwnd, uiMessage, wParam, lParam )                              |
|                                                                              |
|   Description:                                                               |
|       The window proc for the app's main (tiled) window.  This processes all |
|       of the parent window's messages.                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG FAR PASCAL _export AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    BOOL f;

    switch (msg)
    {
        case WM_CREATE:
            break;

        case WM_ACTIVATEAPP:
            fAppActive = (BOOL)wParam;
            break;

        case WM_TIMER:
            break;

        case WM_ERASEBKGND:
            break;

        case WM_INITMENU:
            break;

        case WM_COMMAND:
            return AppCommand(hwnd,msg,wParam,lParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            break;

        case WM_PALETTECHANGED:
            if ((HWND)wParam == hwnd)
                break;

      // fall through to WM_QUERYNEWPALETTE

        case WM_QUERYNEWPALETTE:
            hdc = GetDC(hwnd);

            if (hpalApp)
                SelectPalette(hdc, hpalApp, FALSE);

            f = RealizePalette(hdc);
            ReleaseDC(hwnd,hdc);

            if (f)
                InvalidateRect(hwnd,NULL,TRUE);

            return f;

        case WM_PAINT:
            hdc = BeginPaint(hwnd,&ps);

            if (hpalApp)
            {
                SelectPalette(hdc, hpalApp, FALSE);
                RealizePalette(hdc);
            }
            AppPaint (hwnd,hdc);
            EndPaint(hwnd,&ps);
            return 0L;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

/*----------------------------------------------------------------------------*\
|   AppCommand(hwnd, msg, wParam, lParam )                                     |
|                                                                              |
|   Description:                                                               |
|     Handles WM_COMMAND messages for the main window (hwndApp)                |
|   of the parent window's messages.                                           |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG AppCommand (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    char achFileName[128];
    OPENFILENAME ofn;

    switch(wParam)
    {
        case MENU_ABOUT:
            DialogBox(hInstApp,"AppAbout",hwnd,(DLGPROC)AppAbout);
            break;

        case MENU_OPEN:
            achFileName[0] = 0;

            /* prompt user for file to open */
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.hInstance = NULL;
            ofn.lpstrFilter = szAppFilter;
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter = 0;
            ofn.nFilterIndex = 0;
            ofn.lpstrFile = achFileName;
            ofn.nMaxFile = sizeof(achFileName);
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrTitle = NULL;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.nFileOffset = 0;
            ofn.nFileExtension = 0;
            ofn.lpstrDefExt = NULL;
            ofn.lCustData = 0;
            ofn.lpfnHook = NULL;
            ofn.lpTemplateName = NULL;

            if (GetOpenFileName(&ofn))
            {
                AppOpenFile(hwnd,achFileName);
            }

            break;

        case MENU_EXIT:
            PostMessage(hwnd,WM_CLOSE,0,0L);
            break;

        case MENU_WING:
        case MENU_GDI:
        {
            HMENU hMenu;

            fCustomDither = (wParam == MENU_WING);

            hMenu = GetMenu(hwnd);
            CheckMenuItem(hMenu, MENU_WING,
            fCustomDither ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hMenu, MENU_GDI,
            fCustomDither ? MF_UNCHECKED : MF_CHECKED);

            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }
        break;
  }
    return 0L;
}

/***************************************************************************
  DibHalftoneDIB

  Halftone a 24-bit DIB to the 8-bit WinG Halftone Palette
*/

//*** These tables are defined in httables.c
extern char unsigned  aDividedBy51Rounded[256];
extern char unsigned  aDividedBy51[256];
extern char unsigned  aModulo51[256];
extern char unsigned  aTimes6[6];
extern char unsigned  aTimes36[6];

extern char unsigned  aHalftone8x8[64];

extern char unsigned  aWinGHalftoneTranslation[216];

PDIB DibHalftoneDIB( PDIB pSource )
{
  //*** Create an 8-bit DIB to halftone to
  PDIB pDestination = DibCreate(8,DibWidth(pSource),DibHeight(pSource));

  //*** Only work on 24-bit sources
  if(pDestination && hpalApp && (DibBitCount(pSource) == 24))
  {
    long PixelsPerScanline = DibWidth(pSource);
    int Scanline;

    //*** Fill in the DIB color table with the halftone palette
    DibSetUsage(pDestination, hpalApp, DIB_RGB_COLORS);

    //*** Step through, converting each pixel in each scan line
    //*** to the nearest halftone match
    for(Scanline = 0;Scanline < (int)DibHeight(pSource);Scanline++)
    {
      char unsigned _huge *pSourceScanline =
          (char unsigned _huge *)DibXY(pSource,0,Scanline);
          
      char unsigned _huge *pDestinationScanline =
          (char unsigned _huge *)DibXY(pDestination,0,Scanline);

      int Pixel;

      for(Pixel = 0;Pixel < (int)DibWidth(pSource);Pixel++)
      {
        //*** This is the meat of the halftoning algorithm:
        //*** Convert an RGB into an index into the halftone palette.

        //*** First, extract the raw RGB information
        int Red = pSourceScanline[Pixel*3 + 2];
        int Green = pSourceScanline[Pixel*3 + 1];
        int Blue = pSourceScanline[Pixel*3];

        //*** Now, look up each value in the halftone matrix
        //*** using an 8x8 ordered dither.
        char unsigned RedTemp = aDividedBy51[Red]
          + (aModulo51[Red] > aHalftone8x8[(Pixel%8)*8
          + Scanline%8]);
        char unsigned GreenTemp = aDividedBy51[(char unsigned)Green]
          + (aModulo51[Green] > aHalftone8x8[
          (Pixel%8)*8 + Scanline%8]);
        char unsigned BlueTemp = aDividedBy51[(char unsigned)Blue]
          + (aModulo51[Blue] > aHalftone8x8[
          (Pixel%8)*8 +Scanline%8]);

        //*** Recombine the halftoned RGB values into a palette index
        char unsigned PaletteIndex =
          RedTemp + aTimes6[GreenTemp] + aTimes36[BlueTemp];

        //*** And translate through the WinG Halftone Palette
        //*** translation vector to give the correct value.
        pDestinationScanline[Pixel] = aWinGHalftoneTranslation[PaletteIndex];
      }
    }
  }
  else
  {
    //*** Punt on anything but 24bpp
    DibFree(pDestination);
    pDestination = 0;
  }

  return pDestination;
}
