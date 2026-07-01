//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------

// world.cpp : world window
//

#include "stdafx.h"
#include "world.h"
#include "lastplnt.h"
#include "error.h"
#include "Perf.h"   // radar render profiling + O(units) minimap
#include "area.h"
#include "icons.h"
#include "bitmaps.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "SDL2Panel.h"
#include "RenderingAdapter.h"
#include <SDL.h>

#include "ui.inl"
#include "terrain.inl"
#include "minerals.inl"
#include "unit.inl"
#include "building.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

const int BTN_X_OFF = 8;
const int BTN_Y_OFF = 8;


DWORD CWndWorld::m_clrTerrain[CHex::num_types];        // same order as CHex m_bType
DWORD CWndWorld::m_clrTerrainPaper[CHex::num_types];   // parchment palette — world map only
DWORD CWndWorld::m_clrResources[4];
DWORD CWndWorld::m_clrResHigh[4];
DWORD CWndWorld::m_clrLocation;
DWORD CWndWorld::m_clrHit;

COLORREF CWndWorld::m_rgbTerrain[CHex::num_types] = {
        RGB (128, 128, 128),
        RGB (251, 206, 152),
        RGB (57, 90, 57),
        RGB (71, 90, 152),
        RGB (195, 171, 156),
        RGB (82, 70, 58),
        RGB (115, 123, 201),
        RGB (90, 132, 66),
        RGB (99, 140, 201),
        RGB (124, 109, 109),
        RGB (127, 112, 68),
        RGB (66, 108, 81),
        RGB (87, 83, 51),
        RGB (131, 98, 69)};
// Parchment "paper navigation map" palette — used by the WORLD MAP only (the radar
// keeps the satellite-style m_rgbTerrain above). Cream/tan land, ink-blue water,
// sepia relief — order matches CHex m_bType (city, desert, forest, lake, hill,
// mountain, ocean, plain, river, road, rough, swamp, coastline, fields).
COLORREF CWndWorld::m_rgbTerrainPaper[CHex::num_types] = {
        RGB (150, 122,  92),    // city      — darker sepia (built-up)
        RGB (227, 207, 160),    // desert    — pale sand
        RGB (158, 168, 120),    // forest    — muted sage-green
        RGB (140, 165, 175),    // lake      — soft chart-blue
        RGB (206, 178, 132),    // hill      — light tan
        RGB (170, 136,  98),    // mountain  — relief brown
        RGB (120, 150, 168),    // ocean     — ink-blue
        RGB (224, 205, 158),    // plain     — cream
        RGB (146, 170, 178),    // river     — chart-blue
        RGB ( 96,  74,  52),    // road      — dark ink
        RGB (196, 178, 140),    // rough     — weathered tan
        RGB (150, 162, 128),    // swamp     — drab olive
        RGB (188, 196, 178),    // coastline — pale shore
        RGB (176, 184, 132)};   // fields    — light green-tan
COLORREF CWndWorld::m_rgbResources[4] = {
        RGB (156, 153, 175),
        RGB (8, 9, 9),
        RGB (65, 65, 76),
        RGB (143, 56, 30)};
COLORREF CWndWorld::m_rgbResHigh[4] = {
        RGB (231, 226, 225),
        RGB (127, 133, 130),
        RGB (131, 131, 141),
        RGB (228, 136, 123)};
COLORREF CWndWorld::m_rgbLocation = RGB (0, 0, 0);
COLORREF CWndWorld::m_rgbHit = RGB (255, 0, 0);


/////////////////////////////////////////////////////////////////////////////
// CWndWorld

CWndWorld::CWndWorld() {

    m_pdibButtons = NULL;
    m_pdibRadar = NULL;
    m_piRadarEdges = NULL;
    m_pdibGround0 = NULL;
    m_pdibBase = NULL;

    m_bRBtnDown = FALSE;
    m_bLBtnDown = FALSE;
    m_bRCmdDown = FALSE;
    m_bCapMouse = FALSE;
    m_bNewDir = TRUE;
    m_bNewMode = TRUE;
    m_bNewLocation = TRUE;
    m_bUpdate = TRUE;
    m_bIsRadar = FALSE;

    m_yAdd = 0;                                // for map::window scaling
    m_yRem = 0;
    m_xAdd = 0;
    m_xRem = 0;
    m_xDib = 0;
    m_yDib = 0;
    m_xDibBytes = 0;
    m_pWndArea = NULL;

    m_iResOn = m_iFrameOn = 0;

    /*
    int idsne = IDS_WORLD_NE;

    CString s;
    BOOL    ok = s = EnLoadStdString( IDS_WORLD_NE );

    HINSTANCE hRes = AfxGetResourceHandle( );
    TRACE( "Resource handle: %p\n", hRes );

    TRACE( "ID=%d ok=%d str='%s'\n", IDS_WORLD_NE, ok, s );
    */

    // moved to oncreate
    /*
    m_sDir[0] = EnLoadStdString( IDS_WORLD_NE );
    m_sDir[1] = EnLoadStdString(IDS_WORLD_SE);
    m_sDir[2] = EnLoadStdString(IDS_WORLD_SW);
    m_sDir[3] = EnLoadStdString(IDS_WORLD_NW);
       */
}

void CWndWorld::Close() {

    delete m_pdibGround0;
    delete m_pdibBase;
    delete m_pdibRadar;
    delete[] m_piRadarEdges;
    delete m_pdibButtons;
    m_pdibGround0 = NULL;
    m_pdibBase = NULL;
    m_pdibRadar = NULL;
    m_piRadarEdges = NULL;
    m_pdibButtons = NULL;
}

void CWndWorld::InvalidateWindow(int iMode) {

    if ((iMode & m_iMode) == 0)
        return;

    // no need to rerender vehicles if no command center
    if (!m_bIsRadar)
        if ((iMode & (my_units | other_units)) == 0)
            return;

    m_bUpdate = TRUE;
}

void CWndWorld::ApplyColors(CDIB const *pDib) {


    if (pDib == NULL)
        return;

    for (int iOn = 0; iOn < CHex::num_types; iOn++) {
        m_clrTerrain[iOn]      = pDib->GetColorValue(m_rgbTerrain[iOn]);
        m_clrTerrainPaper[iOn] = pDib->GetColorValue(m_rgbTerrainPaper[iOn]);
    }
    m_clrLocation = pDib->GetColorValue(m_rgbLocation);
    for (int iOn = 0; iOn < 4; iOn++) {
        m_clrResources[iOn] = pDib->GetColorValue(m_rgbResources[iOn]);
        m_clrResHigh[iOn] = pDib->GetColorValue(m_rgbResHigh[iOn]);
    }
    m_clrLocation = pDib->GetColorValue(m_rgbLocation);
    m_clrHit = pDib->GetColorValue(m_rgbHit);
}

const DWORD dwStyleWorldWnd = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

// this is, just the map?? ( think so, the radar window and main map?)
// Map and Radar windows
void CWndWorld::Create(BOOL bStart) {

#ifdef LOGGINGON
    // Print a message with the cx and cy values
    char buf[128];
    sprintf_s( buf, sizeof( buf ), "CWndWorld::Create (Start=%d)\n", bStart );
    OutputDebugStringA( buf );
#endif


    m_bRBtnDown = FALSE;
    m_bLBtnDown = FALSE;
    m_bRCmdDown = FALSE;
    m_bCapMouse = FALSE;

    // the area window must already exist
    ASSERT_STRICT (theAreaList.GetTop() != NULL);
    NewAreaMap(theAreaList.GetTop());

    
    if ( m_sDir[0].empty( ) )
    {
        m_sDir[0] = EnLoadStdString( IDS_WORLD_NE );
        m_sDir[1] = EnLoadStdString( IDS_WORLD_SE );
        m_sDir[2] = EnLoadStdString( IDS_WORLD_SW );
        m_sDir[3] = EnLoadStdString( IDS_WORLD_NW );
    }

    // get min size
    // Radar mode BEFORE you land (rocket not yet placed) and again once you own a command
    // center; the parchment world map shows only in between (landed, no command center yet).
    m_bIsRadar = theGame.GetMe()->GetExists(CStructureData::command_center) ||
                 !theGame.GetMe()->m_bPlacedRocket;
    std::string sTitle = strPrintf(
        EnLoadStdString(m_bIsRadar ? IDS_WORLD_TITLE_RADAR : IDS_WORLD_TITLE_MAP).c_str(),
        m_pWndArea == NULL ? "" : m_sDir[m_pWndArea->GetAA().m_iDir].c_str());

    // World window (so it can have a cross-hair
    LPCTSTR sClass = CConquerApp::EnRegisterWndClass("EnWorldWnd", dwStyleWorldWnd,
                                                     theApp.LoadStandardCursor(IDC_CROSS));

    // Default window size. Both the world map and the radar render square art
    // (288x288, the world is CGameMap m_eX == m_eY) stretched to fill the client,
    // so a square *client* keeps the map undistorted (the legacy screenX/5 x
    // screenY/4 default is only ~square at 4:3 and looked squished on widescreen).
    // BUT the detached window also has our title bar on top, which makes a square
    // client read as too tall overall. Shorten the client ~8% so the whole window
    // (client + title bar + frame) looks balanced; the resulting slight map
    // stretch is small and matches the proportions players are used to.
    int iWorldClientW = theApp.m_iCol1 + 1;          // client width
    int iWorldClientH = ( iWorldClientW * 92 ) / 100; // ~8% shorter than square
    int iWorldDefEX = iWorldClientW + 2 * ::GetSystemMetrics( SM_CXSIZEFRAME );
    int iWorldDefEY = iWorldClientH + ::GetSystemMetrics( SM_CYCAPTION ) +
                      2 * ::GetSystemMetrics( SM_CYSIZEFRAME );

    // if it crashes here, i think a gfx bitmap is missing?
    // theApp.m_pMainWnd->m_hWnd is wrong, i think
    if ( CreateEx( 0, sClass, sTitle.c_str(), dwPopWndStyle, EnGetProfileInt( theApp.m_sResIni.c_str(), "WorldX", 0 ),
                   EnGetProfileInt( theApp.m_sResIni.c_str(), "WorldY", 0 ),
                   EnGetProfileInt( theApp.m_sResIni.c_str(), "WorldEX", iWorldDefEX ),
                   EnGetProfileInt( theApp.m_sResIni.c_str(), "WorldEY", iWorldDefEY ),
                   theApp.m_pMainWnd->m_hWnd, // window parent!
                   NULL, NULL ) == 0 )
    {
        throw( ERR_RES_CREATE_WND );
    }

    if ( bStart & ( !( m_iMode & visible ) ) )
    {
#ifdef LOGGINGON
        OutputDebugStringA( "CWndWorld set OnVisible\n" );
#endif
        OnVisible( );
    }

    // init vars
    NewMode();

    // save for file save
    if (theGame.m_wpWorld.length == 0) {
        theGame.m_wpWorld.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(&(theGame.m_wpWorld));
    }

    // draw it
    ReRender();
}

BEGIN_MESSAGE_MAP(CWndWorld, CWndAnim)
                    //{{AFX_MSG_MAP(CWndWorld)
                    ON_WM_PAINT()
                    ON_WM_DESTROY()
                    ON_WM_SIZE()
                    ON_WM_MOVE()
                    ON_WM_CREATE()
                    ON_WM_SYSCOMMAND()
                    ON_COMMAND(IDA_ENEMY, OnUnits)
                    ON_COMMAND(IDA_RESOURSES, OnRes)
                    ON_COMMAND(IDA_UNITS, OnMine)
                    ON_COMMAND(IDA_VISIBLE, OnVisible)
                    ON_COMMAND(IDA_CLOSE_WIN, OnCloseWin)
                    ON_WM_LBUTTONDOWN()
                    ON_WM_LBUTTONUP()
                    ON_WM_RBUTTONDOWN()
                    ON_WM_RBUTTONUP()
                    ON_WM_MOUSEMOVE()
                    ON_WM_SETCURSOR()
                    ON_WM_ERASEBKGND()
                    ON_WM_LBUTTONDBLCLK()
                    ON_WM_GETMINMAXINFO()
                    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndWorld message handlers

void CWndWorld::OnRes() {

    if (m_iMode & resources)
        m_iMode &= ~resources;
    else
        m_iMode |= resources;

    NewMode();
}

void CWndWorld::OnMine() {

    if (m_iMode & my_units)
        m_iMode &= ~my_units;
    else
        m_iMode |= my_units;

    NewMode();
}

void CWndWorld::OnUnits() {

    if (m_iMode & other_units)
        m_iMode &= ~other_units;
    else
        m_iMode |= other_units;

    NewMode();
}

void CWndWorld::OnVisible() {

    if (m_iMode & visible)
        m_iMode &= ~visible;
    else
        m_iMode |= visible;

    m_iResOn = m_iFrameOn = 0;
    NewMode();
}

void CWndWorld::OnCloseWin() {

    ShowWindow(SW_HIDE);
}

void CWndWorld::OnSysCommand(UINT nID, LPARAM lParam) {

    // for minimize hide it - only if last
    if (theAreaList.GetCount() <= 1)
        if ((nID == SC_MINIMIZE) || (nID == SC_CLOSE)) {
            ShowWindow(SW_HIDE);
            return;
        }

    CWndAnim::OnSysCommand(nID, lParam);
}

int CWndWorld::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    
#ifdef LOGGINGON
    OutputDebugStringA( "CWndWorld::OnCreate\n" );
#endif

    if ( CWndAnim::OnCreate( lpCreateStruct ) == -1 )
    {
        return -1;
    }

    // accelerators for this window
    m_hAccel = ::LoadAccelerators(theApp.m_hInstance, MAKEINTRESOURCE (IDR_WORLD));

    // we had to start with an icon to get a different class
    ::SetClassLongPtr(m_hWnd, GCLP_HCURSOR, NULL);

    m_sDir[0] = EnLoadStdString( IDS_WORLD_NE );
    m_sDir[1] = EnLoadStdString( IDS_WORLD_SE );
    m_sDir[2] = EnLoadStdString( IDS_WORLD_SW );
    m_sDir[3] = EnLoadStdString( IDS_WORLD_NW );

    m_sHelpRMB = EnLoadStdString(IDH_WORLD_WIN_RMB);
    m_sHelp = EnLoadStdString(IDH_WORLD_WIN);
    m_sHelpBtn[pos_res] = EnLoadStdString(IDH_WORLD_RES);
    m_sHelpBtn[pos_vis] = EnLoadStdString(IDH_WORLD_VIS);
    m_sHelpBtn[pos_mine] = EnLoadStdString(IDH_WORLD_OWNER);
    m_sHelpBtn[pos_units] = EnLoadStdString(IDH_WORLD_UNITS);
    m_sHelpBtnDis[pos_res] = EnLoadStdString(IDH_WORLD_RES2);
    m_sHelpBtnDis[pos_vis] = EnLoadStdString(IDH_WORLD_VIS2);
    m_sHelpBtnDis[pos_mine] = EnLoadStdString(IDH_WORLD_OWNER2);
    m_sHelpBtnDis[pos_units] = EnLoadStdString(IDH_WORLD_UNITS2);

    // Radar mode BEFORE you land (rocket not yet placed) and again once you own a command
    // center; the parchment world map shows only in between (landed, no command center yet).
    m_bIsRadar = theGame.GetMe()->GetExists(CStructureData::command_center) ||
                 !theGame.GetMe()->m_bPlacedRocket;
    if (m_bIsRadar)
        m_sHelpFace = "";
    else
        m_sHelpFace = EnLoadStdString(IDH_WORLD_FACE);

    m_hCurArrow = theApp.LoadStandardCursor(IDC_ARROW);
    m_hCurCross = theApp.LoadCursor(IDC_WORLD);
    m_hCurGoto = theApp.LoadCursor(IDC_GOTO3);
    m_hCurTarget = theApp.LoadCursor(IDC_TARGET3);
    m_hCurSelect = theApp.LoadCursor(IDC_SELECT3);
    m_hCurMove = theApp.LoadStandardCursor(IDC_SIZEALL);

    if (m_bIsRadar)
        m_iMode = my_units | other_units | visible;
    else
        m_iMode = my_units | other_units | resources;
    CommandCenterChange();

    ASSERT_STRICT(ptrthebltformat.Value());

    CRect rect;
    GetClientRect(&rect);
    // create the world CDIB

#ifdef LOGGINGON
    {
       // char buf[512];
       // sprintf_s( buf, "CWndWorld::Init called" );// with Format=%d, Type=%d, Direction=%d, Width=%d, Height=%d%s\n",
                //   ptrthebltformat->GetColorFormat( ), ptrthebltformat->GetType( ), ptrthebltformat->GetDirection( ),
                //   rect.Width( ), rect.Height( ), m_bIsRadar ? " (radar)" : "" );
        OutputDebugStringA( "CWndWorld::Init called" );
    }
#endif

    m_dibwnd.Init(this->m_hWnd,
                  new CDIB(ptrthebltformat->GetColorFormat(),
                           ptrthebltformat->GetType(),
                           ptrthebltformat->GetDirection()),
                  rect.Width(),
                  rect.Height());

    // Create SDL2 panel for this world/radar window
    if ( theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        CRect screenRect;
        GetWindowRect( &screenRect );
        const char* name = m_bIsRadar ? "radar" : "world";
        // Ensure title bar is on screen (at least TITLE_BAR_HT from top)
        int panelX = screenRect.left;
        int panelY = screenRect.top;
        // Leave room for resize borders and title bar at screen edges
        if (panelX < SDL2Panel::RESIZE_BORDER)
            panelX = SDL2Panel::RESIZE_BORDER;
        int minY = SDL2Panel::TITLE_BAR_HT + SDL2Panel::RESIZE_BORDER;
        if (panelY < minY)
            panelY = minY;
        m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(
            name, panelX, panelY,
            rect.Width(), rect.Height(), 20 );  // z=20, above area maps
        m_sdlPanel->SetMovable(true);
        m_sdlPanel->SetResizable(true);
        m_sdlPanel->SetTitle(m_bIsRadar ? "Radar" : "World Map");

        // When panel is resized, update the world's DIB and internal data
        CWndWorld* pResize = this;
        m_sdlPanel->SetResizeCallback(
            [pResize](int newW, int newH) {
                // Trigger the MFC size handler which rebuilds internal bitmaps
                pResize->m_dibwnd.Size( MAKELPARAM(newW, newH) );
                pResize->_OnSize();
            });

        // Keep the hidden MFC stub window glued under the visible SDL panel so
        // ScreenToClient-based selection (OnLButtonDown band-box, ::GetCursorPos
        // reads) stays aligned wherever the panel is dragged. See area.cpp for
        // the rationale.
        {
            HWND hMfc = m_hWnd;
            SDL2Panel* pPanel = m_sdlPanel;
            pPanel->SetMoveCallback(
                [hMfc, pPanel](int x, int y, int w, int h) {
                    int sx, sy;
                    if ( pPanel->IsDetached() && pPanel->GetOwnWindow() ) {
                        int wx = 0, wy = 0;
                        SDL_GetWindowPosition( pPanel->GetOwnWindow(), &wx, &wy );
                        sx = wx;
                        sy = wy + pPanel->GetTitleBarHeight();
                    } else {
                        sx = x; sy = y;
                        if ( theApp.m_gameWindow && theApp.m_gameWindow->GetWindow() ) {
                            int wx = 0, wy = 0;
                            SDL_GetWindowPosition( theApp.m_gameWindow->GetWindow(), &wx, &wy );
                            sx += wx; sy += wy;
                        }
                    }
                    // Align the MFC CLIENT rect (what ScreenToClient measures)
                    // to the content origin, compensating for caption/frame.
                    RECT wr, cr; POINT tl = { 0, 0 };
                    ::GetWindowRect( hMfc, &wr );
                    ::GetClientRect( hMfc, &cr );
                    ::ClientToScreen( hMfc, &tl );
                    int ncL = tl.x - wr.left;
                    int ncT = tl.y - wr.top;
                    int ncW = ( wr.right - wr.left ) - ( cr.right - cr.left );
                    int ncH = ( wr.bottom - wr.top ) - ( cr.bottom - cr.top );
                    ::SetWindowPos( hMfc, NULL, sx - ncL, sy - ncT, w + ncW, h + ncH,
                                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
                });
            pPanel->InvokeMoveCallback();
        }

        // SDL2-only renderer now: the MFC stub HWND has no visible role. Hide
        // it from the desktop so the compositor-managed SDL panel (with its
        // own green title bar) is the only visible window. WS_EX_TRANSPARENT
        // keeps any stray hit-testing click-through.
        ::SetWindowLong( m_hWnd, GWL_EXSTYLE,
            ::GetWindowLong( m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED | WS_EX_TRANSPARENT );
        ::SetLayeredWindowAttributes( m_hWnd, 0, 0, LWA_ALPHA );
        ::ShowWindow( m_hWnd, SW_HIDE );

        // Route SDL events to CWndWorld's handler methods
        CWndWorld* pThis = this;
        m_sdlPanel->SetEventCallback(
            [pThis](SDL_Event& event, int localX, int localY) -> bool {
                // Build MFC-style modifier flags
                UINT flags = 0;
                Uint32 ms = SDL_GetMouseState(nullptr, nullptr);
                if (ms & SDL_BUTTON_LMASK) flags |= MK_LBUTTON;
                if (ms & SDL_BUTTON_RMASK) flags |= MK_RBUTTON;
                SDL_Keymod km = SDL_GetModState();
                if (km & KMOD_SHIFT) flags |= MK_SHIFT;
                if (km & KMOD_CTRL)  flags |= MK_CONTROL;

                CPoint pt(localX, localY);

                switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        pThis->OnLButtonDown(flags, pt);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        pThis->OnRButtonDown(flags, pt);
                    else if (event.button.button == SDL_BUTTON_MIDDLE)
                        pThis->OnMButtonDown(flags, pt);
                    return true;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        pThis->OnLButtonUp(flags, pt);
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        pThis->OnRButtonUp(flags, pt);
                    else if (event.button.button == SDL_BUTTON_MIDDLE)
                        pThis->OnMButtonUp(flags, pt);
                    return true;
                case SDL_MOUSEMOTION:
                    pThis->OnMouseMove(flags, pt);
                    pThis->SetMouseState();
                    return true;
                }
                return false;
            });

        // Give the world/radar map its own borderless OS window (purple chrome)
        // so it can be dragged onto any monitor; the move-callback keeps the
        // hidden MFC window aligned for selection.
        m_sdlPanel->Detach( theApp.m_gameWindow.get() );

        // The close [X] hides the window (matches CWndWorld::OnCloseWin); the
        // World icon in the status bar (CWndBar::GotoWorld) brings it back.
        m_sdlPanel->SetClosable(true);
    }

    m_bUpdate = TRUE;
    m_iResOn = m_iFrameOn = 0;

    // animate us
    theAnimList.push_front(this);

#ifdef LOGGINGON
    if ( m_bIsRadar )
    {
        OutputDebugStringA( "CWndWorld::Created (radar) \n" );
    }
    else
    {
        OutputDebugStringA( "CWndWorld::Created \n" );
    }
#endif

    return 0;
}

void CWndWorld::CommandCenterChange() {

    int iOldMode = m_iMode;

    BOOL bOldRadar = m_bIsRadar;

    // Radar mode BEFORE you land (rocket not yet placed) and again once you own a command
    // center; the parchment world map shows only in between (landed, no command center yet).
    m_bIsRadar = theGame.GetMe()->GetExists(CStructureData::command_center) ||
                 !theGame.GetMe()->m_bPlacedRocket;
    if (!m_bIsRadar)
        SetButtonState(1, disabled);
    else {
        for (int iOn = 0; iOn < 4; iOn++)
            if (GetButtonState(iOn) == disabled) {
                SetButtonState(iOn, up);
                switch (iOn) {
                    case pos_res :
                        OnRes();
                        break;
                    case pos_vis :
                        OnVisible();
                        break;
                    case pos_mine :
                        OnMine();
                        break;
                    case pos_units :
                        OnUnits();
                        break;
                }
            }
    }

    // if closed we're done
    if (m_hWnd == NULL) {
        TRAP();
        return;
    }

    if (bOldRadar != m_bIsRadar) {
        _OnSize();
        // The throttled minimap cache (m_pdibRadarStatic) still holds the PREVIOUS mode's
        // image — e.g. the parchment world map right after a command center is built. Force
        // a full rebuild on the next ReRender so the radar never flashes the world-map
        // palette (and vice-versa) during the throttle window.
        m_dwLastRadarDraw = 0;
    }

    if (iOldMode != m_iMode)
        NewMode();

    std::string sTitle;
    if (m_bIsRadar) {
        m_sHelpFace = "";
        sTitle = EnLoadStdString(IDS_WORLD_TITLE_RADAR);
    } else {
        m_sHelpFace = EnLoadStdString(IDH_WORLD_FACE);
        sTitle = EnLoadStdString(IDS_WORLD_TITLE_MAP);
    }

    const char* pArg = (m_pWndArea == NULL) ? "" : m_sDir[m_pWndArea->GetAA().m_iDir].c_str();
    sTitle = strPrintf(sTitle.c_str(), pArg);

    SetWindowText(sTitle.c_str());
    // The SDL2 panel title bar doesn't hear SetWindowText — push the mode change
    // (Radar <-> World Map, with facing direction) to it too, or the panel keeps
    // whatever title it was created with (e.g. "Radar" after the rocket lands).
    if (m_sdlPanel)
        m_sdlPanel->SetTitle(sTitle.c_str());
    // BUGBUG - if change black it & do noise
}

void CWndWorld::OnPaint() {
    CPaintDC dc(this); // device context for painting

    m_dibwnd.Paint(dc.m_ps.rcPaint);

    // Do not call CWnd::OnPaint() for painting messages
}

void CWndWorld::Draw() {
    // Blit DIB content to SDL2 panel if available, else use GDI path
    if ( m_sdlPanel )
    {
        CDIB* pDib = m_dibwnd.GetDIB();
        if ( pDib )
        {
            int dibWidth  = pDib->GetWidth();
            int dibHeight = pDib->GetHeight();
            int bitsPerPixel  = pDib->GetBitsPerPixel();
            int bytesPerPixel = pDib->GetBytesPerPixel();
            int pitch     = pDib->GetPitch();

            if ( dibWidth > 0 && dibHeight > 0 && pitch > 0 )
            {
                CDIBits dibits = pDib->GetBits();
                BYTE* pPixels = (BYTE*)(dibits);
                if ( pPixels )
                {
                    SDL_Surface* panelSurface = m_sdlPanel->GetSurface();
                    if ( panelSurface )
                    {
#ifndef _WIN32
                        if ( getenv("EN_DIAG") ) {
                            static int n=0;
                            if (n++<2) {
                                size_t tot=(size_t)pitch*dibHeight, nz=0; unsigned mx=0;
                                for (size_t i=0;i<tot;++i){ if(pPixels[i]){++nz; if(pPixels[i]>mx)mx=pPixels[i];} }
                                fprintf(stderr,"[DIAG] radar/world DIB %dx%d bpp=%d Bpp=%d pitch=%d nonzero=%zu/%zu maxByte=%u radar=%d\n",
                                        dibWidth,dibHeight,bitsPerPixel,bytesPerPixel,pitch,nz,tot,mx,(int)m_bIsRadar);
                            }
                        }
#endif
                        Uint32 rmask = 0x00FF0000, gmask = 0x0000FF00, bmask = 0x000000FF, amask = 0;
                        SDL_Surface* dibSurf = SDL_CreateRGBSurfaceFrom(
                            pPixels, dibWidth, dibHeight, bitsPerPixel, pitch,
                            (bytesPerPixel >= 3) ? rmask : 0,
                            (bytesPerPixel >= 3) ? gmask : 0,
                            (bytesPerPixel >= 3) ? bmask : 0, amask );
                        if ( dibSurf )
                        {
                            SDL_BlitSurface( dibSurf, nullptr, panelSurface, nullptr );
                            SDL_FreeSurface( dibSurf );
                        }
                        m_sdlPanel->SetDirty();
                    }
                }
            }
        }
    }
    else
    {
        m_dibwnd.Update();
    }
}

void CWndWorld::OnDestroy() {

    // Remove SDL2 panel from compositor
    if ( m_sdlPanel && theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        theApp.m_gameWindow->GetCompositor()->RemovePanel( m_sdlPanel );
        m_sdlPanel = nullptr;
    }

    CWndAnim::OnDestroy();
    Close();
    m_dibwnd.Exit();
}

#ifdef FIXIT
void CWndWorld::CoordOn (CPoint pt, CHexPos & pos) 
{

    // go to this location
    // everything is shown twice so we move everything to the top half of the window
    // and convert to map coords
    pt.x = (pt.x << theMap.GetSideShift ()) / m_cx;
    pt.y = (pt.y << theMap.GetSideShift ()) / m_cy;

    // we now add the point to the center (which is also in the UL corner)
    CWndArea * pWnd = theAreaList.GetTop ();
    if (pWnd == NULL)
        {
        ASSERT (FALSE);
        pos = CHexPos (0, 0);
        return;
        }

    pWnd->GetAA().SetCenter( CMapLoc( CHexCoord( pt.x, pt.y )));
}
#endif

void CWndWorld::OnLButtonDown(UINT, CPoint) {

    m_bLBtnDown = TRUE;
}

// returns which button (0-3), -1 == map, -2 == radar base
int CWndWorld::ButtonOn(CPoint const &pt) const {

    // is this in the screen?
    int *piLeft = m_piRadarEdges + pt.y * 2;
    if ((pt.x > *piLeft) && (pt.x < *(piLeft + 1)))
        return (-1);

    // find the button bitmap and UL corner to test
    // divide into 4ths to figure out which to test
    int x, y, xOff, yOff;
    BYTE *pBtn;
    int iWid = m_pdibButtons->GetWidth() / 2;
    int iHt = m_pdibButtons->GetHeight() / 4;
    int iFunc;

    if (pt.x < m_cx / 2) {
        x = m_xBtnUL;
        if (pt.y < m_cy / 2) {
            y = m_yBtnUL;
            xOff = m_iMode & resources ? iWid : 0;
            yOff = 0;
            iFunc = pos_res;
        } else {
            y = m_yBtnLR;
            xOff = m_iMode & my_units ? iWid : 0;
            yOff = 2;
            iFunc = pos_mine;
        }
    } else {
        x = m_xBtnLR;
        if (pt.y < m_cy / 2) {
            y = m_yBtnUL;
            xOff = m_iMode & visible ? iWid : 0;
            yOff = 1;
            iFunc = pos_vis;
        } else {
            y = m_yBtnLR;
            xOff = m_iMode & other_units ? iWid : 0;
            yOff = 3;
            iFunc = pos_units;
        }
    }

    // are we above/left of it?
    if ((pt.x < x) || (pt.y < y))
        return (-2);
    // below/right?
    if ((pt.x > x + iWid - 1) || (pt.y > y + iHt - 1))
        return (-2);

    // ok we're in the button - see if it's transparent
    CDIBits dibits = m_pdibButtons->GetBits();
    pBtn = dibits + m_pdibButtons->GetOffset(xOff + pt.x - x, yOff * iHt + pt.y - y);
    if (*pBtn == m_pdibButtons->GetColorValue(RGB (255, 0, 255)))
        return (-2);

    return (iFunc);
}

void CWndWorld::OnLButtonUp(UINT nFlags, CPoint pt) {

    if (!m_bLBtnDown)
        return;

    // sometimes the point is outside the window
    if ((pt.x < 0) || (pt.y < 0) || (pt.x >= m_cx) || (pt.y >= m_cy))
        return;

    m_bLBtnDown = FALSE;

    // is this a button press?
    int iFunc = ButtonOn(pt);
    if (iFunc == -2)
        return;

    // it's a button
    if (iFunc >= 0) {
        // call the function - if enabled
        if (GetButtonState(iFunc) != disabled)
            switch (iFunc) {
                case pos_res :
                    OnRes();
                    break;
                case pos_vis :
                    OnVisible();
                    break;
                case pos_mine :
                    OnMine();
                    break;
                case pos_units :
                    OnUnits();
                    break;
            }
        return;
    }

    // it's the map: left click = look there (center the area view) — the
    // send-units command moved to the right button (SendUnitsTo), matching
    // the area map's select-left / command-right split.
    if (m_pWndArea == NULL)
        return;

    int xc = ((pt.x - m_cx / 2) << theMap.GetSideShift()) / m_cx;
    int yc = ((pt.y - m_cy / 2) << theMap.GetSideShift()) / m_cy;

    m_pWndArea->GetAA().MoveCenterHexes(xc, yc);
    m_pWndArea->InvalidateWindow();
    NewLocation();
}

// send the area map's selected units to the map location under `pt` (attack if
// it lands on an enemy) — the command half of the original (1996) OnLButtonUp,
// now driven by the right button.
void CWndWorld::SendUnitsTo(UINT nFlags, CPoint pt) {

    // get the area window
    if (m_pWndArea == NULL)
        return;

    // bug out if nothing selected
    if (m_pWndArea->m_lstUnits.GetCount() <= 0)
        return;

    // Get maploc coords of cursor

    int x = 64 * (pt.x - m_cx / 2) * theMap.Get_eX() / m_cx;
    int y = 64 * (pt.y - m_cy / 2) * theMap.Get_eY() / m_cy;

    int X, Y;

    CAnimAtr &aa = m_pWndArea->GetAA();

    switch (aa.m_iDir) {
        case 0:
            X = x - y;
            Y = x + y;

            break;

        case 1:
            X = -x - y;
            Y = x - y;

            break;

        case 2:
            X = -x + y;
            Y = -x - y;

            break;

        case 3:
            X = x + y;
            Y = -x + y;

            break;
    }

    CMapLoc maplocCenter = aa.GetCenter();

    X += maplocCenter.x;
    Y += maplocCenter.y;

    // Convert to subhex

    CSubHex _sub(CMapLoc(X, Y));

    _sub.Wrap();

    CUnit *pUnitOn = theBuildingHex._GetBuilding(_sub);
    if (pUnitOn == NULL)
        pUnitOn = theVehicleHex._GetVehicle(_sub);
    ASSERT_VALID_OR_NULL (pUnitOn);

    // its attack if forced attack or enemy & not forced goto
    BOOL bAttk = FALSE;
    if ((nFlags & (MK_CONTROL | MK_SHIFT)) == (MK_CONTROL | MK_SHIFT))
        bAttk = TRUE;
    else if ((!(nFlags & MK_CONTROL)) && (pUnitOn != NULL) &&
             (pUnitOn->GetOwner()->GetRelations() >= RELATIONS_NEUTRAL))
        bAttk = TRUE;

    // spread out the dest
    int iDestRand = 0;
    CSubHex _subDest(_sub);
    if ((pUnitOn == NULL) && (m_pWndArea->m_lstUnits.GetCount() > 2)) {
        iDestRand = (int) sqrt((float) m_pWndArea->m_lstUnits.GetCount()) + 1;
        _subDest.x -= iDestRand;
        _subDest.y -= iDestRand;
        _subDest.Wrap();
        iDestRand *= 2;
    }

    POSITION pos;
    for (pos = m_pWndArea->m_lstUnits.GetHeadPosition(); pos != NULL;) {
        CUnit *pUnit = m_pWndArea->m_lstUnits.GetNext(pos);
        ASSERT_VALID (pUnit);

        // if it can attack & we are supposed to attack - we do it
        if ((bAttk) && (pUnit->GetFireRate() > 0))
            pUnit->MsgSetTarget(_sub);
        else if (pUnit->GetUnitType() == CUnit::vehicle) {
            CVehicle *pVeh = ((CVehicle *) pUnit);
            pVeh->SetEvent(CVehicle::none);

            CSubHex _subVeh(_subDest.x + RandNum(iDestRand), _subDest.y + RandNum(iDestRand));
            _subVeh.Wrap();
            int iCost = theMap.GetTerrainCost(_subVeh, _subVeh, 0, pVeh->GetData()->GetWheelType());
            if ((iCost == 0) || (iCost > theMap.GetTerrainCost(_sub, _sub, 0, pVeh->GetData()->GetWheelType()) * 2))
                _subVeh = _sub;
            pVeh->SetDest(_subVeh);
        }
    }
}

// RMB = command: send the selected units to the clicked map location (on RMB-up,
// like a normal click). The old RMB drag-scroll moved to the middle button.
void CWndWorld::OnRButtonDown(UINT, CPoint) {

    m_bRCmdDown = TRUE;
}

void CWndWorld::OnRButtonUp(UINT nFlags, CPoint pt) {

    if (!m_bRCmdDown)
        return;
    m_bRCmdDown = FALSE;

    // sometimes the point is outside the window
    if ((pt.x < 0) || (pt.y < 0) || (pt.x >= m_cx) || (pt.y >= m_cy))
        return;

    // only the map area takes commands (not the corner buttons / radar chrome)
    if (ButtonOn(pt) != -1)
        return;

    SendUnitsTo(nFlags, pt);
}

// MMB held = drag-scroll the area view (this was the RMB behavior pre-2026)
void CWndWorld::OnMButtonDown(UINT, CPoint pt) {
    if (m_pWndArea == NULL)
        return;

    m_bRBtnDown = TRUE;
    m_ptRMB = pt;

    CaptureMouse();
    theApp.m_wndBar.SetStatusText(1, m_sHelpRMB.c_str());

    // put up the move cursor
    ::SetCursor(m_hCurMove);

    NewLocation();
}

void CWndWorld::OnMButtonUp(UINT, CPoint) {

    m_bRBtnDown = FALSE;
    if (m_bCapMouse)
        ReleaseMouse();
    theApp.m_wndBar.SetStatusText(1, m_sHelp.c_str());

    NewLocation();
}

void CWndWorld::OnMouseMove(UINT nFlags, CPoint point) {

    // sometimes the point is outside the window
    if ((point.x < 0) || (point.y < 0) || (point.x >= m_cx) || (point.y >= m_cy))
        return;

    // if RMB then we scroll
    if (m_bRBtnDown) {
        if (m_pWndArea == NULL)
            return;

        // handle the scroll
        int x = ((point.x - m_ptRMB.x) * m_cx) >> theMap.GetSideShift();
        int y = ((point.y - m_ptRMB.y) * m_cy) >> theMap.GetSideShift();

        if ((x != 0) || (y != 0)) {
//			m_pWndArea->GetAA().MoveCenterPixels( x << 6, y << 6 );
            m_pWndArea->GetAA().MoveCenterPixels(x << 4, y << 4);
            ASSERT_STRICT_VALID_STRUCT (&(m_pWndArea->GetAA()));

            // paint it
            m_pWndArea->InvalidateWindow();

            // we've moved
            NewLocation();
        }
        m_ptRMB = point;
    }

    // handle help
    int iFunc = ButtonOn(point);
    if (iFunc == -2) {
        theApp.m_wndBar.SetStatusText(1, m_sHelpFace.c_str());
        return;
    }
    if (iFunc == -1) {
        if (m_bRBtnDown)
            theApp.m_wndBar.SetStatusText(1, m_sHelpRMB.c_str());
        else
            theApp.m_wndBar.SetStatusText(1, m_sHelp.c_str());
        return;
    }

    if ((!m_bIsRadar) || (GetButtonState(iFunc) == disabled))
        theApp.m_wndBar.SetStatusText(1, m_sHelpBtnDis[iFunc].c_str());
    else
        theApp.m_wndBar.SetStatusText(1, m_sHelpBtn[iFunc].c_str());
}

int CWndWorld::GetButtonState(int iBtn) const {

    // check for disabled
    if ((m_iMode & (0x10 << iBtn)) != 0)
        return (disabled);

    // check for down
    if ((m_iMode & (0x01 << iBtn)) != 0)
        return (down);

    return (up);
}

void CWndWorld::SetButtonState(int iBtn, int iState) {

    switch (iState) {
        case up :
            m_iMode &= ~(0x01 << iBtn);
            m_iMode &= ~(0x10 << iBtn);
            break;
        case down :
            TRAP();
            m_iMode |= 0x01 << iBtn;
            m_iMode &= ~(0x10 << iBtn);
            break;
        case disabled :
            m_iMode &= ~(0x01 << iBtn);
            m_iMode |= 0x10 << iBtn;
            break;
    }

    NewMode();
}


void CWndWorld::OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI) {

    CRect rect(0, 0, 32, 32);

    AdjustWindowRect(&rect, dwStyleWorldWnd, FALSE);

    // we limit how small it can be
    if (lpMMI->ptMinTrackSize.x < rect.Width())
        lpMMI->ptMinTrackSize.x = rect.Width();
    if (lpMMI->ptMinTrackSize.y < rect.Height())
        lpMMI->ptMinTrackSize.y = rect.Height();

    if (theApp.m_wndBar.IsCreated()) {
        CRect rect;
        theApp.m_wndBar.GetWindowRect(&rect);
        lpMMI->ptMaxTrackSize.y = __min (lpMMI->ptMaxTrackSize.y, rect.top);
        lpMMI->ptMaxSize.y = __min (lpMMI->ptMaxSize.y, rect.top);
    }

    CWndAnim::OnGetMinMaxInfo(lpMMI);
}

void CWndWorld::OnSize(UINT nType, int cx, int cy) {

    
#ifdef LOGGINGON
    // Print a message with the cx and cy values
    char buf[128];
    sprintf_s( buf, sizeof( buf ), "CWndWorld::OnSize called: cx=%d, cy=%d, type=%d, radar=%d\n", cx, cy, nType, m_bIsRadar );
    OutputDebugStringA( buf );
#endif

    CWndAnim::OnSize(nType, cx, cy);

    _OnSize();

    theGame.m_wpWorld.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&(theGame.m_wpWorld));
}

void CWndWorld::OnMove(int x, int y) {
    CWndAnim::OnMove(x, y);

    if ( m_sdlPanel && !m_sdlPanel->IsMovable() )
    {
        CRect screenRect;
        GetWindowRect( &screenRect );
        m_sdlPanel->SetPosition( screenRect.left, screenRect.top );
    }
}

void CWndWorld::_OnSize() {

#ifdef LOGGINGON
    OutputDebugStringA( "world _OnSize\n" );
#endif


    // can get called when there is no hWnd (command center change)
    if (m_hWnd == NULL) {

#ifdef LOGGINGON
        OutputDebugStringA( "no m_hWnd!\n" );
#endif
        TRAP();
        return;
    }

    // When the SDL panel is user-resizable it owns the dimensions;
    // otherwise fall back to the MFC client rect.
    int right, bottom;
    if ( m_sdlPanel && m_sdlPanel->IsResizable() )
    {
        right  = m_sdlPanel->GetWidth();
        bottom = m_sdlPanel->GetHeight();
    }
    else
    {
        CRect rect;
        GetClientRect(&rect);
        right  = rect.right;
        bottom = rect.bottom;

        // Sync SDL panel to MFC position/size
        if ( m_sdlPanel )
        {
            CRect screenRect;
            GetWindowRect( &screenRect );
            m_sdlPanel->SetRect( screenRect.left, screenRect.top, right, bottom );
        }
    }
    m_dibwnd.Size(MAKELPARAM(right, bottom));

#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "m_dibwnd.Size rect.right=%d rect.bottom=%d\n", rect.right, rect.bottom );
    OutputDebugStringA( buf );
#endif

    // we build a copy of the map here each time the size
    // changes so we can then BLT it up fast each frame
    delete m_pdibGround0;
    delete m_pdibBase;
    delete m_pdibRadar;
    delete[] m_piRadarEdges;
    delete m_pdibButtons;

    // m_pdibRadarStatic (the throttled minimap/radar bake cache) is size-dependent like the
    // DIBs above, but was MISSING from this resize-invalidation list. On a window resize it
    // kept its OLD dimensions while the window DIB grew, so ReRender's unit-dot erase fast
    // path indexed the stale-sized cache with new-size coords -> out-of-bounds memcpy -> AV
    // crash (BUGS #16: operator hit it resizing the minimap). Free + NULL it so the next bake
    // reallocates it at the new size (and bRebuildBg fires meanwhile, skipping the fast path
    // until the cache matches the window again). It's lazily reallocated (not new'd here), so
    // it MUST be NULLed, not just deleted.
    delete m_pdibRadarStatic;
    m_pdibRadarStatic = NULL;

    int iBytesPerPixel = m_dibwnd.GetDIB()->GetBytesPerPixel();

    m_cx = __max (1, right);
    m_cy = __max (1, bottom);
    m_pdibGround0 = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                             CBLTFormat::DIR_TOPDOWN, m_cx, m_cy);
    m_pdibBase = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                          CBLTFormat::DIR_TOPDOWN, m_cx, m_cy);

    m_cxLine = m_pdibGround0->GetDirPitch();
    m_lSizeBytes = m_pdibGround0->GetDirPitch() * bottom;

    // stretch the radar art over
    m_pdibRadar = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                           CBLTFormat::DIR_TOPDOWN, m_cx, m_cy);
    CDIB *pDibRadarBm = theBitmaps.GetByIndex(m_bIsRadar ? DIB_RADAR : DIB_WORLD);
    pDibRadarBm->StretchBlt(m_pdibRadar, m_pdibRadar->GetRect(), pDibRadarBm->GetRect());


    m_piRadarEdges = new int[m_cy * 2];

    int cx         = m_cx / 2;
    int cy         = m_cy / 2;
    int halfWidth  = m_cx / 2;
    int halfHeight = m_cy / 2;

    for ( int y = 0; y < m_cy; ++y )
    {
        float dy    = abs( y - cy );
        float ratio = dy / halfHeight;  // 0 at center, 1 at top/bottom

        int left  = static_cast<int>( cx - halfWidth * ( 1.0f - ratio ) );
        int right = static_cast<int>( cx + halfWidth * ( 1.0f - ratio ) );

        if ( left < 0 )
            left = 0;
        if ( right > m_cx )
            right = m_cx;

        m_piRadarEdges[y * 2]     = left;
        m_piRadarEdges[y * 2 + 1] = right;
    }


    // get the buttons
    CDIB *pDibBtnBm = theBitmaps.GetByIndex(m_bIsRadar ? DIB_RADAR_BUTTONS : DIB_WORLD_BUTTONS);
    m_xBtnUL = (BTN_X_OFF * m_pdibRadar->GetWidth()) / pDibRadarBm->GetWidth();
    m_yBtnUL = (BTN_Y_OFF * m_pdibRadar->GetHeight()) / pDibRadarBm->GetHeight();
    m_xBtnLR = ((pDibRadarBm->GetWidth() - BTN_X_OFF - pDibBtnBm->GetWidth() / 2) * m_pdibRadar->GetWidth()) /
               pDibRadarBm->GetWidth();
    m_yBtnLR = ((pDibRadarBm->GetHeight() - BTN_Y_OFF - pDibBtnBm->GetHeight() / 4) * m_pdibRadar->GetHeight()) /
               pDibRadarBm->GetHeight();

    m_pdibButtons = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                             CBLTFormat::DIR_TOPDOWN,
                             (((pDibBtnBm->GetWidth() * m_pdibRadar->GetWidth()) / pDibRadarBm->GetWidth()) + 1) & ~1,
                             (((pDibBtnBm->GetHeight() * m_pdibRadar->GetHeight()) / pDibRadarBm->GetHeight()) + 3) &
                             ~3);
    pDibBtnBm->StretchBlt(m_pdibButtons, m_pdibButtons->GetRect(), pDibBtnBm->GetRect());

    // this is here because we need it for when the window is created
    ApplyColors(m_pdibGround0);

    // vars used to make regular updates faster
    CDIB *pdib = m_dibwnd.GetDIB();
    m_yAdd = theMap.Get_eY() / m_cy;
    m_yRem = theMap.Get_eY() % m_cy;
    m_xAdd = theMap.Get_eX() / m_cx;
    m_xRem = theMap.Get_eX() % m_cx;

    NewDir();
}

typedef void (*SETPIXEL)(BYTE *pDest, DWORD dwClr);

static inline void SetPixel1(BYTE *pDib, DWORD dwClr) {

    *pDib = (BYTE) dwClr;
}

static inline void SetPixel2(BYTE *pDib, DWORD dwClr) {

    *((WORD *) pDib) = (WORD) dwClr;
}

static inline void SetPixel3(BYTE *pDib, DWORD dwClr) {

    *((DWORD *) pDib) &= 0xFF000000;
    *((DWORD *) pDib) |= dwClr;
}

static inline void SetPixel4(BYTE *pDib, DWORD dwClr) {

    *((DWORD *) pDib) = dwClr;
}

void CWndWorld::_NewDir() {

    m_bNewDir = FALSE;
    m_bBldgHit = FALSE;

    // we get called on CWndArea before we're ready
    if ((m_pdibGround0 == NULL) || (m_pWndArea == NULL))
        return;

    ASSERT_STRICT_VALID (this);
    ASSERT_STRICT (m_hWnd != NULL);
    ASSERT_STRICT (m_pWndArea->m_hWnd != NULL);

    // put up the new dir
    std::string sTitle = EnLoadStdString( m_bIsRadar ? IDS_WORLD_TITLE_RADAR : IDS_WORLD_TITLE_MAP );

#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "New Title: %s\n", sTitle.c_str() );
    OutputDebugStringA( buf );
#endif

    sTitle = strPrintf( sTitle.c_str(), m_sDir[m_pWndArea->GetAA().m_iDir].c_str() );
    SetWindowText(sTitle.c_str());

    int iBytesPerPixel = m_dibwnd.GetDIB()->GetBytesPerPixel();

    // this is quick & dirty - we grab every n'th tile

    // X,Y          - CHex on
    // xStrt, yStrt - X,Y for next y row
    // if (iOdd & 1) -> xStrt += aInc[0]; yStrt += aInc[1]
    //   else           xStrt += aInc[2]; yStrt += aInc[3]
    // X += aInc[4]; Y += aInc[5]
    int aInc[6];
    switch (m_pWndArea->GetAA().m_iDir) {
        case 0 :
            aInc[0] = aInc[3] = 0;
            aInc[1] = aInc[4] = aInc[5] = 1;
            aInc[2] = -1;
            break;
        case 1 :
            aInc[1] = aInc[2] = aInc[4] = -1;
            aInc[0] = aInc[3] = 0;
            aInc[5] = 1;
            break;
        case 2 :
            aInc[0] = aInc[3] = 0;
            aInc[1] = aInc[4] = aInc[5] = -1;
            aInc[2] = 1;
            break;
        case 3 :
            aInc[1] = aInc[2] = aInc[4] = 1;
            aInc[0] = aInc[3] = 0;
            aInc[5] = -1;
            break;
    }

    CHexCoord _hex(0, 0);
    CHexCoord _hexStrt(0, 0);
    int iOdd = 1;
    CDIBits dibits = m_pdibGround0->GetBits();
    BYTE *pDib = dibits;

    // *2 to get it to wrap at bottom
    int yAdd = m_yAdd;
    int yRem = m_yRem;
    int yAcc = 0;
    int xAdd = m_xAdd;
    int xRem = m_xRem;
    int iPad = m_cxLine - m_cx * iBytesPerPixel;

    // trick for pentium pipeline in tests below
    DWORD bdwUnits = (m_iMode & (my_units | other_units)) ? -1 : 0;
    DWORD bdwRes = (m_iMode & resources) ? -1 : 0;
    DWORD bdwCopper = (bdwRes & theGame.GetMe()->CanCopper()) ? -1 : 0;

    // The world map gets the parchment "paper navigation map" palette; the radar keeps
    // the satellite-style colors. Both fill m_pdibGround0 here, so just pick the table.
    const DWORD* pclrTerrain = m_bIsRadar ? m_clrTerrain : m_clrTerrainPaper;

    // TEMP DEBUG: resource-render diagnosis counters (feed the [_NewDir] readout below)
    int dbgFlagged = 0, dbgLookupOK = 0, dbgDrawn = 0;

    SETPIXEL fnSetPixel;
    switch (iBytesPerPixel) {
        case 1 :
            fnSetPixel = SetPixel1;
            break;
        case 2 :
            fnSetPixel = SetPixel2;
            break;
        case 3 :
            fnSetPixel = SetPixel3;
            break;
        case 4 :
            fnSetPixel = SetPixel4;
            break;
    }

    for (int y = 0; y < m_cy; y++) {
        ASSERT_STRICT (pDib == m_pdibGround0->GetBits() + iBytesPerPixel * (long) y * (long) m_cxLine);

        // these are the accumulators for a single row in m_pdibGround0
        // above for hexStrt because this is for THIS line
        int xAcc, _yAcc;
        if (m_pWndArea->GetAA().m_iDir & 1) {
            _yAcc = ((m_cy - yAcc) * m_cx) / m_cy;
            xAcc = (yAcc * m_cx) / m_cy;
        } else {
            xAcc = ((m_cy - yAcc) * m_cx) / m_cy;
            _yAcc = (yAcc * m_cx) / m_cy;
        }

        // inc to next
        int iSkip = yAdd;
        yAcc += yRem;
        if (yAcc >= m_cy) {
            iSkip++;
            yAcc -= m_cy;
        }

        _hexStrt.X() += iSkip * aInc[2];
        _hexStrt.Y() += iSkip * aInc[1];
        _hexStrt.Wrap();

        for (int x = 0; x < m_cx; x++) {
            // handle wrap
            _hex.Wrap();

            CHex *pHex = theMap._GetHex(_hex);
            DWORD dwClr;

            // show buildings
            if (bdwUnits & (pHex->GetUnits() & CHex::bldg)) {
                CBuilding *pBldg = theBuildingHex._GetBuilding(_hex);

                // do we do it
#ifdef _CHEAT
                if ((pBldg != NULL) && ((pBldg->IsVisible ()) || _bShowWorld))
#else
                if ((pBldg != NULL) && (pBldg->IsVisible()))
#endif

                    if (((pBldg->GetOwner()->IsMe()) && (m_iMode & my_units)) ||
                        ((!pBldg->GetOwner()->IsMe()) && (m_iMode & other_units))) {
                        if ((pBldg->GetOwner()->IsMe()) && (pBldg->m_iFrameHit != 0))
                            dwClr = m_clrHit;
                        else
                            dwClr = pBldg->GetOwner()->GetPalColor();
                        goto GotClr;
                    }
            }

            // show resources
            if (bdwRes & (pHex->GetUnits() & CHex::minerals)) {
                dbgFlagged++;
                CMinerals *pMn;
                if (theMinerals.Lookup(_hex, pMn)) {
                    dbgLookupOK++;

                    // VTFIXME
                    // this was (is!?) crfashing for loaded games - not sure why, perhaps the deserialization failed
                    /* if ( !pMn->IsValid( ) )
                    {
#ifdef LOGGINGON
                        OutputDebugStringA( "Invalid Minerals!\n" );
#endif
                        return;  // fuck idk, i think it doesn't deserialize correctly when loading
                    }*/

                    switch (pMn->GetType()) {
                        case CMaterialTypes::copper :
                            if (bdwCopper != 0) {
                                dwClr = m_clrResources[0];
                                break;
                            }
                            goto ShowTerrain;
                        case CMaterialTypes::oil :
                            dwClr = m_clrResources[1];
                            break;
                        case CMaterialTypes::coal :
                            dwClr = m_clrResources[2];
                            break;
                        case CMaterialTypes::iron :
                            dwClr = m_clrResources[3];
                            break;
                    }
                    dbgDrawn++;
                    goto GotClr;
                }
            }

            ShowTerrain:
            // show terrain
            dwClr = pclrTerrain[pHex->GetVisibleType()];

            GotClr:
            (*fnSetPixel)(pDib, dwClr);

            pDib += iBytesPerPixel;

            // inc to next
            int iSkip = xAdd;
            xAcc += xRem;
            if (xAcc >= m_cx) {
                iSkip++;
                xAcc -= m_cx;
            }
            _hex.X() += iSkip * aInc[4];

            // the Y dir is incremented just as much - it just had a different starting point
            iSkip = xAdd;
            _yAcc += xRem;
            if (_yAcc >= m_cx) {
                iSkip++;
                _yAcc -= m_cx;
            }
            _hex.Y() += iSkip * aInc[5];
        }

        pDib += iPad;
        _hex = _hexStrt;
    }
    ASSERT_STRICT (pDib == m_pdibGround0->GetBits() + m_lSizeBytes);

    {
        // Mineral-rendering health probe — a cheap diagnostic kept after the
        // save/load mineral bug (see project_cmap_serialize_noop_trap). minTotal
        // is theMinerals.GetCount(): the number that went to 0 in bugged loaded
        // saves. flagged-vs-lookupOK and lookupOK-vs-drawn isolate which stage
        // breaks if minerals ever regress. Emits via OutputDebugString only when
        // the readout CHANGES — prints once at load, again only if it shifts —
        // so there's no per-frame spam and no disk write.
        char dbgbuf[256];
        sprintf_s( dbgbuf, sizeof( dbgbuf ),
                   "[_NewDir] radar=%d mode=0x%X resBit=%d minTotal=%d | flagged=%d lookupOK=%d drawn=%d cx=%d cy=%d eX=%d eY=%d\n",
                   (int)m_bIsRadar, m_iMode, (m_iMode & resources) ? 1 : 0, (int)theMinerals.GetCount(),
                   dbgFlagged, dbgLookupOK, dbgDrawn, m_cx, m_cy, theMap.Get_eX(), theMap.Get_eY() );
        static char s_dbgLast[256] = "";
        if ( strcmp( dbgbuf, s_dbgLast ) != 0 )
        {
            strcpy_s( s_dbgLast, sizeof( s_dbgLast ), dbgbuf );
            OutputDebugStringA( dbgbuf );
        }
    }

    NewLocation();
}

void CWndWorld::NewAreaMap(CWndArea *pWnd) {

    // if no area map - delete us
    if ((m_pWndArea = pWnd) == NULL) {
        DestroyWindow();
        return;
    }

    NewDir();
}

void CWndWorld::_NewMode() {

    m_bNewMode = FALSE;

    NewDir();
}

void CWndWorld::_NewLocation( )
{

    ASSERT_STRICT_VALID( this );

    m_bNewLocation = FALSE;

    // we get called on CWndArea before we're ready
    if ( ( m_pdibGround0 == NULL ) || ( m_pWndArea == NULL ) )
        return;

    // area map stuff
    CHexCoord hexcoord( m_pWndArea->GetAA( ).GetCenter( ) );
    hexcoord.Wrap( );
    hexcoord.X( ) /= 2;
    hexcoord.Y( ) /= 2;

    // get the start position for the world bitmap
    int aMul[4];
    theMap.DirMult( m_pWndArea->GetAA( ).m_iDir, aMul );

    int xMap = theMap.WrapX( hexcoord.X( ) * aMul[0] + hexcoord.Y( ) * aMul[1] );
    int yMap = theMap.WrapY( hexcoord.X( ) * aMul[2] + hexcoord.Y( ) * aMul[3] );
    m_xDib   = ( xMap * m_cx ) >> theMap.GetSideShift( );
    m_yDib   = ( yMap * m_cy ) >> theMap.GetSideShift( );

    // got a GPF below and this is the only way I see how
    while ( m_xDib >= m_cx )
    {
        TRAP( );
        m_xDib -= m_cx;
    }
    while ( m_xDib < 0 )
    {
        TRAP( );
        m_xDib += m_cx;
    }
    while ( m_yDib >= m_cy )
    {
        TRAP( );
        m_yDib -= m_cy;
    }
    while ( m_yDib < 0 )
    {
        TRAP( );
        m_yDib += m_cy;
    }

    ASSERT_STRICT( ptrthebltformat.Value( ) );

    m_xDibBytes = m_xDib * ptrthebltformat->GetBytesPerPixel( );

    m_iLenBytes = ptrthebltformat->GetBytesPerPixel( ) * ( m_cx - m_xDib );

    // copy the already generated map centering what's in the area map
    // by definition the center is also in the UL corner (cause every spot is shown twice)
    CSubHex _subCen( m_pWndArea->GetAA( ).GetCenter( ) );

    {  // GG: New scope so CDIBits objects leave scope before TranBlt()
        CDIBits dibitsDest = m_pdibBase->GetBits( );
        BYTE*   pDest      = dibitsDest;

        CDIBits dibitsGr0 = m_pdibGround0->GetBits( );
        BYTE*   pSrc      = dibitsGr0 + m_pdibGround0->GetOffset( m_xDib, m_yDib );
        BYTE*   pMax      = dibitsGr0 + m_lSizeBytes;

        for ( int y = 0; y < m_cy; y++ )
        {
            ASSERT_STRICT( m_pdibBase->IsInRange( pDest, m_iLenBytes ) );
            ASSERT_STRICT( m_pdibBase->IsInRange( pDest + m_iLenBytes, m_xDibBytes ) );
            ASSERT_STRICT( m_pdibGround0->IsInRange( pSrc, m_iLenBytes ) );
            ASSERT_STRICT( m_pdibGround0->IsInRange( pSrc - m_xDibBytes, m_xDibBytes ) );

            memcpy( pDest, pSrc, m_iLenBytes );
            memcpy( pDest + m_iLenBytes, pSrc - m_xDibBytes, m_xDibBytes );

            pDest += m_pdibBase->GetDirPitch( );
            pSrc += m_pdibGround0->GetDirPitch( );

            if ( pSrc + m_iLenBytes >= pMax )
                pSrc = dibitsGr0 + m_pdibGround0->GetOffset( m_xDib, 0 );
        }
    }

    // draw radar over it
    m_pdibRadar->TranBlt( m_pdibBase, m_pdibBase->GetRect( ), CPoint( 0, 0 ) );

    // draw buttons over it
    int skipPixelTop  = 1;
    int skipPixelLeft   = 1;
    int bottomPixelSkip = 0;
    int iDown         = m_pdibButtons->GetWidth( ) / 2;

    int quarterHeight = m_pdibButtons->GetHeight( ) / 4;
    int halfWidth     = m_pdibButtons->GetWidth( ) / 2;

    // Top-left button
    CRect rect( skipPixelLeft, skipPixelTop, halfWidth, quarterHeight - bottomPixelSkip );
    rect.OffsetRect( m_xBtnUL, m_yBtnUL );
    m_pdibButtons->TranBlt( m_pdibBase, rect,
                            CPoint( ( m_iMode & resources ) ? iDown : 0, skipPixelTop ) + CPoint( skipPixelLeft, 0 ) );

    // Top-right button
    rect.OffsetRect( m_xBtnLR - m_xBtnUL, 0 );
    m_pdibButtons->TranBlt( m_pdibBase, rect,
                            CPoint( ( m_iMode & visible ) ? iDown : 0, quarterHeight + skipPixelTop ) +
                                CPoint( skipPixelLeft, 0 ) );

    // Bottom-right button
    rect.OffsetRect( 0, m_yBtnLR - m_yBtnUL );
    m_pdibButtons->TranBlt( m_pdibBase, rect,
                            CPoint( ( m_iMode & other_units ) ? iDown : 0, 3 * quarterHeight + skipPixelTop ) +
                                CPoint( skipPixelLeft, 0 ) );

    // Bottom-left button
    rect.OffsetRect( -m_xBtnLR + m_xBtnUL, 0 );
    m_pdibButtons->TranBlt( m_pdibBase, rect,
                            CPoint( ( m_iMode & my_units ) ? iDown : 0, 2 * quarterHeight + skipPixelTop ) +
                                CPoint( skipPixelLeft, 0 ) );
    m_bUpdate = TRUE;
}

void CWndWorld::ReRender( )
{
    // redraw whatever needs to be redrawn
    // (each split-counted: ri.rerender showed ~70ms/s unattributed under load — these
    // prologue redraws run BEFORE the rr.radar/rr.world scopes below)
    if ( m_bNewMode )
    { Perf::ScopeCounter _cm( "rr.nmode" ); _NewMode( ); }

    if ( m_bNewDir ||
         ( m_dwDirBakePending != 0 && timeGetTime( ) - m_dwLastDirBake >= 1500 ) )
    {
        Perf::ScopeCounter _cd( "rr.ndir" );
        m_dwDirBakePending = 0;
        m_dwLastDirBake    = timeGetTime( );
        _NewDir( );
    }

    if ( m_bNewLocation )
    { Perf::ScopeCounter _cl( "rr.nloc" ); _NewLocation( ); }

    // unit may have moved under (or been created)
    CPoint pt;
    ::GetCursorPos( &pt );
    if ( ::WindowFromPoint( pt ) == m_hWnd )
    {
        // make sure in client area
        ScreenToClient( &pt );
        if ( ( pt.x >= 0 ) && ( pt.y >= 0 ) )
        {
            CRect rect;
            GetClientRect( &rect );
         //   if ( ( pt.x < rect.right ) && ( pt.y < rect.bottom ) )
         //       OutputDebugString( "CWndWorld::ReRender: before SetMouseState\n" );
            SetMouseState( );
        }
    }

    // trick for pentium pipeline in tests below
    DWORD bdwUnits  = ( m_iMode & ( my_units | other_units ) ) ? -1 : 0;
    DWORD bdwRes    = ( m_iMode & resources ) ? -1 : 0;
    DWORD bdwVis    = ( m_iMode & visible ) ? -1 : 0;
    DWORD bdwCopper = ( bdwRes & theGame.GetMe( )->CanCopper( ) ) ? -1 : 0;

    // repaint resources
    if ( bdwRes )
    {
        // hold for 1/3 of a second
        m_iFrameOn += theGame.GetFramesElapsed( );
        if ( m_iFrameOn >= NUM_FRAMES_SHOW_RES )
        {
            m_bUpdate  = TRUE;
            m_iFrameOn = 0;
            m_iResOn++;
            if ( m_iResOn > 7 )
                m_iResOn = 0;
        }
    }

    // only repaint if dirty
    if ( !m_bUpdate )
        return;

    m_bUpdate = FALSE;

    ASSERT_STRICT_VALID( this );

    // we get called on CWndArea before we're ready
    if ( ( m_pdibGround0 == NULL ) || ( m_pWndArea == NULL ) )
        return;

    Perf::ScopeCounter _crr( m_bIsRadar ? "rr.radar" : "rr.world" );

    // RADAR root-cause perf fix. The minimap redraw below is a per-pixel CPU walk over the WHOLE
    // map (~18ms in Debug) — but everything it draws except moving units (terrain/buildings/
    // minerals/fog) is static or slow-changing. So:
    //   * bake the UNIT-FREE background on a ~7fps throttle into m_pdibRadarStatic, and
    //   * on every other frame DON'T re-walk and DON'T re-blit the whole DIB — instead keep the
    //     window DIB as-is and only ERASE last frame's unit dots (restore those few pixels from
    //     the cached background), then redraw the current dots. That's O(units), not O(map).
    // Units are plotted LIVE below (object-iterated + projected), so they stay smooth at full
    // rate while the heavy walk runs ~7fps. (Non-radar world map keeps the original full path.)
    DWORD dwRadarNow = timeGetTime( );
    // Throttle the expensive per-pixel WALK for BOTH radar and the world map. The walk
    // samples a map-sized source DIB, so on a 1024² map it's ~230ms (cache-miss bound) and
    // was ~85% of the render budget at 20 players. Cache its output in m_pdibRadarStatic and
    // re-walk only every N ms; between walks blit the cache (world map has no live unit dots,
    // so a plain blit is a complete frame). Radar re-walks fast (140ms, units live); the
    // world map overview changes slowly, so 800ms is imperceptible and ~3-4x cheaper.
    // INPUT-GATED on top of the throttle: the walk renders ground (m_pdibBase) +
    // buildings + minerals + fog-of-war + the resource-highlight cycle. Hash everything
    // those depend on; if NOTHING changed since the last bake, skip the walk entirely —
    // the cached background + live unit dots are still a complete, correct frame. On a
    // calm view this takes the radar's O(window-pixels) map sampling from ~3/s to ~0;
    // during exploration the fog generation counter naturally re-enables it. Unit dots
    // are NOT part of the bake (drawn live every frame), so they never gate.
    unsigned long long walkSig = 0;
    bool bCtrMoved = false;
    // #4 (minimap/world-map lag on pan/zoom): the world map was EXCLUDED from this
    // input-detection by the radar-only gate, so it never noticed a center/zoom/dir
    // change and only re-walked on the blind 1500ms timer = the reported 1-2s lag.
    // Run the detection for the world map too so an input delta fast-paths its re-bake
    // (the m_pdibRadarStatic cache + bake-state bookkeeping below is ALREADY maintained
    // for the world map). Radar branch is unchanged.
    if ( m_pWndArea != NULL )
    {
        extern unsigned g_enTerrainEditGen;   // defined in SDL2Terrain.cpp (runtime terrain edits)
        CMapLoc ctr = m_pWndArea->GetAA( ).GetCenter( );
        bCtrMoved = ( ctr.x != m_ptLastBakeCtr.x || ctr.y != m_ptLastBakeCtr.y );
        walkSig = ( (unsigned long long)g_enFogVisGen << 32 )
                ^ (unsigned long long)g_enTerrainEditGen
                ^ ( (unsigned long long)theBuildingMap.GetCount( ) << 12 )
                ^ ( (unsigned long long)(unsigned)m_iResOn << 24 )
                ^ ( (unsigned long long)(unsigned)m_iMode << 16 )                       // mode BUTTONS change what the bake draws
                ^ ( (unsigned long long)( m_pWndArea->GetAA( ).m_iZoom & 3 ) << 56 )    // zoom changes the view-rect box size
                ^ ( (unsigned long long)(unsigned)ctr.x << 40 )
                ^ ( (unsigned long long)(unsigned)ctr.y << 52 )
                ^ ( (unsigned long long)( m_pWndArea->GetAA( ).m_iDir & 3 ) << 60 )
                ^ 0x9E3779B97F4A7C15ull;   // non-zero so a fresh member (0) never matches
        // Mode-button presses and zoom changes must reflect IMMEDIATELY (user: "when I
        // press buttons it takes a long time to update... the black viewbox takes too
        // long"): they were missing from the sig entirely (skipped until an unrelated
        // input changed), and even in the sig they'd wait out the throttle. Treat them
        // like a moving centre: fast path.
        if ( !bCtrMoved && walkSig != m_qwLastWalkSig )
            bCtrMoved = true;   // any input delta -> 140ms cadence; the gate still skips no-change frames
    }
    // SCROLL-FOLLOW vs in-place throttle. The radar image is anchored to the area-view
    // CENTRE: scrolling shifts the whole background, but the LIVE unit dots are drawn at
    // CURRENT positions every frame — any bake latency makes the player's dots slide
    // against a stale background (user-reported; enemies/resources are IN the bake so
    // they stay coherent with it). While the centre is moving, re-bake at the original
    // 140ms cadence (never user-visible pre-gate); only IN-PLACE changes (fog ticks,
    // building count, highlight cycle) use the longer 320ms throttle.
    // World map: re-bake fast (140ms) WHILE the user is actively panning/zooming
    // (bCtrMoved = any center/zoom/dir/mode delta, set above), else the cheap 1500ms
    // idle cadence — the same responsive-on-input / cheap-when-idle split the radar uses
    // (#4). Bounds the expensive whole-map walk to the active-interaction window.
    const DWORD kWalkThrottle = m_bIsRadar ? ( bCtrMoved ? 140u : 320u )
                                           : ( bCtrMoved ? 140u : 1500u );
    bool  bRebuildBg = !m_pdibRadarStatic ||
                       ( ( dwRadarNow - m_dwLastRadarDraw >= kWalkThrottle ) &&
                         // hit-flash animation must keep re-baking while active
                         ( !m_bIsRadar || walkSig != m_qwLastWalkSig || m_bBldgHit ) );
    Perf::CounterAdd( m_bIsRadar ? ( bRebuildBg ? "rr.bg" : "rr.fast" ) : ( bRebuildBg ? "rr.world.n" : "rr.world.blit" ), 1 );

    if ( !m_bIsRadar && !bRebuildBg )
    {
        // WORLD-MAP FAST PATH: blit the cached unit-free background (no live dots to redraw).
        Perf::ScopeCounter _cb( "rr.world.blitms" );
        m_pdibRadarStatic->BitBlt( m_dibwnd.GetDIB( ), m_pdibRadarStatic->GetRect( ), CPoint( 0, 0 ) );
    }
    else if ( m_bIsRadar && !bRebuildBg )
    {
        // FAST PATH: window DIB still holds (cached background + last frame's dots). Erase only
        // the old dots by restoring their pixels from the unit-free cache; current dots drawn
        // below. No whole-map walk, no whole-DIB blit.
        Perf::ScopeCounter _ce( "rr.erase" );
        CDIB* pwin = m_dibwnd.GetDIB( );
        int   bpp  = pwin->GetBytesPerPixel( );
        BYTE* pw   = pwin->GetBits( )              + pwin->GetOffset( 0, 0 );
        BYTE* ps   = m_pdibRadarStatic->GetBits( ) + m_pdibRadarStatic->GetOffset( 0, 0 );
        // Each DIB has its OWN pitch (and possibly orientation: top-down vs bottom-up → opposite
        // pitch sign). Using the window's pitch to index the static buffer reads out of bounds
        // → AV. Index every DIB with its own pitch.
        int   pitW = pwin->GetDirPitch( );
        int   pitS = m_pdibRadarStatic->GetDirPitch( );
        for ( const CPoint& d : m_radarDots )
        {
            BYTE* w = pw + d.y * pitW + d.x * bpp;
            BYTE* s = ps + d.y * pitS + d.x * bpp;
            memcpy( w,           s,           bpp );   // centre + 4-neighbour plus (matches draw)
            memcpy( w - bpp,     s - bpp,     bpp );
            memcpy( w + bpp,     s + bpp,     bpp );
            memcpy( w - pitW,    s - pitS,    bpp );
            memcpy( w + pitW,    s + pitS,    bpp );
        }
        m_radarDots.clear( );
    }
    else
    {

    // put up everything except vehicles & visibility
    m_pdibBase->BitBlt( m_dibwnd.GetDIB( ), m_pdibBase->GetRect( ), CPoint( 0, 0 ) );

    CDIB* pdib = m_dibwnd.GetDIB( );

    // is it a radar or a map
    DWORD bRadar    = m_bIsRadar ? -1 : 0;
    // Radar vehicles are plotted LIVE below (object-iterated), NOT in this per-pixel walk, so
    // the cached background stays unit-free. Force off here.
    DWORD bdwRadUni = m_bIsRadar ? 0 : ( bRadar & bdwUnits );

    int iBytesPerPixel = m_dibwnd.GetDIB( )->GetBytesPerPixel( );

    // this is quick & dirty - we grab every n'th tile

    // X,Y          - CHex on
    // xStrt, yStrt - X,Y for next y row
    // if (iOdd & 1) -> xStrt += aInc[0]; yStrt += aInc[1]
    //   else           xStrt += aInc[2]; yStrt += aInc[3]
    // X += aInc[4]; Y += aInc[5]
    int aInc[6];
    switch ( m_pWndArea->GetAA( ).m_iDir )
    {
    case 0:
        aInc[0] = aInc[3] = 0;
        aInc[1] = aInc[4] = aInc[5] = 1;
        aInc[2]                     = -1;
        break;
    case 1:
        aInc[1] = aInc[2] = aInc[4] = -1;
        aInc[0] = aInc[3] = 0;
        aInc[5]           = 1;
        break;
    case 2:
        aInc[0] = aInc[3] = 0;
        aInc[1] = aInc[4] = aInc[5] = -1;
        aInc[2]                     = 1;
        break;
    case 3:
        aInc[1] = aInc[2] = aInc[4] = 1;
        aInc[0] = aInc[3] = 0;
        aInc[5]           = -1;
        break;
    }

    CHexCoord hexcoord( m_pWndArea->GetAA( ).GetCenter( ) );


#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "MapCoors X=%d Y=%d\n", hexcoord.X( ), hexcoord.Y( ) );
    OutputDebugStringA( buf );
#endif

    CSubHex _sub( hexcoord );
    CSubHex _subStrt( _sub );
    int     iOdd = 1;

    // * 2 cause sub-hex
    int yAdd = m_yAdd * 2;
    int yRem = m_yRem * 2;
    if ( yRem >= m_cy )
    {
        yAdd++;
        yRem -= m_cy;
    }
    int yAcc = 0;
    int xAdd = m_xAdd * 2;
    int xRem = m_xRem * 2;
    if ( xRem >= m_cx )
    {
        xAdd++;
        xRem -= m_cx;
    }

    // Phase 6 Stage 4: DDraw-specific surface-missing check removed
    // (DIB_DIRECTDRAW and HasDDSurface are gone). The CDIB always has a
    // valid backing if GetBits()/GetOffset() are about to be called.

    // dest is where we write, radar tells us if we should write
    BYTE *  pDibDest, *pDibDestLine;
    CDIBits dibits = pdib->GetBits( );
    pDibDest = pDibDestLine = dibits + pdib->GetOffset( 0, 0 );
    int iDestPitch          = pdib->GetDirPitch( );

    int* piEdge = m_piRadarEdges;

    SETPIXEL fnSetPixel;
    switch ( iBytesPerPixel )
    {
    case 1:
        fnSetPixel = SetPixel1;
        break;
    case 2:
        fnSetPixel = SetPixel2;
        break;
    case 3:
        fnSetPixel = SetPixel3;
        break;
    case 4:
        fnSetPixel = SetPixel4;
        break;
    }

    // draw radar and fog stuff
    for ( int y = 0; y < m_cy; y++ )
    {

        // these are the accumulators for a single row in m_pdibGround0
        // above for hexStrt because this is for THIS line
        int xAcc, _yAcc;
        if ( m_pWndArea->GetAA( ).m_iDir & 1 )
        {
            _yAcc = ( ( m_cy - yAcc ) * m_cx ) / m_cy;
            xAcc  = ( yAcc * m_cx ) / m_cy;
        }
        else
        {
            xAcc  = ( ( m_cy - yAcc ) * m_cx ) / m_cy;
            _yAcc = ( yAcc * m_cx ) / m_cy;
        }

        // inc to next
        int iSkip = yAdd;
        yAcc += yRem;
        if ( yAcc >= m_cy )
        {
            iSkip++;
            yAcc -= m_cy;
        }

        _subStrt.x += iSkip * aInc[2];
        _subStrt.y += iSkip * aInc[1];
        _subStrt.Wrap( );

        // skip initial non-transparent
        int x = ( *piEdge++ ) + 1;

        pDibDest += x * iBytesPerPixel;

        // inc sub-hex to match
        int   iJmp  = xRem * x;
        int   iAdd  = xAdd * x;
        div_t dtNum = div( iJmp + xAcc, m_cx );
        xAcc        = dtNum.rem;
        _sub.x += ( iAdd + dtNum.quot ) * aInc[4];

        dtNum = div( iJmp + _yAcc, m_cx );
        _yAcc = dtNum.rem;
        _sub.y += ( iAdd + dtNum.quot ) * aInc[5];

        // cause we don't do first pixel
        if ( ( x < m_cx ) && ( bdwVis ) && ( !( ( x + y ) & 1 ) ) )
        {
            CHexCoord _hex( _sub );
            if ( !theMap.GetHex( _hex.X( ), _hex.Y( ) )->GetVisible( ) )
            {
                DWORD dwClr = 0;

                ( *fnSetPixel )( pDibDest - 1, dwClr );  // adds like a black bar around the edge of the radar
            }
        }

        // do the transparent part
        int iMax = ( *piEdge++ ) - 1;
        for ( ; x < iMax; x++ )
        {
            // handle wrap
            _sub.Wrap( );
            CHexCoord _hex( _sub );

            CHex*      pHex = theMap._GetHex( _hex );
            DWORD      dwClr;
            CBuilding* pBldg;

            // our buildings - never draw over those
            if ( pHex->GetUnits( ) & CHex::bldg )
            {
                pBldg = theBuildingHex._GetBuilding( _hex );
                if ( pBldg->GetOwner( )->IsMe( ) )
                {
                    // if it was hit we need to draw it (this may be the one that just went to 0)
                    if ( m_bBldgHit )
                    {
                        if ( pBldg->m_iFrameHit != 0 )
                            ( *fnSetPixel )( pDibDest, m_clrHit );
                        else
                            ( *fnSetPixel )( pDibDest, pBldg->GetOwner( )->GetPalColor( ) );
                    }
                    goto PixelDrawn;
                }
            }

            // show resources
            if ( bdwRes & ( pHex->GetUnits( ) & CHex::minerals ) )
            {
                CMinerals* pMn;
                if ( theMinerals.Lookup( _hex, pMn ) )
                {
                    switch ( pMn->GetType( ) )
                    {
                    case CMaterialTypes::copper:
                        if ( bdwCopper != 0 )
                        {
                            dwClr = m_iResOn == 0 ? m_clrResHigh[0] : m_clrResources[0];
                            break;
                        }
                        goto NotMinerals;
                    case CMaterialTypes::oil:
                        dwClr = m_iResOn == 2 ? m_clrResHigh[1] : m_clrResources[1];
                        break;
                    case CMaterialTypes::coal:
                        dwClr = m_iResOn == 4 ? m_clrResHigh[2] : m_clrResources[2];
                        break;
                    case CMaterialTypes::iron:
                        dwClr = m_iResOn == 6 ? m_clrResHigh[3] : m_clrResources[3];
                        break;
                    }
                    ( *fnSetPixel )( pDibDest, dwClr );  // actual draw of resources colours
                }
            }
            else
            {

            NotMinerals:;
                // visible (can only see vehicles on visible hexes)
                // cheat - visible only on if radar
                if ( bdwVis & ( !pHex->GetVisible( ) ) )
                {
                    // we can't draw over buildings that are visible (even though the hex isn't)
                    BOOL bOkDraw = TRUE;
                    if ( pHex->GetUnits( ) & CHex::bldg )
                    {
                        // got this above -- CBuilding * pBldg = theBuildingHex.GetBuilding (_hex);
                        if ( pBldg->IsVisible( ) )
                            bOkDraw = FALSE;
                    }

                    if ( bOkDraw && ( ( x + y ) & 1 ) )
                    {
                        ( *fnSetPixel )( pDibDest, 0 );  // This is the radar "fog of war" right here!!!
                    }
                }
                else

                    // show vehicles
                    if ( bdwRadUni & ( pHex->GetUnits( ) & CHex::veh ) )
                    {
                        CVehicle* pVeh = theVehicleHex._GetVehicle( _sub );

#ifdef _CHEAT
                        if ( ( pVeh != NULL ) && ( ( pVeh->IsVisible( ) && pHex->GetVisible( ) ) || _bShowWorld ) )
#else
                        if ( ( pVeh != NULL ) && pVeh->IsVisible( ) && pHex->GetVisible( ) )
#endif

                            if ( ( ( pVeh->GetOwner( )->IsMe( ) ) && ( m_iMode & my_units ) ) ||
                                 ( ( !pVeh->GetOwner( )->IsMe( ) ) && ( m_iMode & other_units ) ) )
                            {
                                if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( pVeh->m_iFrameHit != 0 ) )
                                {
                                    pVeh->m_iFrameHit -= theGame.GetFramesElapsed( );
                                    pVeh->m_iFrameHit = __max( 0, pVeh->m_iFrameHit );
                                    dwClr             = m_clrHit;
                                }
                                else
                                    dwClr = pVeh->GetOwner( )->GetPalColor( );
                                // we can do this because the radar screen means we are not on an edge
                                ( *fnSetPixel )( pDibDest - iDestPitch, dwClr );
                                ( *fnSetPixel )( pDibDest - 1, dwClr );
                                ( *fnSetPixel )( pDibDest, dwClr );
                                ( *fnSetPixel )( pDibDest + 1, dwClr );
                                ( *fnSetPixel )( pDibDest + iDestPitch, dwClr );
                            }
                    }
            }

        PixelDrawn:
            pDibDest += iBytesPerPixel;

            // inc to next
            int iSkip = xAdd;
            xAcc += xRem;
            if ( xAcc >= m_cx )
            {
                iSkip++;
                xAcc -= m_cx;
            }
            _sub.x += iSkip * aInc[4];

            iSkip = xAdd;
            _yAcc += xRem;
            if ( _yAcc >= m_cx )
            {
                iSkip++;
                _yAcc -= m_cx;
            }
            _sub.y += iSkip * aInc[5];
        }

        // cause we don't do last pixel
        if ( ( x < m_cx ) && ( bdwVis ) && ( ( x + y ) & 1 ) )
            if ( !theMap.GetHex( _sub )->GetVisible( ) )            
                ( *fnSetPixel )( pDibDest, 0 );  // adds like a black bar around the edge of the radar            

        pDibDest = ( pDibDestLine += iDestPitch );
        _sub     = _subStrt;
    }

#ifdef BUGBUG
    // each vehicle needs at least 1 pixel
    if ( m_bRadar & bdwUnits )
    {
        TRAP( );
        pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( pVeh->GetHexOwnership( ) )
            {
                if ( ( m_iMode & my_units ) && pVeh->GetOwner( )->IsMe( ) )
                {
                    TRAP( );
                }
                else if ( ( m_iMode & other_units ) && ( !pVeh->GetOwner( )->IsMe( ) ) )
                    if ( theMap.GetHex( pVeh->GetPtHead( ) )->GetVisible( ) )
                    {
                        TRAP( );
                    }
            }
        }
    }
#endif

    // draw a square showing the Area Window
    CRect rect;
    m_pWndArea->GetClientRect( &rect );

    int _iZoom    = m_pWndArea->GetAA( ).m_iZoom;
    int iWid      = ( m_cx * rect.Width( ) ) / ( CGameMap::HexWid( _iZoom ) << theMap.GetSideShift( ) ) + 2;
    int iWidBytes = ptrthebltformat->GetBytesPerPixel( ) * iWid;
    if ( iWid > m_cx )
        iWid = m_cx;
    int iHt = ( m_cy * rect.Height( ) ) / ( CGameMap::HexHt( _iZoom ) << theMap.GetSideShift( ) ) + 2;
    if ( iHt > m_cy )
        iHt = m_cy;

    BYTE* pDib = dibits + pdib->GetOffset( ( m_cx - iWid ) / 2, ( m_cy - iHt ) / 2 );
#ifdef BUGBUG
    if ( pdib->IsTopDown( ) )
        pDib = dibits + pdib->GetPitch( ) * ( m_cy - iHt ) / 2;
    else
        pDib = dibits + pdib->GetPitch( ) * ( m_cy - 1 - ( m_cy - iHt ) / 2 );
    pDib += ptrthebltformat->GetBytesPerPixel( ) * ( m_cx - iWid ) / 2;
#endif

    int iSkipBytes;

    iSkipBytes = pdib->GetDirPitch( ) - iWidBytes;

    iHt -= 2;
    if ( iHt < 0 )
        iHt = 0;

    // draw top of the viewbox (area showing what the main/selected window can see)
    for ( int iOn = 0; iOn < iWid; iOn++, pDib += iBytesPerPixel ) ( *fnSetPixel )( pDib, m_clrLocation );

    pDib += iSkipBytes;
    // Draw sides of the box
    while ( iHt-- )
    {
        ( *fnSetPixel )( pDib, m_clrLocation );

        pDib += iWidBytes;

        ( *fnSetPixel )( pDib, m_clrLocation );

        pDib += iSkipBytes;
    }

    // Draw bottom line of hte box
    for ( int iOn = 0; iOn < iWid; iOn++, pDib += iBytesPerPixel ) ( *fnSetPixel )( pDib, m_clrLocation );

    // when anyone zeros it'll get set again
    m_bBldgHit = NULL;

    // Cache this freshly-built UNIT-FREE background; the window DIB now holds the clean bg, so
    // there are no old dots to erase next frame. Done for the world map too now (its fast path
    // blits this cache instead of re-walking) — not just the radar.
    {
        if ( !m_pdibRadarStatic )
            m_pdibRadarStatic = new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY,
                                          CBLTFormat::DIR_TOPDOWN, m_cx, m_cy );
        m_dibwnd.GetDIB( )->BitBlt( m_pdibRadarStatic, m_dibwnd.GetDIB( )->GetRect( ), CPoint( 0, 0 ) );
        m_dwLastRadarDraw = dwRadarNow;
        m_qwLastWalkSig   = walkSig;   // inputs this bake rendered (skip-gate, see above)
        if ( m_pWndArea != NULL )      // centre this bake is anchored to (scroll-follow)
        {
            CMapLoc c = m_pWndArea->GetAA( ).GetCenter( );
            m_ptLastBakeCtr = CPoint( c.x, c.y );
        }
        m_radarDots.clear( );
    }
    }   // end else (full whole-map background walk)

    // --- LIVE radar unit dots: object-iterate vehicles and project each STRAIGHT to its radar
    //     pixel (the inverse of OnLButtonUp's pixel->map map). O(units), drawn over the persistent
    //     window DIB every frame; positions recorded so next frame can erase just these pixels. ---
    if ( m_bIsRadar && ( m_iMode & ( my_units | other_units ) ) )
    {
        Perf::ScopeCounter _cv( "rr.veh" );
        CDIB*    pwin  = m_dibwnd.GetDIB( );
        int      bppV  = pwin->GetBytesPerPixel( );
        BYTE*    baseV = pwin->GetBits( ) + pwin->GetOffset( 0, 0 );
        int      pitchV = pwin->GetDirPitch( );
        SETPIXEL fnSP  = bppV == 4 ? SetPixel4 : bppV == 3 ? SetPixel3 : bppV == 2 ? SetPixel2 : SetPixel1;

        CAnimAtr& aaV  = m_pWndArea->GetAA( );
        CMapLoc   cen  = aaV.GetCenter( );
        int       dirV = aaV.m_iDir;
        int       eX   = theMap.Get_eX( ), eY = theMap.Get_eY( );

        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( pVeh == NULL || pVeh->GetOwner( ) == NULL )
                continue;
            bool bMine = pVeh->GetOwner( )->IsMe( ) != 0;
            if ( bMine ? !( m_iMode & my_units ) : !( m_iMode & other_units ) )
                continue;
            if ( !pVeh->IsVisible( ) )
                continue;
            // CHECKED hex lookup (GetHex, not _GetHex): a vehicle on an out-of-range/edge hex
            // would otherwise return a bad pointer -> AV. Enemies only show on lit hexes.
            CHex* pHexV = theMap.GetHex( CHexCoord( pVeh->GetPtHead( ) ) );
            if ( pHexV == NULL )
                continue;
            if ( !bMine && !pHexV->GetVisible( ) )
                continue;

            // map loc -> radar pixel: subtract center, inverse-rotate by camera dir, scale.
            CMapLoc ml( pVeh->GetPtHead( ) );
            int X = ml.x - cen.x, Y = ml.y - cen.y, x, y;
            switch ( dirV )
            {
            case 0:  x = ( X + Y ) / 2; y = ( Y - X ) / 2;  break;
            case 1:  x = ( Y - X ) / 2; y = -( X + Y ) / 2; break;
            case 2:  x = -( X + Y ) / 2; y = ( X - Y ) / 2; break;
            default: x = ( X - Y ) / 2; y = ( X + Y ) / 2;  break;
            }
            int px = m_cx / 2 + ( eX ? ( x * m_cx ) / ( 64 * eX ) : 0 );
            int py = m_cy / 2 + ( eY ? ( y * m_cy ) / ( 64 * eY ) : 0 );
            if ( px < 1 || px >= m_cx - 1 || py < 1 || py >= m_cy - 1 )
                continue;

            DWORD clr;
            if ( bMine && pVeh->m_iFrameHit != 0 )
            {
                pVeh->m_iFrameHit = __max( 0, pVeh->m_iFrameHit - (int)theGame.GetFramesElapsed( ) );
                clr               = m_clrHit;
            }
            else
                clr = pVeh->GetOwner( )->GetPalColor( );

            BYTE* p = baseV + py * pitchV + px * bppV;
            ( *fnSP )( p, clr );
            ( *fnSP )( p - bppV, clr );
            ( *fnSP )( p + bppV, clr );
            ( *fnSP )( p - pitchV, clr );
            ( *fnSP )( p + pitchV, clr );
            m_radarDots.push_back( CPoint( px, py ) );   // for next-frame erase
        }
    }

    InvalidateRect(NULL, FALSE);
}


void CWndWorld::PaletteChange() {

    ApplyColors(m_pdibGround0);
    NewDir();
}

BOOL CWndWorld::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message) {

    if ((pWnd->GetSafeHwnd() != m_hWnd) || (nHitTest != HTCLIENT))
        return CWndAnim::OnSetCursor(pWnd, nHitTest, message);

    SetMouseState();
    return (TRUE);
}

void CWndWorld::SetMouseState() {

    // if move
    if (m_bRBtnDown) {
        ::SetCursor(m_hCurMove);
        return;
    }

    // get the cursor location
    CPoint pt = CPoint();
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    // are we outside the client area (happened once)
    // sometimes the point is outside the window
    if ((pt.x < 0) || (pt.y < 0) || (pt.x >= m_cx) || (pt.y >= m_cy))
        return;

    // if outside the radar screen it's an arrow
    int *piLeft = m_piRadarEdges + pt.y * 2;
    if ((pt.x < *piLeft) || (pt.x > *(piLeft + 1))) {
        ::SetCursor(m_hCurArrow);
        return;
    }

    // if nothing selected we can't do a goto or attack
    CWndArea *pWndArea = theAreaList.GetTop();
    if (pWndArea == NULL) {
        ::SetCursor(m_hCurCross);
        return;
    }
    if (pWndArea->m_lstUnits.GetCount() <= 0) {
        ::SetCursor(m_hCurCross);
        return;
    }

    // Get maploc coords of cursor
    int x = 64 * (pt.x - m_cx / 2) * theMap.Get_eX() / m_cx;
    int y = 64 * (pt.y - m_cy / 2) * theMap.Get_eY() / m_cy;

    int X = 0;
    int Y = 0;
    CAnimAtr &aa = m_pWndArea->GetAA();

    switch (aa.m_iDir) {
        case 0:
            X = x - y;
            Y = x + y;
            break;
        case 1:
            X = -x - y;
            Y = x - y;
            break;
        case 2:
            X = -x + y;
            Y = -x - y;
            break;
        case 3:
            X = x + y;
            Y = -x + y;
            break;
    }

    CMapLoc maplocCenter = aa.GetCenter();

    X += maplocCenter.x;
    Y += maplocCenter.y;

    // Convert to subhex
    auto point = CMapLoc(X, Y);
    CSubHex _sub( point );
    _sub.Wrap();

    // get building under
    CUnit *pUnitOn = theBuildingHex._GetBuilding(_sub);
    if (pUnitOn != NULL)
        if (!pUnitOn->IsVisible())
            pUnitOn = NULL;

    // veh only visible if hex is visible
    if (pUnitOn == NULL) {
        pUnitOn = theVehicleHex._GetVehicle(_sub);
        if (pUnitOn != NULL)
            if (!theMap._GetHex(_sub)->GetVisibility())
                pUnitOn = NULL;
    }
    ASSERT_STRICT_VALID_OR_NULL (pUnitOn);

    // its attack if forced attack or enemy & not forced goto
    int iShift = GetKeyState(VK_SHIFT) & ~1;
    int iCtrl = GetKeyState(VK_CONTROL) & ~1;
    BOOL bAttk = FALSE;
    if (iShift && iCtrl)
        bAttk = TRUE;
    else if ((!iCtrl) && (pUnitOn != NULL) && (pUnitOn->GetOwner()->GetRelations() >= RELATIONS_NEUTRAL))
        bAttk = TRUE;

    if (bAttk)
        ::SetCursor(m_hCurTarget);
    else
        ::SetCursor(m_hCurGoto);
}

BOOL CWndWorld::OnEraseBkgnd(CDC *) {

    return TRUE;
}

