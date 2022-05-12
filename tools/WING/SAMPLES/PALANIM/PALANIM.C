/**************************************************************************

    PALANIM.C - A palette animation demo for WinG

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
#include <commdlg.h>
#include <wing.h>

#include "palanim.h"
#include "..\utils\utils.h"

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/
static  char       szAppName[]="WinG Palette Animation App";

static  HINSTANCE  hInstApp;
static  HWND       hwndApp;
static  HPALETTE   hpalApp;
static  BOOL       fAppActive;

static  HDC        hdcOffscreen;
void far *         gpBits;

struct {
  BITMAPINFOHEADER InfoHeader;
  RGBQUAD ColorTable[256];
} gInfo;

int  fAnimatePalette = 0;    // Don't animate
int  fIncludeStatic = 0;     // Use the static color entries
enum {Red, Green, Blue}  gWashColor = Red;

PALETTEENTRY  aPalette[256];

extern HBITMAP  ghBitmapMonochrome;

#ifdef WIN32
    #define _export
#endif

//*** Setting up SYSPAL_NOSTATIC

#define NumSysColors (sizeof(SysPalIndex)/sizeof(SysPalIndex[1]))
#define rgbBlack RGB(0,0,0)
#define rgbWhite RGB(255,255,255)

//*** These are the GetSysColor display element identifiers
static int SysPalIndex[] = {
  COLOR_ACTIVEBORDER,
  COLOR_ACTIVECAPTION,
  COLOR_APPWORKSPACE,
  COLOR_BACKGROUND,
  COLOR_BTNFACE,
  COLOR_BTNSHADOW,
  COLOR_BTNTEXT,
  COLOR_CAPTIONTEXT,
  COLOR_GRAYTEXT,
  COLOR_HIGHLIGHT,
  COLOR_HIGHLIGHTTEXT,
  COLOR_INACTIVEBORDER,

  COLOR_INACTIVECAPTION,
  COLOR_MENU,
  COLOR_MENUTEXT,
  COLOR_SCROLLBAR,
  COLOR_WINDOW,
  COLOR_WINDOWFRAME,
  COLOR_WINDOWTEXT
};

//*** This array translates the display elements to black and white
static COLORREF MonoColors[] = {
  rgbBlack,
  rgbWhite,
  rgbWhite,
  rgbWhite,
  rgbWhite,
  rgbBlack,
  rgbBlack,
  rgbBlack,
  rgbBlack,
  rgbBlack,
  rgbWhite,
  rgbWhite,
  rgbWhite,
  rgbWhite,
  rgbBlack,
  rgbWhite,
  rgbWhite,
  rgbBlack,

  rgbBlack
};

//*** This array holds the old color mapping so we can restore them
static COLORREF OldColors[NumSysColors];



/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   d e f i n i t i o n s                                    |
|                                                                              |
\*----------------------------------------------------------------------------*/

LONG FAR PASCAL _export AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
int  ErrMsg (LPSTR sz,...);
LONG AppCommand (HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

void AppExit(void);
BOOL AppIdle(void);

/***************************************************************************
  Internal functions for the animation
*/

void  CreateWashPalette(void);
void  DibCreateWash(BITMAPINFOHEADER far *Info, void far *pBits);
void  DibHorizontalLine(BITMAPINFOHEADER far *Info, void far *pBits,
                        int y, char unsigned color);

/***************************************************************************
  Sample functions from wing.hlp
*/

HDC   Create100x100WinGDC(void);
void  Destroy100x100WinGDC(HDC hWinGDC);

void  AppActivate(BOOL fActive);

HPALETTE CreateIdentityPalette(RGBQUAD aRGB[], int nColors);

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
  WNDCLASS  cls;
  int       dx,dy;
  HBITMAP   hbmOffscreen;
  HMENU     hMenu;
  HDC       Screen;
  int 		PaletteDevice;
  int 		nBitsPixel;         
  int 		nPlanes;         
  int       nColorDepth;
  	
    /* Save instance handle for DialogBoxes */
    hInstApp = hInst;

    /* Refuse to run if this is a non-palettized device */
    Screen = GetDC(0);

    if (Screen)
    {
      PaletteDevice = GetDeviceCaps(Screen, RASTERCAPS) & RC_PALETTE;
      nBitsPixel = GetDeviceCaps(Screen, BITSPIXEL);         
      nPlanes = GetDeviceCaps(Screen, PLANES);         
      nColorDepth = nBitsPixel * nPlanes;
      ReleaseDC(0, Screen);

      if ((8 != nColorDepth) || (0 == PaletteDevice))
      {
        MessageBox(0,
         "Palette animation requires a palettized display device - change to 256 colors.",
         "Non-palettized Display",
         MB_OK);
        return FALSE;
      }
    }

// Clear the System Palette.
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

  //*** Create the WinGDC  (actually 256x256)
  hdcOffscreen = Create100x100WinGDC();

  //*** Check the menu to reflect initial palette (red)
  hMenu = GetMenu(hwndApp);
  CheckMenuItem(hMenu, MENU_STATIC, fIncludeStatic ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hMenu, MENU_RED, MF_CHECKED);
  CheckMenuItem(hMenu, MENU_GREEN, MF_UNCHECKED);
  CheckMenuItem(hMenu, MENU_BLUE, MF_UNCHECKED);

  //*** Hack to get the offscreen HBITMAP
  hbmOffscreen = (HBITMAP)SelectObject(hdcOffscreen, ghBitmapMonochrome);
  SelectObject(hdcOffscreen, hbmOffscreen);

  //*** Initialize the DIB to a wash from top to bottom
  gpBits = WinGGetDIBPointer(hbmOffscreen, (BITMAPINFO far *)&gInfo);
  DibCreateWash(&gInfo.InfoHeader, gpBits);

  //*** Reset the color palette
  ClearSystemPalette();
  CreateWashPalette();

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
  if (hdcOffscreen)
    Destroy100x100WinGDC(hdcOffscreen);

  if (hpalApp)
    DeleteObject(hpalApp);

  //*** Be sure to restore the state on exit!
  if (fIncludeStatic)
    AppActivate(FALSE);
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
        return TRUE;
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
    RECT    rc;

    GetClientRect(hwnd,&rc);

    SetTextColor(hdc,GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc,GetSysColor(COLOR_WINDOW));

  if (hdcOffscreen)
  {
    int i;
    PALETTEENTRY aPalette[256];
    RGBQUAD aPaletteRGB[256];
 
    //*** BEFORE BLTTING, match the DIB color table to the
    //*** current palette to match the animated palette
    GetPaletteEntries(hpalApp, 0, 256, aPalette);
    //*** Alas, palette entries are r-g-b, rgbquads are b-g-r
    for (i=0; i<256; ++i)
    {
      aPaletteRGB[i].rgbRed = aPalette[i].peRed;
      aPaletteRGB[i].rgbGreen = aPalette[i].peGreen;
      aPaletteRGB[i].rgbBlue = aPalette[i].peBlue;
      aPaletteRGB[i].rgbReserved = 0;
    }
    WinGSetDIBColorTable(hdcOffscreen, 0, 256, aPaletteRGB);

    WinGBitBlt(hdc,0,0,256,256,hdcOffscreen,0,0);
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
    static int CurtainY = 0;


    switch (msg)
    {
      case WM_CREATE:
        break;

      case WM_ACTIVATEAPP:
        fAppActive = (BOOL)wParam;

        //*** Remap the system colors and deal with the palette
        if (fIncludeStatic == 0)
        {
          AppActivate(fAppActive);

          if (hpalApp)
          {
            hdc = GetDC(hwnd);

            UnrealizeObject(hpalApp);
            SelectPalette(hdc, hpalApp, FALSE);
            RealizePalette(hdc);

            ReleaseDC(hwnd, hdc);
          }
        }
        break;

    case WM_TIMER:
      switch (wParam)
      {
        case 1:
          //*** Animate or terminate the "falling black curtain"
          {
            if (CurtainY < 256)
            {
              DibHorizontalLine(&gInfo.InfoHeader, gpBits, CurtainY, 0);
              CurtainY++;
            }
            else
            {
              CurtainY = 0;
              DibCreateWash(&gInfo.InfoHeader, gpBits);
              KillTimer(hwnd, wParam);
            }
          }

          //*** The DIB has changed - redisplay it
          InvalidateRect(hwnd, NULL, FALSE);
          UpdateWindow(hwnd);

          break;

        case 2:
          //*** Get the current palette
          GetPaletteEntries(hpalApp, 0, 256, aPalette);
	      hdc = GetDC(hwnd);

          if (fIncludeStatic)
          {
            //*** We'll rotate the middle 236 entries, leaving
            //*** black and white at the top
            aPalette[245] = aPalette[10];
	        SelectPalette(hdc, hpalApp, FALSE);
            AnimatePalette(hpalApp, 10, 236, &aPalette[11]);
          }
          else
          {
            //*** We'll rotate the middle 254 entries, leaving
            //*** black and white at the top
            aPalette[255] = aPalette[1];
	        SelectPalette(hdc, hpalApp, FALSE);
            AnimatePalette(hpalApp, 1, 254, &aPalette[2]);
          }

          ReleaseDC(hwnd, hdc);

          InvalidateRect(hwnd, NULL, FALSE);
          UpdateWindow(hwnd);

          break;
      }
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
  HMENU hMenu;
  HPALETTE hPal = GetStockObject(DEFAULT_PALETTE);
  HDC hdc = GetDC(hwnd);

    switch(wParam)
    {
        case MENU_ABOUT:
            DialogBox(hInstApp,"AppAbout",hwnd,AppAbout);
            break;

        case MENU_EXIT:
            PostMessage(hwnd,WM_CLOSE,0,0L);
            break;

    case MENU_RED:
    case MENU_GREEN:
    case MENU_BLUE:
      hMenu = GetMenu(hwndApp);

      CheckMenuItem(hMenu, MENU_RED, MF_UNCHECKED);
      CheckMenuItem(hMenu, MENU_GREEN, MF_UNCHECKED);
      CheckMenuItem(hMenu, MENU_BLUE, MF_UNCHECKED);
      CheckMenuItem(hMenu, wParam, MF_CHECKED);
      
      if (wParam == MENU_RED) gWashColor = Red;
      else if (wParam == MENU_GREEN) gWashColor = Green;
      else gWashColor = Blue;

      //*** Delete the old palette and create a new one
      hPal = SelectPalette(hdc, hPal, FALSE);
      DeleteObject(hPal);

      CreateWashPalette();

      InvalidateRect(hwnd, NULL, FALSE);
      UpdateWindow(hwnd);
      break;

    case MENU_PALETTE:
      hMenu = GetMenu(hwndApp);
      if (fAnimatePalette)
      {
        fAnimatePalette = 0;
        CheckMenuItem(hMenu, MENU_PALETTE, MF_UNCHECKED);
        KillTimer(hwnd, 2);
      }
      else
      {
        fAnimatePalette = 1;
        CheckMenuItem(hMenu, MENU_PALETTE, MF_CHECKED);
        SetTimer(hwnd, 2, 1, 0);
      }
      break;

    case MENU_CURTAIN:
      //*** Just start off the falling curtain
      SetTimer(hwnd, 1, 10, 0);
      break;

    case MENU_STATIC:
      hMenu = GetMenu(hwndApp);

      if (fIncludeStatic)
      {
        //*** Flag no static color use
        fIncludeStatic = 0;
        CheckMenuItem(hMenu, MENU_STATIC, MF_UNCHECKED);

        //*** Remap the system colors
        AppActivate(TRUE);
      }
      else
      {
        //*** Flag static color use
        fIncludeStatic = 1;
        CheckMenuItem(hMenu, MENU_STATIC, MF_CHECKED);

        //*** Remap the system colors to normal
        AppActivate(FALSE);
      }

      //*** Delete the old palette and create a new one
      hPal = SelectPalette(hdc, hPal, FALSE);
      DeleteObject(hPal);

      CreateWashPalette();

      InvalidateRect(hwnd, NULL, FALSE);
      UpdateWindow(hwnd);
      break;
    }

   ReleaseDC(hwnd, hdc);

    return 0L;
}

/*----------------------------------------------------------------------------*\
|   ErrMsg - Opens a Message box with a error message in it.  The user can     |
|            select the OK button to continue                                  |
\*----------------------------------------------------------------------------*/
int ErrMsg (LPSTR sz,...)
{
    char ach[128];

    wvsprintf (ach,sz,(LPSTR)(&sz+1));   /* Format the string */
    MessageBox(hwndApp,ach,szAppName,MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
    return FALSE;
}


/***************************************************************************
  Palette Creation
  Create three palettes of solid color washes, with or without statics
*/

void CreateWashPalette(void)
{
  RGBQUAD aWash[256];
  int i;

  //*** Fill in the palette with a 256-color ramp
  for (i=0; i<256; ++i)
  {
    aWash[i].rgbRed = aWash[i].rgbBlue = aWash[i].rgbGreen = 0;

    switch (gWashColor)
    {
      case Red:
        aWash[i].rgbRed = i;
        break;

      case Green:
        aWash[i].rgbGreen = i;
        break;

      case Blue:
        aWash[i].rgbBlue = i;
        break;
    }

    aWash[i].rgbReserved = 0;
  }

  //*** Turn the wash into a palette
  if (hpalApp)
    DeleteObject(hpalApp);
  hpalApp = CreateIdentityPalette(aWash, 256);
}

/***************************************************************************
  Dib drawing routines

  One creates a wash of color from left to right,
  the other draws a horizontal line at a given Y in the DIB
*/

void DibCreateWash(BITMAPINFOHEADER far *Info, void far *pBits)
{
  unsigned int dxBytes = DibWidthBytes(Info);
  int dxWidth = DibWidth(Info);
  int dyLines = (int)Info->biHeight;
  int i, j;
  char unsigned huge *pScanline = (char unsigned huge *)pBits;
  char unsigned huge *pPixel;

  if (dyLines < 0)
  {
    dyLines = -dyLines;
  }

  for (i=0; i<dyLines; ++i)
  {
    //*** Point to the beginning of this scan line in the DIB
    pPixel = pScanline;

    //*** Step through this scan line and fill it
    //*** Wash up on the evens, then down on the odds
    for (j=0; j<256; j+=2)
    {
      *pPixel = (char unsigned)(j % 256);
      pPixel++;
    }
    for (j=253; j>0; j-=2)
    {
      *pPixel = (char unsigned)(j % 256);
      pPixel++;
    }
    //*** Make the last column white
    *pPixel = 255;

    //*** Move pointer to the next scan line
    pScanline += dxBytes;
  }
}

void DibHorizontalLine(BITMAPINFOHEADER far *Info, void far *pBits,
  int y, char unsigned color)
{
  unsigned int dxBytes = DibWidthBytes(Info);
  char unsigned huge *pPixel;
  int dxWidth = DibWidth(Info);
  int dyLines = (int)Info->biHeight;
  int i;

  //*** Account for top-down and bottom-up DIBs
  if (dyLines > 0)
  {
    pPixel = (char unsigned huge *)pBits +
      (long)(dyLines - 1 - y) * (long)dxBytes;
  }
  else
  {
    pPixel = (char unsigned huge *)pBits +
      (long)y * (long)dxBytes;
  }

  for (i=0; i<dxWidth; ++i)
  {
    *pPixel = color;
    pPixel++;
  }
}


//*** Creating an identity palette code here

HPALETTE CreateIdentityPalette(RGBQUAD aRGB[], int nColors)
{
  int i;
  struct
  {
    WORD Version;
    WORD NumberOfEntries;
    PALETTEENTRY aEntries[256];
  } Palette =
  {
    0x300,
    256
  };
  HDC hdc = GetDC(NULL);

  //*** For SYSPAL_NOSTATIC, just copy the color table into
  //*** a PALETTEENTRY array and replace the first and last entries
  //*** with black and white
  if (GetSystemPaletteUse(hdc) == SYSPAL_NOSTATIC)
  {
    //*** Fill in the palette with the given values, marking each
    //*** as PC_RESERVED
    for(i = 0; i < nColors; i++)
    {
      Palette.aEntries[i].peRed = aRGB[i].rgbRed;
      Palette.aEntries[i].peGreen = aRGB[i].rgbGreen;
      Palette.aEntries[i].peBlue = aRGB[i].rgbBlue;
      Palette.aEntries[i].peFlags = PC_RESERVED;
    }

    //*** Mark any remaining entries PC_RESERVED
    for (; i < 256; ++i)
    {
      Palette.aEntries[i].peFlags = PC_RESERVED;
    }

    //*** Make sure the last entry is white
    //*** This may replace an entry in the array!
    Palette.aEntries[255].peRed = 255;
    Palette.aEntries[255].peGreen = 255;
    Palette.aEntries[255].peBlue = 255;
    Palette.aEntries[255].peFlags = 0;

    //*** And the first is black
    //*** This may replace an entry in the array!
    Palette.aEntries[0].peRed = 0;
    Palette.aEntries[0].peGreen = 0;
    Palette.aEntries[0].peBlue = 0;
    Palette.aEntries[0].peFlags = 0;
  }
  else
  //*** For SYSPAL_STATIC, get the twenty static colors into
  //*** the array, then fill in the empty spaces with the
  //*** given color table
  {
    int nStaticColors;
    int nUsableColors;

    //*** Get the static colors
    nStaticColors = GetDeviceCaps(hdc, NUMCOLORS);
    GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

    //*** Set the peFlags of the lower static colors to zero
    nStaticColors = nStaticColors / 2;
    for (i=0; i<nStaticColors; i++)
      Palette.aEntries[i].peFlags = 0;

    //*** Fill in the entries from the given color table
    nUsableColors = nColors - nStaticColors;
    for (; i<nUsableColors; i++)
    {
      Palette.aEntries[i].peRed = aRGB[i].rgbRed;
      Palette.aEntries[i].peGreen = aRGB[i].rgbGreen;
      Palette.aEntries[i].peBlue = aRGB[i].rgbBlue;
      Palette.aEntries[i].peFlags = PC_RESERVED;
    }

    //*** Mark any empty entries as PC_RESERVED
    for (; i<256 - nStaticColors; i++)
      Palette.aEntries[i].peFlags = PC_RESERVED;

    //*** Set the peFlags of the upper static colors to zero
    for (i = 256 - nStaticColors; i<256; i++)
      Palette.aEntries[i].peFlags = 0;
  }

  ReleaseDC(NULL, hdc);

  //*** Create the palette
  return CreatePalette((LOGPALETTE *)&Palette);
}


//*** AppActivate sets the system palette use and
//*** remaps the system colors accordingly.
void AppActivate(BOOL fActive)
{
  HDC hdc;
  int i;

  //*** Just use the screen DC
  hdc = GetDC(NULL);

  //*** If the app is activating, save the current color mapping
  //*** and switch to SYSPAL_NOSTATIC
  if (fActive && GetSystemPaletteUse(hdc) == SYSPAL_STATIC)

  {
    //*** Store the current mapping
    for (i=0; i<NumSysColors; i++)
      OldColors[i] = GetSysColor(SysPalIndex[i]);

    //*** Switch to SYSPAL_NOSTATIC and remap the colors
    SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
    SetSysColors(NumSysColors, SysPalIndex, MonoColors);
  }
  else if (!fActive)
  {
    //*** Always switch back to SYSPAL_STATIC and the old mapping
    SetSystemPaletteUse(hdc, SYSPAL_STATIC);

    SetSysColors(NumSysColors, SysPalIndex, OldColors);
  }

  //*** Be sure to release the DC!
  ReleaseDC(NULL,hdc);
}


//*** Creating an offscreen buffer (WinGCreateBitmap)

HBITMAP ghBitmapMonochrome = 0;

HDC Create100x100WinGDC(void)
{
  HDC hWinGDC;
  HBITMAP hBitmapNew;
  struct {
    BITMAPINFOHEADER InfoHeader;
    RGBQUAD ColorTable[256];
  } Info;
  void far *pSurfaceBits;

  // Set up an optimal bitmap
  if (WinGRecommendDIBFormat((BITMAPINFO far *)&Info) == FALSE)
    return 0;

  // Set the width and height of the DIB but preserve the
  // sign of biHeight in case top-down DIBs are faster

  // NOTE: Changed 100 to 256 for my purposes...
  Info.InfoHeader.biHeight *= 256;

  Info.InfoHeader.biWidth = 256;

  // Create a WinGDC and Bitmap, then select away
  hWinGDC = WinGCreateDC();

  if (hWinGDC)
  {
    hBitmapNew = WinGCreateBitmap(hWinGDC,
      (BITMAPINFO far *)&Info, &pSurfaceBits);
    if (hBitmapNew)
    {
      ghBitmapMonochrome = (HBITMAP)SelectObject(hWinGDC,
        hBitmapNew);
    }
    else
    {
      DeleteDC(hWinGDC);
      hWinGDC = 0;

    }
  }

  return hWinGDC;
}

void Destroy100x100WinGDC(HDC hWinGDC)
{
  HBITMAP hBitmapOld;

  if (hWinGDC && ghBitmapMonochrome)
  {
    // Select the stock 1x1 monochrome bitmap back in
    hBitmapOld = (HBITMAP)SelectObject(hWinGDC,         
    ghBitmapMonochrome);
    DeleteObject(hBitmapOld);
    DeleteDC(hWinGDC);
  }
}


