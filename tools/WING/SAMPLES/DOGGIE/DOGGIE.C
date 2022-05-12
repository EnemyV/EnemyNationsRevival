/**************************************************************************

    DOGGIE.C - a sprite demo for WinG

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
#include "doggie.h"
#include "mmsystem.h"
#include "..\utils\utils.h"
#include <wing.h>

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
static  char  szAppName[]="Doggie: WinG Sprite Demo";

static  HINSTANCE hInstApp;
static  HWND      hwndApp;
static  HPALETTE  hpalApp;
static  BOOL      fAppActive;

typedef struct header
{
  BITMAPINFOHEADER  Header;
  RGBQUAD           aColors[256];
} header;

header    BufferHeader;
long      Orientation = 1;     // assume bottom-up DIBs
void far *pBuffer = 0;
HDC       Buffer = 0;

char unsigned TransparentColor = 0xf3;

typedef struct pal
{
  WORD Version;
  WORD NumberOfEntries;
  PALETTEENTRY aEntries[256];
} pal;

pal LogicalPalette =
{
  0x300,
  256
};

HBITMAP  gbmOldMonoBitmap = 0;
PDIB     pBitmap;
int      BitmapX, BitmapY;
int      DogX, DogY;
int      dx,dy;


/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

LONG FAR PASCAL _export  AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
LONG  AppCommand (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

void  AppExit(void);
BOOL  AppIdle(void);

/*----------------------------------------------------------------------------*\
|   AppAbout( hDlg, uiMessage, wParam, lParam )                                |
|                                                                              |
|   Description:                                                               |
|       This function handles messages belonging to the "About" dialog box.    |
|       The only message that it looks for is WM_COMMAND, indicating the user  |
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
        cls.hbrBackground  = (HBRUSH) NULL;
        cls.hInstance      = hInst;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = (WNDPROC)AppWndProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        if (!RegisterClass(&cls))
            return FALSE;
    }

    dx = 400;
    dy = 400;

    pBitmap = DibOpenFile("Doggie");      
    
    if (!pBitmap)
      pBitmap = DibOpenFile("doggie2.bmp");

    BitmapX = DibWidth(pBitmap);
    BitmapY = DibHeight(pBitmap);

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
  if (Buffer)
  {
    HBITMAP hbm;

    //  Retrieve the current WinGBitmap, replace with the original
    hbm = (HBITMAP)SelectObject(Buffer, gbmOldMonoBitmap);

    //  And delete the WinGBitmap and WinGDC
    DeleteObject(hbm);
    DeleteDC(Buffer);
  }

  if(hpalApp)
  {
    DeleteObject(hpalApp);
  }
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
  WinGBitBlt(hdc,0,0,dx,dy,Buffer,0,0);

  return TRUE;
}


/*----------------------------------------------------------------------------

PaintDoggie

*/


void PaintDoggie( HDC Screen, int X, int Y )
{
  int DestinationX = X - BitmapX/2;
  int DestinationY = Y - BitmapY/2;
  int Width = BitmapX;
  int Height = BitmapY;

  TransparentDIBits((BITMAPINFO far *)&BufferHeader,pBuffer,X-BitmapX/2,
    Y-BitmapY/2,DibPtr(pBitmap),DibInfo(pBitmap),0,0,DIB_RGB_COLORS,
    TransparentColor);

  WinGBitBlt(Screen,DestinationX,DestinationY,Width,Height,
      Buffer,X-BitmapX/2,Y-BitmapY/2);

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
    static int XOffset, YOffset;
    BOOL f;
    DWORD Offset;
    int X;
    int Y;

    switch (msg)
    {
        case WM_CREATE:
      break;

        case WM_ACTIVATEAPP:
      fAppActive = (BOOL)wParam;
      break;

      case WM_SIZE:
        dx = LOWORD(lParam);
        dy = HIWORD(lParam);

      if(Buffer)
      {
        HBITMAP hbm;

        //  Create a new 8-bit WinGBitmap with the new size
        BufferHeader.Header.biWidth = LOWORD(lParam);
        BufferHeader.Header.biHeight = HIWORD(lParam) * Orientation;
        hbm = WinGCreateBitmap(Buffer,
          (BITMAPINFO far *)&BufferHeader, &pBuffer);

        //  Select it in and delete the old one
        hbm = (HBITMAP)SelectObject(Buffer, hbm);
        DeleteObject(hbm);
      }
      else
      {
        //  Create a new WinGDC and 8-bit WinGBitmap

        HBITMAP hbm;
          int Counter;
          HDC Screen;
          RGBQUAD far *pColorTable;

        //  Get WinG to recommend the fastest DIB format

        if(WinGRecommendDIBFormat((BITMAPINFO far *)&BufferHeader))
        {
          //  make sure it's 8bpp and remember the orientation

          BufferHeader.Header.biBitCount = 8;
          BufferHeader.Header.biCompression = BI_RGB;
          Orientation = BufferHeader.Header.biHeight;
        }
        else
        {
          //  set it up ourselves
  
          BufferHeader.Header.biSize = sizeof(BITMAPINFOHEADER);
          BufferHeader.Header.biPlanes = 1;
          BufferHeader.Header.biBitCount = 8;
          BufferHeader.Header.biCompression = BI_RGB;
          BufferHeader.Header.biSizeImage = 0;
          BufferHeader.Header.biClrUsed = 0;
          BufferHeader.Header.biClrImportant = 0;
        }

        BufferHeader.Header.biWidth = LOWORD(lParam);
        BufferHeader.Header.biHeight = HIWORD(lParam) * Orientation;

        //  create an identity palette from the DIB's color table

        //  get the 20 system colors as PALETTEENTRIES
    
          Screen = GetDC(0);

          GetSystemPaletteEntries(Screen,0,10,LogicalPalette.aEntries);
          GetSystemPaletteEntries(Screen,246,10,
                LogicalPalette.aEntries + 246);

        ReleaseDC(0,Screen);

        // initialize the logical palette and DIB color table

          for(Counter = 0;Counter < 10;Counter++)
          {
            // copy the system colors into the DIB header
            // WinG will do this in WinGRecommendDIBFormat,
            // but it may have failed above so do it here anyway
            
            BufferHeader.aColors[Counter].rgbRed =
                    LogicalPalette.aEntries[Counter].peRed;
            BufferHeader.aColors[Counter].rgbGreen =
                    LogicalPalette.aEntries[Counter].peGreen;
            BufferHeader.aColors[Counter].rgbBlue =
                    LogicalPalette.aEntries[Counter].peBlue;
            BufferHeader.aColors[Counter].rgbReserved = 0;

            LogicalPalette.aEntries[Counter].peFlags = 0;

            BufferHeader.aColors[Counter + 246].rgbRed =
                  LogicalPalette.aEntries[Counter + 246].peRed;
            BufferHeader.aColors[Counter + 246].rgbGreen =
                  LogicalPalette.aEntries[Counter + 246].peGreen;
            BufferHeader.aColors[Counter + 246].rgbBlue =
                  LogicalPalette.aEntries[Counter + 246].peBlue;
            BufferHeader.aColors[Counter + 246].rgbReserved = 0;

            LogicalPalette.aEntries[Counter + 246].peFlags = 0;
          }


          pColorTable = (RGBQUAD far *)
            ((char far *)pBitmap + pBitmap->biSize);

          for(Counter = 10;Counter < 246;Counter++)
          {
            // copy from the original color table to the WinGBitmap's
            // color table and the logical palette

            BufferHeader.aColors[Counter].rgbRed =
              LogicalPalette.aEntries[Counter].peRed =
                pColorTable[Counter].rgbRed;
            BufferHeader.aColors[Counter].rgbGreen =
              LogicalPalette.aEntries[Counter].peGreen =
                pColorTable[Counter].rgbGreen;
            BufferHeader.aColors[Counter].rgbBlue =
              LogicalPalette.aEntries[Counter].peBlue =
                pColorTable[Counter].rgbBlue;
            BufferHeader.aColors[Counter].rgbReserved = 0;
            LogicalPalette.aEntries[Counter].peFlags = PC_NOCOLLAPSE;
          }

          hpalApp = CreatePalette((LOGPALETTE far *)&LogicalPalette);
          
        //  Create a WinGDC and Bitmap, then select away
        Buffer = WinGCreateDC();
        hbm = WinGCreateBitmap(Buffer,
          (BITMAPINFO far *)&BufferHeader, &pBuffer);

        //  Store the old hbitmap to select back in before deleting
        gbmOldMonoBitmap = (HBITMAP)SelectObject(Buffer, hbm);
      }

      PatBlt(Buffer, 0,0,dx,dy, BLACKNESS);

      //  Stick the doggie into the center of the buffer
      TransparentDIBits((BITMAPINFO far *)&BufferHeader,pBuffer,
        dx/2-BitmapX/2,dy/2-BitmapY/2,DibPtr(pBitmap),DibInfo(pBitmap),
        DIB_RGB_COLORS,0,0,TransparentColor);
            break;
        case WM_MOUSEMOVE:
          if(GetKeyState(VK_LBUTTON) < 0)
          {
            hdc = GetDC(hwnd);
            Offset = GetViewportOrg(hdc);
            XOffset = LOWORD(Offset);
            YOffset = HIWORD(Offset);
          
                X = LOWORD(lParam);
                Y = HIWORD(lParam);

        SelectPalette(hdc,hpalApp,FALSE);
        RealizePalette(hdc);
            PaintDoggie(hdc,X - XOffset,Y - YOffset);
            ReleaseDC(hwnd,hdc);
          }

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
     SelectPalette(hdc, hpalApp, FALSE);
     RealizePalette(hdc);
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
    switch(wParam)
    {
        case MENU_ABOUT:
            DialogBox(hInstApp,"AppAbout",hwnd,(DLGPROC)AppAbout);
            break;

        case MENU_EXIT:
            PostMessage(hwnd,WM_CLOSE,0,0L);
            break;
    }
    return 0L;
}
