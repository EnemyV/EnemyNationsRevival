/**************************************************************************

    TIMEWING.CPP - A timing demo for WinG

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
#include "timewing.h"

#include <string.h>
#include <mmsystem.h>
#include <fstream.h>
#include <strstrea.h>

#include <wing.h>
#include "..\utils\utils.h" 

/*----------------------------------------------------------------------------*\
|                                        |
| g l o b a l   v a r i a b l e s                        |
|                                        |
\*----------------------------------------------------------------------------*/
static  char       szAppName[]="WinG Timer Sample";
static  char       szAppFilter[]="Bitmaps\0*.bmp\0";

static  HINSTANCE  hInstApp;
HWND               hwndApp;
static  HPALETTE   hpalApp;
static  BOOL       fAppActive;

static int         dx, dy;

PDIB               pCurrentDIB;
char               aDescription[200];
char               aBuffer[5000];
UINT               CurrentDIBUsage;

struct
{
  BITMAPINFOHEADER Header;
  RGBQUAD aColors[256];
} Info;

int Iterations = 100;
int StretchMenu = 0;
float StretchFactor = 1;

/*----------------------------------------------------------------------------

Timers

*/

struct timing_result;
typedef void timer( timing_result *pResults, HWND Window );

timer TimeStretchBlt;
timer TimeStretchDIBits;
timer TimeWinGBU;
timer TimeWinGTD;


/*----------------------------------------------------------------------------

Timer structure.

*/

void PrintTimingResults( ostream &Out );

struct timing_result
{
  DWORD Time;
  timer *pTimer;
  char const *pDescription;
} aTimings[] =
{
  0, TimeStretchBlt, "StretchBlt",
  0, TimeStretchDIBits, "StretchDIBits",
  0, TimeWinGTD, "WinGStretchBlt top-down",
  0, TimeWinGBU, "WinGStretchBlt bottom-up",
};

int const NumberOfTimings = sizeof(aTimings) / sizeof(aTimings[0]);
  


  
#if defined(WIN32) || defined(_WIN32)
  #define _export
#endif

/*----------------------------------------------------------------------------*\
|                                        |
| f u n c t i o n   d e f i n i t i o n s                    |
|                                        |
\*----------------------------------------------------------------------------*/

LONG FAR PASCAL _export  AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
int   ErrMsg (LPSTR sz,...);
LONG  AppCommand (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

void  AppExit(void);
BOOL  AppIdle(void);
void  AppOpenFile(HWND hwnd, LPSTR szFileName);

/*----------------------------------------------------------------------------*\
| AppAbout( hDlg, uiMessage, wParam, lParam )                  |
|                                        |
| Description:                                 |
|   This function handles messages belonging to the "About" dialog box.    |
|   The only message that it looks for is WM_COMMAND, indicating the use   |
|   has pressed the "OK" button.  When this happens, it takes down       |
|   the dialog box.                              |
|                                        |
| Arguments:                                   |
|   hDlg      window handle of about dialog window           |
|   uiMessage   message number                       |
|   wParam      message-dependent                    |
|   lParam      message-dependent                    |
|                                        |
| Returns:                                   |
|   TRUE if message has been processed, else FALSE               |
|                                        |
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
| AppInit( hInst, hPrev)                                                       |
|                                                                              |
| Description:                                                                 |
|   This is called when the application is first loaded into                   |
|   memory.  It performs all initialization that doesn't need to be done       |
|   once per instance.                                                         |
|                                                                              |
| Arguments:                                                                   |
|   hInstance   instance handle of current instance                            |
|   hPrev       instance handle of previous instance                           |
|                                                                              |
| Returns:                                                                     |
|   TRUE if successful, FALSE if not                                           |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HINSTANCE hInst,HINSTANCE hPrev,int sw,LPSTR szCmdLine)
{
  WNDCLASS cls;

  /* Save instance handle for DialogBoxes */
  hInstApp = hInst;

// Clear the System Palette so that WinGStretchBlt runs at full speed.
  ClearSystemPalette();
  
  if (!hPrev)
  {
    /*
     *  Register a class for the main application window
     */
    cls.hCursor      = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon      = LoadIcon(hInst,"AppIcon");
    cls.lpszMenuName   = "AppMenu";
    cls.lpszClassName  = szAppName;
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance    = hInst;
    cls.style      = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    cls.lpfnWndProc    = (WNDPROC)AppWndProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    if (!RegisterClass(&cls))
      return FALSE;
  }

  dx = 400;
  dy = 400;

  hwndApp = CreateWindow (szAppName,    // Class name
              szAppName,                // Caption
              WS_OVERLAPPEDWINDOW,      // Style bits
              50, 50,                   // Position
              dx+20,dy+75,              // Size
              (HWND)NULL,               // Parent window (no parent)
              (HMENU)NULL,              // use class menu
              hInst,                    // handle to window instance
              (LPSTR)NULL               // no params to pass on
               );
  ShowWindow(hwndApp,sw);


    HMENU Menu = GetMenu(hwndApp);
    CheckMenuItem(Menu,MENU_1TO1,MF_CHECKED);

    //
    // build the timing menu.
    //

    HMENU hmenu = GetSubMenu(Menu, 3);
    DeleteMenu(hmenu, MENU_TIME, MF_BYCOMMAND);

    for (int i=0; i<NumberOfTimings; i++)
    {
        AppendMenu(hmenu, 0, MENU_TIME+i, aTimings[i].pDescription);
    }

    hpalApp = WinGCreateHalftonePalette();
    AppOpenFile(hwndApp,"frog.bmp");

    if(!pCurrentDIB)
    {
      // we couldn't load froggie

      PostMessage(hwndApp,WM_COMMAND,MENU_OPEN,0);
    }

  return TRUE;
}


/*----------------------------------------------------------------------------*\
| AppExit()                                                                    |
|                                                                              |
| Description:                                                                 |
|   App is just about to exit, cleanup.                                        |
|                                                                              |
\*----------------------------------------------------------------------------*/
void AppExit()
{
  DeleteObject(hpalApp);
}

/*----------------------------------------------------------------------------*\
| WinMain( hInst, hPrev, lpszCmdLine, cmdShow )                                |
|                                                                              |
| Description:                                                                 |
|   The main procedure for the App.  After initializing, it just goes          |
|   into a message-processing loop until it gets a WM_QUIT message             |
|   (meaning the app was closed).                                              |
|                                                                              |
| Arguments:                                                                   |
|   hInst     instance handle of this instance of the app                      |
|   hPrev     instance handle of previous instance, NULL if first              |
|   szCmdLine ->null-terminated command line                                   |
|   cmdShow   specifies how the window is initially displayed                  |
|                                                                              |
| Returns:                                                                     |
|   The exit code as specified in the WM_QUIT message.                         |
|                                                                              |
\*----------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
  MSG   msg;

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
| AppIdle()                    
|                                        
| Description:                                 
|   Place to do idle time stuff.               
|                                        
\*----------------------------------------------------------------------------*/
BOOL AppIdle()
{
  if (fAppActive)
  {
  //
  // we are the foreground app.
  //
  return TRUE;    // nothing to do.
  }
  else
  {
  //
  // we are a background app.
  //
  return TRUE;    // nothing to do.
  }
}

/*----------------------------------------------------------------------------*\
| AppOpenFile()                  
|                                        
| Description:                                 
|   Open a file.                
|                                        
\*----------------------------------------------------------------------------*/
void AppOpenFile(HWND hwnd, LPSTR szFileName)
{
  PDIB pDIB = DibOpenFile(szFileName);

  if(pDIB)
  {
    if(pCurrentDIB)
    {
      DibFree(pCurrentDIB);
    }

    pCurrentDIB = pDIB;

	 // Map the dib to the palette if it's not 4-bit

    if (DibBitCount(pDIB) != 4)
    {
      DibMapToPalette(pCurrentDIB,hpalApp);
		CurrentDIBUsage = DIB_PAL_COLORS;

      hmemcpy(&Info,pDIB,sizeof(Info));

      DibSetUsage(pCurrentDIB,hpalApp,CurrentDIBUsage);
    }
	 else
	 {
	   CurrentDIBUsage = DIB_RGB_COLORS;
	 }

    ostrstream Out(aDescription,sizeof(aDescription));

    while(*szFileName)
    {
      Out<<*szFileName++;   // can't use far * and near stream.
    }
    
    Out<<" - "<<DibWidth(pDIB)<<" x "<<DibHeight(pDIB)<<ends;

    SetWindowText(hwnd,aDescription);
  }
}

/*----------------------------------------------------------------------------*\
| AppPaint(hwnd, hdc)                              
|                                        
| Description:                                 
|   The paint function.  
|                                        
| Arguments:                                   
|   hwnd       window painting into            
|   hdc        display context to paint to               
|                                        
\*----------------------------------------------------------------------------*/
AppPaint (HWND hwnd, HDC hdc)
{
  RECT  rc;

  GetClientRect(hwnd,&rc);

  SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
  SetBkColor(hdc,GetSysColor(COLOR_WINDOW));

  return TRUE;
}

/*----------------------------------------------------------------------------*\
| AppWndProc( hwnd, uiMessage, wParam, lParam )                                |
|                                                                              |
| Description:                                                                 |
|   The window proc for the app's main (tiled) window.  This processes all     |
|   of the parent window's messages.                                           |
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

    case WM_SIZE:
    {
      static FirstTime = 1;

      if(FirstTime)
      {
        FirstTime = 0;
        break;
      }
      else
      {
        return 0;
        break;
      }
    }

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
    AppPaint (hwnd,hdc);
      EndPaint(hwnd,&ps);
      return 0L;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

/*----------------------------------------------------------------------------*\
| AppCommand(hwnd, msg, wParam, lParam )                                       |
|                                                                              |
| Description:                                                                 |
|   Handles WM_COMMAND messages for the main window (hwndApp)                  |
| of the parent window's messages.                                             |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG AppCommand (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  static char achFileName[128];
  OPENFILENAME ofn;
  ostrstream Out(aBuffer,sizeof(aBuffer));
  int PrevStretchMenu;       
  int i;

  switch(wParam)
  {
    case MENU_ABOUT:
    DialogBox(hInstApp,"AppAbout",hwnd,(DLGPROC)AppAbout);
      break;

    case MENU_TIMEALL:
    {
      HCURSOR Arror = SetCursor(0);

      for(int Counter = 0;Counter < NumberOfTimings;Counter++)
      {
        SetWindowText(hwndApp,aTimings[Counter].pDescription);

        (*aTimings[Counter].pTimer)(aTimings+Counter,hwnd);
      }

      SetCursor(Arror);
      SetWindowText(hwndApp,aDescription);

      PrintTimingResults(Out);
      MessageBox(0,aBuffer,"Timing Results",MB_OK);

      break;
    }

    case MENU_1TO1:
    case MENU_1TO2:
    case MENU_1TOALMOST2:
    {
      float aStretchFactors[3] = { 1, 2, 1.9 };
      HMENU Menu = GetMenu(hwnd);

      // uncheck current selection
      CheckMenuItem(Menu,MENU_1TO1 + StretchMenu,MF_UNCHECKED);

      PrevStretchMenu = StretchMenu;

      StretchMenu = wParam - MENU_1TO1;

      // get the stretch factor
      StretchFactor = aStretchFactors[StretchMenu];

      Iterations = (StretchFactor == 1) ? 100 : 20;

      CheckMenuItem(Menu,wParam,MF_CHECKED);

      if (StretchMenu != PrevStretchMenu)
      {
        for (i = 0; i < NumberOfTimings; i++)
          aTimings[i].Time = 0;
      }
    
      break;
    }

    default:
    {
      int Index = (int)LOWORD(wParam) - MENU_TIME;

      if(Index >= 0 && Index < NumberOfTimings)
      {
        HCURSOR Arror = SetCursor(0);

        SetWindowText(hwndApp,aTimings[Index].pDescription);
        
        (*aTimings[Index].pTimer)(aTimings+Index,hwnd);

        SetCursor(Arror);
        SetWindowText(hwndApp,aDescription);
      }

      PrintTimingResults(Out);
      MessageBox(0,aBuffer,"Timing Results",MB_OK);
      InvalidateRect(hwnd,0,TRUE);
      UpdateWindow(hwnd);

      break;
    }

  case MENU_OPEN:
  {
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

      if(GetOpenFileName(&ofn))
      {
        AppOpenFile(hwnd,achFileName);
        break;
      }
  }

      break;

    case MENU_EXIT:
      PostMessage(hwnd,WM_CLOSE,0,0L);
      break;
  }
  return 0L;
}




/*----------------------------------------------------------------------------*\
| ErrMsg - Opens a Message box with a error message in it.  The user can       |
|      select the OK button to continue                                        |
\*----------------------------------------------------------------------------*/
int ErrMsg (LPSTR sz,...)
{
  char ach[128];

  wvsprintf (ach,sz,(LPSTR)(&sz+1));   /* Format the string */
  MessageBox(hwndApp,ach,szAppName,MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
  return FALSE;
}


void PrintTimingResults( ostream &Out )
{
  int CurrentOffset = 0;

  for(int Counter = 0;Counter < NumberOfTimings;Counter++)
  {
    Out<<aTimings[Counter].pDescription
      <<":\t";

    if(aTimings[Counter].Time)
    {
      Out.setf(ios::fixed | ios::showpoint);
      Out.width(6);
      Out.precision(2);
      Out<<(Iterations / (aTimings[Counter].Time / 1000.0))<<" per Second"<<endl;
    }
    else
    {
      Out<<"Not timed yet"<<endl;
    }
    
  }

  Out<<ends;

}



/*----------------------------------------------------------------------------

StretchBlt

*/

void TimeStretchBlt( timing_result *pResult, HWND Window )
{
  int Counter;
  HDC Screen = GetDC(Window);
  DWORD Time;

  // setup here

  HDC Buffer = CreateCompatibleDC(Screen);
  HBITMAP Bitmap = CreateCompatibleBitmap(Screen,DibWidth(pCurrentDIB),
            DibHeight(pCurrentDIB));
  Bitmap = SelectBitmap(Buffer,Bitmap);

  HPALETTE OldPalette = SelectPalette(Buffer,hpalApp,FALSE);
  SelectPalette(Screen,hpalApp,FALSE);
  RealizePalette(Screen);
  RealizePalette(Buffer);
  SetStretchBltMode(Buffer,COLORONCOLOR);

  StretchDIBits(Buffer,0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),
    0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),DibPtr(pCurrentDIB),
    DibInfo(pCurrentDIB),CurrentDIBUsage,SRCCOPY);

  SetStretchBltMode(Screen,COLORONCOLOR);

  int Width = StretchFactor * DibWidth(pCurrentDIB);
  int Height = StretchFactor * DibHeight(pCurrentDIB);
  
  Time = timeGetTime();

  for(Counter = 0;Counter < Iterations;Counter++)
  {
    StretchBlt(Screen,Counter,Counter,Width,Height,
      Buffer,0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),
      SRCCOPY);
  }

  Time = timeGetTime() - Time;

  pResult->Time = Time;

  // clean up

  SelectPalette(Buffer,OldPalette,FALSE);
  SelectPalette(Screen,OldPalette,FALSE);
  DeleteObject(SelectBitmap(Buffer,Bitmap));
  DeleteDC(Buffer);

  ReleaseDC(Window,Screen);
}



/*----------------------------------------------------------------------------

StretchDIBits

*/

void TimeStretchDIBits( timing_result *pResult, HWND Window )
{
  int Counter;
  HDC Screen = GetDC(Window);
  DWORD Time;

  // setup here

  HPALETTE OldPalette = SelectPalette(Screen,hpalApp,FALSE);
  RealizePalette(Screen);

  int Width = StretchFactor * DibWidth(pCurrentDIB);
  int Height = StretchFactor * DibHeight(pCurrentDIB);
  
  Time = timeGetTime();

  for(Counter = 0;Counter < Iterations;Counter++)
  {
    StretchDIBits(Screen,Counter,Counter,Width,Height,
      0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),
      DibPtr(pCurrentDIB),DibInfo(pCurrentDIB),CurrentDIBUsage,SRCCOPY);
  }

  Time = timeGetTime() - Time;

  pResult->Time = Time;

  // clean up

  SelectPalette(Screen,OldPalette,FALSE);

  ReleaseDC(Window,Screen);
}

/*----------------------------------------------------------------------------

WinGStretchBlt - bottom-up

*/

void TimeWinGBU( timing_result *pResult, HWND Window )
{
  int Counter;
  HDC Screen = GetDC(Window);
  DWORD Time;

  void far *pBits;

  // setup here

  HDC WinGDC = WinGCreateDC();

  Info.Header.biSize = sizeof(BITMAPINFOHEADER);
  Info.Header.biWidth = DibWidth(pCurrentDIB);

  // NOTE!  You should use the WinGRecommendedDIBFormat for your WinGBitmaps.
  // This is forced to bottom-up for this example.  See the WinG.HLP file.
  
  Info.Header.biHeight = DibHeight(pCurrentDIB);

  HBITMAP WinGBitmap = WinGCreateBitmap(WinGDC,(BITMAPINFO far *)&Info,
                &pBits);

  WinGBitmap = SelectBitmap(WinGDC,WinGBitmap);

  HPALETTE OldPalette = SelectPalette(Screen,hpalApp,FALSE);
  RealizePalette(Screen);

  // We're selecting a palette into the WinGDC so we can use the
  // DIB_PAL_COLORS pCurrentDIB.  This does NOT change the WinGBitmap's
  // color table, but GDI needs to look up the DIB_PAL_COLORS somewhere!
  // Different devices can't share a palette, so we need another copy.

  HPALETTE WinGPalette = WinGCreateHalftonePalette();
  
  HPALETTE OldWinGPalette = SelectPalette(WinGDC,WinGPalette,FALSE);
  RealizePalette(WinGDC);

  SetStretchBltMode(WinGDC,COLORONCOLOR);

  StretchDIBits(WinGDC,0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),
    0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),DibPtr(pCurrentDIB),
    DibInfo(pCurrentDIB),CurrentDIBUsage,SRCCOPY);

  // we just needed the palette for the StretchDIBits
  
  SelectPalette(WinGDC,OldWinGPalette,FALSE);
  DeleteObject(WinGPalette);
  
  int Width = StretchFactor * DibWidth(pCurrentDIB);
  int Height = StretchFactor * DibHeight(pCurrentDIB);

  // WinG does a lot of work on the first blt, so do it outside the loop

  WinGStretchBlt(Screen,0,0,Width,Height,
      WinGDC,0,0,DibWidth(pCurrentDIB),
      DibHeight(pCurrentDIB));

  Time = timeGetTime();

  for(Counter = 0;Counter < Iterations;Counter++)
  {
    WinGStretchBlt(Screen,Counter,Counter,Width,Height,
      WinGDC,0,0,DibWidth(pCurrentDIB),
      DibHeight(pCurrentDIB));
  }

  Time = timeGetTime() - Time;

  pResult->Time = Time;

  // clean up

  SelectPalette(Screen,OldPalette,FALSE);
  SelectPalette(WinGDC,OldPalette,FALSE);
  DeleteObject(SelectBitmap(WinGDC,WinGBitmap));
  DeleteDC(WinGDC);

  ReleaseDC(Window,Screen);
}


/*----------------------------------------------------------------------------

WinGStretchBlt - top-down

*/

void TimeWinGTD( timing_result *pResult, HWND Window )
{
  int Counter;
  HDC Screen = GetDC(Window);
  DWORD Time;

  void far *pBits;

  // setup here

  HDC WinGDC = WinGCreateDC();

  Info.Header.biSize = sizeof(BITMAPINFOHEADER);
  Info.Header.biWidth = DibWidth(pCurrentDIB);

  // NOTE!  You should use the WinGRecommendedDIBFormat for your WinGBitmaps.
  // This is forced to top-down for this example.  See the WinG.HLP file.
  
  Info.Header.biHeight = -int(DibHeight(pCurrentDIB));

  HBITMAP WinGBitmap = WinGCreateBitmap(WinGDC,(BITMAPINFO far *)&Info,
                &pBits);

  WinGBitmap = SelectBitmap(WinGDC,WinGBitmap);

  HPALETTE OldPalette = SelectPalette(Screen,hpalApp,FALSE);
  RealizePalette(Screen);

  // We're selecting a palette into the WinGDC so we can use the
  // DIB_PAL_COLORS pCurrentDIB.  This does NOT change the WinGBitmap's
  // color table, but GDI needs to look up the DIB_PAL_COLORS somewhere!
  // Different devices can't share a palette, so we need another copy.

  HPALETTE WinGPalette = WinGCreateHalftonePalette();
  
  HPALETTE OldWinGPalette = SelectPalette(WinGDC,WinGPalette,FALSE);
  RealizePalette(WinGDC);

  SetStretchBltMode(WinGDC,COLORONCOLOR);

  StretchDIBits(WinGDC,0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),
    0,0,DibWidth(pCurrentDIB),DibHeight(pCurrentDIB),DibPtr(pCurrentDIB),
    DibInfo(pCurrentDIB),CurrentDIBUsage,SRCCOPY);

  // we just needed the palette for the StretchDIBits
  
  SelectPalette(WinGDC,OldWinGPalette,FALSE);
  DeleteObject(WinGPalette);
  
  int Width = StretchFactor * DibWidth(pCurrentDIB);
  int Height = StretchFactor * DibHeight(pCurrentDIB);

  // WinG does a lot of work on the first blt, so do it outside the loop

  WinGStretchBlt(Screen,0,0,Width,Height,
      WinGDC,0,0,DibWidth(pCurrentDIB),
      DibHeight(pCurrentDIB));

  Time = timeGetTime();

  for(Counter = 0;Counter < Iterations;Counter++)
  {
    WinGStretchBlt(Screen,Counter,Counter,Width,Height,
      WinGDC,0,0,DibWidth(pCurrentDIB),
      DibHeight(pCurrentDIB));
  }

  Time = timeGetTime() - Time;

  pResult->Time = Time;

  // clean up

  SelectPalette(Screen,OldPalette,FALSE);
  SelectPalette(WinGDC,OldPalette,FALSE);
  DeleteObject(SelectBitmap(WinGDC,WinGBitmap));
  DeleteDC(WinGDC);

  ReleaseDC(Window,Screen);
}
