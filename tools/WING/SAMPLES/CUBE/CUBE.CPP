/**************************************************************************

    CUBE.CPP - A spinning cube demo for WinG

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
#include <wing.h>

#include "cube.hpp"
#include "dumb3d.hpp"
#include "..\utils\utils.h"

#if defined(WIN32)
#define _export
#endif

/**************************************************************************
  Global Variables
 **************************************************************************/

static char szAppName[]="Spinning Cube";

//*** Global Windows needs
static HINSTANCE  hInstApp;
static BOOL       fAppActive;
static HWND       hwndApp;
static HPALETTE   hpalApp = 0;
static HDC        hdcWinG;
static HBITMAP    OldBitmap;
static HBITMAP    WinGBitmap;

struct
{
  BITMAPINFOHEADER  Header;
  RGBQUAD           aColorTable[256];

} HeaderAndPalette =
{
  sizeof(BITMAPINFOHEADER),
  50, 50,
  1, 8,
  BI_RGB,
  0, 0, 0, 0, 0
};


static int        DibWidth, DibHeight;

//*** Cube vertices, normals, shades, and modeling transform
static point_4 CubeVertices[8] =
{
  point_4( -10,  10, -10 ),
  point_4( -10,  10,  10 ),
  point_4(  10,  10,  10 ),
  point_4(  10,  10, -10 ),
  point_4(  10, -10, -10 ),
  point_4(  10, -10,  10 ),
  point_4( -10, -10,  10 ),
  point_4( -10, -10, -10 )
};
static vector_4   CubeSurfaceNormals[6];
static real       CubeSurfaceShades[6];
static matrix_4x4 CubeTransform;

//*** Cube edges - ordered indices into the vertex array
const int CubeFaces[6][4] =
{
  0, 1, 2, 3,
  2, 1, 6, 5,
  3, 2, 5, 4,
  0, 3, 4, 7,
  1, 0, 7, 6,
  4, 5, 6, 7
};

//*** Cube colors - one RGB color per surface
const unsigned char CubeColors[6][3] =
{
  240,  20,  20,    // Unsaturated Red
   20, 240,  20,    // Unsaturated Green
   20,  20, 240,    // Unsaturated Blue
  128,  64,   0,    // Brown
  240,  20, 240,    // Unsaturated Magenta
  240, 240,  20     // Unsaturated Yellow
};

//*** Lighting
vector_4   LightSourceDirection;
const real AmbientLight = 0.2;

//*** Viewing and perspective
static matrix_4x4  ViewPerspective;
static point_4     Viewpoint(60, 60, 60);
static vector_4    Up(0, 1, 0);
static point_4     Origin;

//*** Interaction
static real   XMove,YMove;
static short  gSpinFlag = 1;

//*** Dithering
static int DitherType = 0;
static int Monochrome = 0;

/**************************************************************************
   Internal function declarations
 **************************************************************************/

LONG FAR PASCAL _export  AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
void  AppExit(void);
BOOL  AppIdle(void);
void  AppPaint(HWND hwnd, HDC hdc);

void  TransformCube(matrix_4x4 const &Transform);
void  ProjectAndDrawCube(HDC hdc, int XOffset, int YOffset);

/**************************************************************************
  AppAbout

  Description:
    This function handles messages belonging to the "About" dialog box.
  The only message that it looks for is WM_COMMAND, indicating the user
  has pressed the "OK" button.
 **************************************************************************/

BOOL FAR PASCAL _export AppAbout(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  switch (msg)
  {
    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK)
        EndDialog(hwnd, TRUE);
      break;

    case WM_INITDIALOG:
      return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  AppInit

  Description:
    This is called when the application is first loaded. It initializes
  all variables, registers the window class, and creates the main app
  window.
 **************************************************************************/

BOOL AppInit(HINSTANCE hInst,HINSTANCE hPrev,int sw,LPSTR szCmdLine)
{
  WNDCLASS cls;

  /* Save instance handle for DialogBoxes */
  hInstApp = hInst;

// Clear the System Palette so that WinG blting runs at full speed.
  ClearSystemPalette();

  if (!hPrev)
  {
    //***  Register a class for the main application window
    cls.hCursor        = LoadCursor(0,IDC_ARROW);

    //*** Just for fun, we'll draw our own spinning cube icon.
    cls.hIcon          = 0;
    cls.lpszMenuName   = "AppMenu";
    cls.lpszClassName  = szAppName;
    cls.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
    cls.hInstance      = hInst;
    cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW;
    cls.lpfnWndProc    = (WNDPROC)AppWndProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = 0;

    if (!RegisterClass(&cls))
      return FALSE;
  }

  //*** Set and normalize the light source
  LightSourceDirection = vector_4(50, 30, -15);
  LightSourceDirection.Normalize();

  //*** Distance to view plane:
  ViewPerspective.SetElement(3, 2, 1/300.0);
  ViewPerspective.SetElement(3, 3, 0);

  //*** Viewport scaling - some arbitrary number like 3.5 will do
  ViewPerspective.SetElement(0, 0, 3.5);
  ViewPerspective.SetElement(1, 1, 3.5);

  //*** Calculate the initial normals and shades
  TransformCube(CubeTransform);

  //*** Then generate an interesting rotation for the spin
  CubeTransform.ConcatenateYRotation(6.0);
  CubeTransform.ConcatenateXRotation(3.5);
  CubeTransform.ConcatenateZRotation(2.0);

  hwndApp = CreateWindow (szAppName,   // Class name
                  szAppName,           // Caption
                  WS_OVERLAPPED |
                  WS_CAPTION |
                  WS_SYSMENU |
                  WS_MINIMIZEBOX,      // Style bits
                  CW_USEDEFAULT, 0,    // Position
                  350,350,             // Size
                  0,                   // Parent window (no parent)
                  0,                   // use class menu
                  hInst,               // handle to window instance
                  0                    // no params to pass on
                  );
  hdcWinG = WinGCreateDC();

  ShowWindow(hwndApp,sw);

  //*** Check the default dither selection
  HMENU hMenu = GetMenu(hwndApp);
  CheckMenuItem(hMenu, MENU_DISPERSED8x8, MF_CHECKED);
  CheckMenuItem(hMenu, MENU_SPIN, MF_CHECKED);

  return TRUE;
}

/**************************************************************************
  WinMain

  Description:
    The main procedure for the App.  After initializing, it just goes
  into a message-processing loop until it gets a WM_QUIT message.
 **************************************************************************/

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
  MSG     msg;

  //*** Call initialization procedure
  if (!AppInit(hInst,hPrev,sw,szCmdLine))
    return FALSE;

    //*** Polling messages from event queue until quit
  for (;;)
  {
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
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

  return msg.wParam;
}

/**************************************************************************
  AppIdle

  Description:
 **************************************************************************/

BOOL AppIdle()
{
  //*** Spin while the app is active, lbutton is up, and spinning is on.
  //*** Spin while the app is iconized.
  if ( (gSpinFlag && fAppActive && GetKeyState(VK_LBUTTON) >= 0)
      || IsIconic(hwndApp))
  {
    //*** If the app is active, spin the cube and redraw
    TransformCube(CubeTransform);
    HDC hdc = GetDC(hwndApp);
    if (hpalApp)
    {
      SelectPalette(hdc, hpalApp, FALSE);
      RealizePalette(hdc);
    }
    AppPaint(hwndApp, hdc);
    ReleaseDC(hwndApp, hdc);
    return FALSE;
  }
  else
  {
    //*** Don't do anything when not the active app
    return TRUE;
  }
}

/**************************************************************************
  AppPaint

  Description:
    The paint function. Draws the centered cube in the offscreen DIBDC,
  then copies it to the screen using WinGBitBlt or WinGStretchBlt.
 **************************************************************************/

void AppPaint(HWND hwnd, HDC hdc)
{
  //*** Clear the DIBDC buffer to white
  PatBlt(hdcWinG, 0, 0, DibWidth, DibHeight, WHITENESS);

  //*** Move the viewpoint according to the mouse movement
  //*** Rotate it around the Y and X axes
  if(YMove || XMove)
  {
    matrix_4x4 Movement;
    Movement.ConcatenateYRotation(-YMove);
    Movement.ConcatenateXRotation(XMove);

    XMove = YMove = 0;

    TransformCube(Movement);
  }

  //*** and GO!
  ProjectAndDrawCube(hdcWinG, DibWidth/2, DibHeight/2);

  RECT rc;
  GetClientRect(hwndApp, &rc);

  //*** Flip the buffers using WinGBitBlt or WinGStretchBlt
  if (IsIconic(hwndApp))
  {
    WinGStretchBlt(hdc,0,0,rc.right,rc.bottom,hdcWinG,0,0,
        DibWidth,DibHeight);
  }
  else
    WinGBitBlt(hdc,0,0,rc.right,rc.bottom,hdcWinG,0,0);
}

/**************************************************************************
  AppWndProc

  Description:
    Main window proc. Standard Windows fare.
 **************************************************************************/

LONG FAR PASCAL _export AppWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;
  BOOL f;
  HMENU hMenu;
  static int LastX, LastY;

  switch (msg)
  {
    case WM_CREATE:
    {
      //*** Use the halftone palette to make things pretty
      hpalApp = WinGCreateHalftonePalette();
      GetPaletteEntries(hpalApp,0,256,
        (PALETTEENTRY far *)HeaderAndPalette.aColorTable);

      for(int Counter = 0;Counter < 256;Counter++)
      {
        // PALETTEENTRYs and RGBQUADs are backwards

        BYTE Temp = HeaderAndPalette.aColorTable[Counter].rgbBlue;

        HeaderAndPalette.aColorTable[Counter].rgbBlue =
            HeaderAndPalette.aColorTable[Counter].rgbRed;

        HeaderAndPalette.aColorTable[Counter].rgbRed = Temp;
      }
      break;
    }
    case WM_ACTIVATEAPP:
      //*** Keep track of whether or not the app is in the foreground
      fAppActive = (BOOL)wParam;
      break;

    case WM_COMMAND:
      switch(wParam)
      {
        case MENU_ABOUT:
          DialogBox(hInstApp, "AppAbout", hwnd, (DLGPROC)AppAbout);
          break;

        case MENU_EXIT:
          PostMessage(hwnd, WM_CLOSE, 0, 0L);
          break;

        case MENU_SPIN:
          //*** Toggle the spin flag and the check mark
          hMenu = GetMenu(hwnd);
          if (gSpinFlag)
          {
            gSpinFlag = 0;
            CheckMenuItem(hMenu, MENU_SPIN, MF_UNCHECKED);
          }
          else
          {
            gSpinFlag = 1;
            CheckMenuItem(hMenu, MENU_SPIN, MF_CHECKED);
          }
          break;

        case MENU_DISPERSED8x8:
        case MENU_DISPERSED4x4:
        case MENU_CLUSTERED4x4:
		  case MENU_UNDITHERED:
          hMenu = GetMenu(hwnd);

          //*** Uncheck the current selection
          CheckMenuItem(hMenu, MENU_DISPERSED8x8 + DitherType,
            MF_UNCHECKED);

          //*** Get the new selection and check it
          DitherType = wParam - MENU_DISPERSED8x8;
          CheckMenuItem(hMenu, wParam, MF_CHECKED);

          //*** Redraw
          InvalidateRect(hwnd, 0, FALSE);
          UpdateWindow(hwnd);
          break;

		  case MENU_MONOCHROME:
          hMenu = GetMenu(hwnd);

          Monochrome = (Monochrome == 1) ? 0 : 1;
          CheckMenuItem(hMenu, wParam,
            (Monochrome == 1) ? MF_CHECKED : MF_UNCHECKED);

          //*** Redraw
          InvalidateRect(hwnd, 0, FALSE);
          UpdateWindow(hwnd);
          break;
      }
      return 0L;

    case WM_DESTROY:
      //*** Clean up before leaving
      if (hpalApp)
        DeleteObject(hpalApp);
      if (hdcWinG)
      {
        SelectObject(hdcWinG,OldBitmap);
        DeleteObject(WinGBitmap);
        DeleteDC(hdcWinG);
      }

      PostQuitMessage(0);
      break;

    case WM_LBUTTONDOWN:
      //*** Get the start location for mouse rotations
      LastX = LOWORD(lParam);
      LastY = HIWORD(lParam);
      break;

    case WM_MOUSEMOVE:
      //*** While the mouse button is down, keep track of movement
      //*** to update the eye position on AppPaint.
      if(GetKeyState(VK_LBUTTON) < 0)
      {
        int X = LOWORD(lParam);
        int Y = HIWORD(lParam);

        YMove = X - LastX;
        XMove = Y - LastY;

        LastX = X;
        LastY = Y;

        InvalidateRect(hwnd, 0, FALSE);
        UpdateWindow(hwnd);
      }
      break;

    case WM_PALETTECHANGED:
      if ((HWND)wParam == hwnd)
        break;
      //*** Else fall through to WM_QUERYNEWPALETTE

    case WM_QUERYNEWPALETTE:
      hdc = GetDC(hwnd);
      if (hpalApp)
        SelectPalette(hdc, hpalApp, FALSE);
      f = RealizePalette(hdc);
      ReleaseDC(hwnd,hdc);

      if (f)
        InvalidateRect(hwnd, 0, FALSE);
      return f;

     case WM_PAINT:
      hdc = BeginPaint(hwnd,&ps);

      if (hpalApp)
      {
        SelectPalette(hdc, hpalApp, FALSE);
        RealizePalette(hdc);
      }
      AppPaint (hwnd, hdc);

      EndPaint(hwnd,&ps);
      return 0L;

    case WM_SIZE:
      if (wParam != SIZE_MINIMIZED)
      {
        //*** Create a WinGBitmap for the buffer that fills the client area
        if (WinGBitmap)
        {
          SelectObject(hdcWinG,OldBitmap);
          DeleteObject(WinGBitmap);
        }
        RECT rect;
        GetClientRect(hwnd, &rect);

		  //*** Set up the Header for the WinGBitmap
		  WinGRecommendDIBFormat((BITMAPINFO FAR *)&HeaderAndPalette);
        HeaderAndPalette.Header.biWidth = rect.right;
        HeaderAndPalette.Header.biHeight *= rect.bottom;

        WinGBitmap = WinGCreateBitmap(hdcWinG,
          (BITMAPINFO far *)&HeaderAndPalette,0);

        OldBitmap = SelectBitmap(hdcWinG,WinGBitmap);
        DibWidth = rect.right;
        DibHeight = rect.bottom;
      }

      //*** Select a null pen so the polygons aren't outlined
      if (hdcWinG)
        SelectPen(hdcWinG, GetStockObject(NULL_PEN));
      break;
  }

  return DefWindowProc(hwnd,msg,wParam,lParam);
}

/**************************************************************************
  TransformCube

  Description:
    Transforms the cube vertices by the current rotation matrix.
    Recalculates normals and flat shade values for the
  directional light source.
 **************************************************************************/

void TransformCube(matrix_4x4 const &Transform)
{
  int i;

  //*** Transform the cube by the matrix
  for (i = 0; i < 8; ++i)
    CubeVertices[i] = Transform * CubeVertices[i];

  //*** Recalculate normals and shades
  for (i = 0; i < 6; ++i)
  {
    //*** Normals are perpendicular to two edges of the cube
    vector_4 Edge1, Edge2;
    Edge1 = CubeVertices[CubeFaces[i][1]] - CubeVertices[CubeFaces[i][0]];
    Edge2 = CubeVertices[CubeFaces[i][3]] - CubeVertices[CubeFaces[i][0]];
    CubeSurfaceNormals[i] = CrossProduct(Edge1, Edge2);
    CubeSurfaceNormals[i].Normalize();

    //*** Cosine shading based on the surface normal, clamped to [0, 1]
    real Shade = DotProduct(CubeSurfaceNormals[i], LightSourceDirection);
    Shade = Shade + AmbientLight;
    if (Shade < 0) Shade = 0;
    else if (Shade > 1.0) Shade = 1.0;
    CubeSurfaceShades[i] = Shade;
  }
}

/**************************************************************************
  ProjectAndDrawCube

  Description:
    Projects the cube vertices for the current viewpoint then culls
  in screen space and draws into the DC. In this case, the DC is a DIBDC.
 **************************************************************************/

void ProjectAndDrawCube(HDC hdc, int XOffset, int YOffset)
{
  //*** Create a viewing transform for the current eye position
  vector_4 ViewDirection = Origin - Viewpoint;
  ViewDirection.Normalize();
  view_transform View(Viewpoint, ViewDirection, Up);

  //*** Transform and project the vertices into screen space
  int i;
  POINT aScreenVertices[8];
  for (i = 0; i < 8; ++i)
  {
    point_4 Temp = View * CubeVertices[i];
    Temp = ViewPerspective * Temp;
    Temp.Homogenize();

    aScreenVertices[i].x = (int)Temp.GetX() + XOffset;
    aScreenVertices[i].y = (int)Temp.GetY() + YOffset;
  }

  for (i = 0; i < 6; ++i)
  {
    //*** Standard culling operation based on the z value of the
    //*** cross product of the edges: are the vertices oriented in the
    //*** counterclockwise or clockwise direction?
    real v1 = aScreenVertices[ CubeFaces[i][2] ].x -
      aScreenVertices[ CubeFaces[i][1] ].x;
    real w1 = aScreenVertices[ CubeFaces[i][0] ].x -
      aScreenVertices[ CubeFaces[i][1] ].x;
    real v2 = aScreenVertices[ CubeFaces[i][2] ].y -
      aScreenVertices[ CubeFaces[i][1] ].y;
    real w2 = aScreenVertices[ CubeFaces[i][0] ].y -
      aScreenVertices[ CubeFaces[i][1] ].y;
    if ((v1*w2 - v2*w1) <= 0)
      continue;

    //*** Create a brush for the shaded face color using the selected dither

	 HBRUSH hbr;
    static WING_DITHER_TYPE DitherTypeList[3] =
      { WING_DISPERSED_8x8, WING_DISPERSED_4x4, WING_CLUSTERED_4x4 };

    //*** Get the shading colors

	 int Red, Green, Blue;

	 if (Monochrome)
    {
      Red = Green = Blue = real(240) * CubeSurfaceShades[i];
    }
    else
    {
      Red = (real)CubeColors[i][0] * CubeSurfaceShades[i];
      Green = (real)CubeColors[i][1] * CubeSurfaceShades[i];
      Blue = (real)CubeColors[i][2] * CubeSurfaceShades[i];
    }

    //*** Create the dithered or PALETTERGB brush

    COLORREF cr;

    if (DitherType > 2)
    {
      cr = PALETTERGB(Red, Green, Blue);
      hbr = WinGCreateHalftoneBrush(hdcWinG,cr,WING_DISPERSED_4x4);
    }
    else
    {
      cr = RGB(Red, Green, Blue);
      hbr = WinGCreateHalftoneBrush(hdcWinG,cr,DitherTypeList[DitherType]);
    }

    //*** Collect the correct points in an array
    POINT aQuadVertices[4];
    for (int j = 0; j < 4; ++j)
      aQuadVertices[j] = aScreenVertices[ CubeFaces[i][j] ];

    //*** Use GDI to draw the face
    hbr = SelectBrush(hdc, hbr);
    Polygon(hdc, aQuadVertices, 4);
    hbr = SelectBrush(hdc, hbr);
    DeleteObject(hbr);
  }
}
