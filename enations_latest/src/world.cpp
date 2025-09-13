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
#include "area.h"
#include "icons.h"
#include "bitmaps.h"

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
    BOOL    ok = s.LoadString( IDS_WORLD_NE );

    HINSTANCE hRes = AfxGetResourceHandle( );
    TRACE( "Resource handle: %p\n", hRes );

    TRACE( "ID=%d ok=%d str='%s'\n", IDS_WORLD_NE, ok, s );
    */

    // moved to oncreate
    /*
    m_sDir[0].LoadString( IDS_WORLD_NE );
    m_sDir[1].LoadString(IDS_WORLD_SE);
    m_sDir[2].LoadString(IDS_WORLD_SW);
    m_sDir[3].LoadString(IDS_WORLD_NW);
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

    for (int iOn = 0; iOn < CHex::num_types; iOn++)
        m_clrTerrain[iOn] = pDib->GetColorValue(m_rgbTerrain[iOn]);
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
    m_bCapMouse = FALSE;

    // the area window must already exist
    ASSERT_STRICT (theAreaList.GetTop() != NULL);
    NewAreaMap(theAreaList.GetTop());

    
    if ( m_sDir[0].IsEmpty( ) )
    {
        m_sDir[0].LoadString( IDS_WORLD_NE );
        m_sDir[1].LoadString( IDS_WORLD_SE );
        m_sDir[2].LoadString( IDS_WORLD_SW );
        m_sDir[3].LoadString( IDS_WORLD_NW );
    }

    // get min size
    CString sTitle, sClass;
    m_bIsRadar = theGame.GetMe()->GetExists(CStructureData::command_center);
    if (m_bIsRadar)
        sTitle.LoadString(IDS_WORLD_TITLE_RADAR);
    else
        sTitle.LoadString(IDS_WORLD_TITLE_MAP);

    if (m_pWndArea == NULL)
        csPrintf(&sTitle, (char const *) "");
    else
        csPrintf(&sTitle, (char const *) m_sDir[m_pWndArea->GetAA().m_iDir]);

    // World window (so it can have a cross-hair
    sClass = AfxRegisterWndClass(dwStyleWorldWnd, theApp.LoadStandardCursor(IDC_CROSS), 0, 0);

    // if it crashes here, i think a gfx bitmap is missing?
    // theApp.m_pMainWnd->m_hWnd is wrong, i think
    if ( CreateEx( 0, sClass, sTitle, dwPopWndStyle, theApp.GetProfileInt( theApp.m_sResIni, "WorldX", 0 ),
                   theApp.GetProfileInt( theApp.m_sResIni, "WorldY", 0 ),
                   theApp.GetProfileInt( theApp.m_sResIni, "WorldEX", theApp.m_iCol1 + 1 ),
                   theApp.GetProfileInt( theApp.m_sResIni, "WorldEY", theApp.m_iRow1 + 1 ), 
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
    //ReRender(); // don't draw it?
}

BEGIN_MESSAGE_MAP(CWndWorld, CWndAnim)
                    //{{AFX_MSG_MAP(CWndWorld)
                    ON_WM_PAINT()
                    ON_WM_DESTROY()
                    ON_WM_SIZE()
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
    ::SetClassLong(m_hWnd, GCL_HCURSOR, NULL);

    m_sDir[0].LoadString( IDS_WORLD_NE );
    m_sDir[1].LoadString( IDS_WORLD_SE );
    m_sDir[2].LoadString( IDS_WORLD_SW );
    m_sDir[3].LoadString( IDS_WORLD_NW );

    m_sHelpRMB.LoadString(IDH_WORLD_WIN_RMB);
    m_sHelp.LoadString(IDH_WORLD_WIN);
    m_sHelpBtn[pos_res].LoadString(IDH_WORLD_RES);
    m_sHelpBtn[pos_vis].LoadString(IDH_WORLD_VIS);
    m_sHelpBtn[pos_mine].LoadString(IDH_WORLD_OWNER);
    m_sHelpBtn[pos_units].LoadString(IDH_WORLD_UNITS);
    m_sHelpBtnDis[pos_res].LoadString(IDH_WORLD_RES2);
    m_sHelpBtnDis[pos_vis].LoadString(IDH_WORLD_VIS2);
    m_sHelpBtnDis[pos_mine].LoadString(IDH_WORLD_OWNER2);
    m_sHelpBtnDis[pos_units].LoadString(IDH_WORLD_UNITS2);

    m_bIsRadar = theGame.GetMe()->GetExists(CStructureData::command_center);
    if (m_bIsRadar)
        m_sHelpFace = "";
    else
        m_sHelpFace.LoadString(IDH_WORLD_FACE);

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

    // DO WE NEED TO SIZE IT?

    m_bUpdate = TRUE;
    m_iResOn = m_iFrameOn = 0;

    // animate us
    theAnimList.AddHead(this);

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

    m_bIsRadar = theGame.GetMe()->GetExists(CStructureData::command_center);
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

    if (bOldRadar != m_bIsRadar)
        _OnSize();

    if (iOldMode != m_iMode)
        NewMode();

    CString sTitle;
    if (m_bIsRadar) {
        m_sHelpFace = "";
        sTitle.LoadString(IDS_WORLD_TITLE_RADAR);
    } else {
        m_sHelpFace.LoadString(IDH_WORLD_FACE);
        sTitle.LoadString(IDS_WORLD_TITLE_MAP);
    }

    if (m_pWndArea == NULL)
        csPrintf(&sTitle, (char const *) "");
    else
        csPrintf(&sTitle, (char const *) m_sDir[m_pWndArea->GetAA().m_iDir]);

    SetWindowText(sTitle);
    // BUGBUG - if change black it & do noise
}

void CWndWorld::OnPaint() {
    CPaintDC dc(this); // device context for painting

    m_dibwnd.Paint(dc.m_ps.rcPaint);

    // Do not call CWnd::OnPaint() for painting messages
}

void CWndWorld::OnDestroy() {

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

void CWndWorld::OnRButtonDown(UINT, CPoint pt) {
    if (m_pWndArea == NULL)
        return;

    int x = ((pt.x - m_cx / 2) << theMap.GetSideShift()) / m_cx;
    int y = ((pt.y - m_cy / 2) << theMap.GetSideShift()) / m_cy;

    m_pWndArea->GetAA().MoveCenterHexes(x, y);

    m_pWndArea->InvalidateWindow();

    m_bRBtnDown = TRUE;
    m_ptRMB = pt;

    CaptureMouse();
    theApp.m_wndBar.SetStatusText(1, m_sHelpRMB);

    // put up the move cursor
    ::SetCursor(m_hCurMove);

    NewLocation();
}

void CWndWorld::OnRButtonUp(UINT, CPoint) {

    m_bRBtnDown = FALSE;
    if (m_bCapMouse)
        ReleaseMouse();
    theApp.m_wndBar.SetStatusText(1, m_sHelp);

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
        theApp.m_wndBar.SetStatusText(1, m_sHelpFace);
        return;
    }
    if (iFunc == -1) {
        if (m_bRBtnDown)
            theApp.m_wndBar.SetStatusText(1, m_sHelpRMB);
        else
            theApp.m_wndBar.SetStatusText(1, m_sHelp);
        return;
    }

    if ((!m_bIsRadar) || (GetButtonState(iFunc) == disabled))
        theApp.m_wndBar.SetStatusText(1, m_sHelpBtnDis[iFunc]);
    else
        theApp.m_wndBar.SetStatusText(1, m_sHelpBtn[iFunc]);
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

    if (theApp.m_wndBar.m_hWnd != NULL) {
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

    // we have to do this cause the button bar is in the cx, cy area
    // resize the WinG wnd
    CRect rect;
    GetClientRect(&rect);
    int right = rect.right;
    int bottom = rect.bottom;
    m_dibwnd.Size(MAKELPARAM (right, bottom)); // this SHOULD onsize the texture I think?

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

    int iBytesPerPixel = m_dibwnd.GetDIB()->GetBytesPerPixel();

    m_cx = __max (1, rect.right);
    m_cy = __max (1, rect.bottom);
    m_pdibGround0 = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                             CBLTFormat::DIR_TOPDOWN, m_cx, m_cy);
    m_pdibBase = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                          CBLTFormat::DIR_TOPDOWN, m_cx, m_cy);

    m_cxLine = m_pdibGround0->GetDirPitch();
    m_lSizeBytes = m_pdibGround0->GetDirPitch() * rect.bottom;

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
    CString sTitle;
    if (m_bIsRadar)
        sTitle.LoadString( IDS_WORLD_TITLE_RADAR );
    else
        sTitle.LoadString(IDS_WORLD_TITLE_MAP);

#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "New Title: %s\n", (LPCSTR) sTitle );
    OutputDebugStringA( buf );
#endif

    csPrintf(&sTitle, (char const *) m_sDir[m_pWndArea->GetAA().m_iDir]);
    SetWindowText(sTitle);

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
                CMinerals *pMn;
                if (theMinerals.Lookup(_hex, pMn)) {

                    // VTFIXME
                    // this was crfashing for loaded games - not sure why, perhaps the deserialization failed
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
                    goto GotClr;
                }
            }

            ShowTerrain:
            // show terrain
            dwClr = m_clrTerrain[pHex->GetVisibleType()];

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

void CWndWorld::_NewLocation() {

    ASSERT_STRICT_VALID (this);

    m_bNewLocation = FALSE;

    // we get called on CWndArea before we're ready
    if ((m_pdibGround0 == NULL) || (m_pWndArea == NULL))
        return;

    // area map stuff
    CHexCoord hexcoord(m_pWndArea->GetAA().GetCenter());
    hexcoord.Wrap();
    hexcoord.X() /= 2;
    hexcoord.Y() /= 2;

    // get the start position for the world bitmap
    int aMul[4];
    theMap.DirMult(m_pWndArea->GetAA().m_iDir, aMul);

    int xMap = theMap.WrapX(hexcoord.X() * aMul[0] + hexcoord.Y() * aMul[1]);
    int yMap = theMap.WrapY(hexcoord.X() * aMul[2] + hexcoord.Y() * aMul[3]);
    m_xDib = (xMap * m_cx) >> theMap.GetSideShift();
    m_yDib = (yMap * m_cy) >> theMap.GetSideShift();

    // got a GPF below and this is the only way I see how
    while (m_xDib >= m_cx) {
        TRAP();
        m_xDib -= m_cx;
    }
    while (m_xDib < 0) {
        TRAP();
        m_xDib += m_cx;
    }
    while (m_yDib >= m_cy) {
        TRAP();
        m_yDib -= m_cy;
    }
    while (m_yDib < 0) {
        TRAP();
        m_yDib += m_cy;
    }

    ASSERT_STRICT(ptrthebltformat.Value());

    m_xDibBytes = m_xDib * ptrthebltformat->GetBytesPerPixel();

    m_iLenBytes = ptrthebltformat->GetBytesPerPixel() * (m_cx - m_xDib);

    // copy the already generated map centering what's in the area map
    // by definition the center is also in the UL corner (cause every spot is shown twice)
    CSubHex _subCen(m_pWndArea->GetAA().GetCenter());

    { // GG: New scope so CDIBits objects leave scope before TranBlt()
        CDIBits dibitsDest = m_pdibBase->GetBits();
        BYTE *pDest = dibitsDest;

        CDIBits dibitsGr0 = m_pdibGround0->GetBits();
        BYTE *pSrc = dibitsGr0 + m_pdibGround0->GetOffset(m_xDib, m_yDib);
        BYTE *pMax = dibitsGr0 + m_lSizeBytes;

        for (int y = 0; y < m_cy; y++) {
            ASSERT_STRICT(m_pdibBase->IsInRange(pDest, m_iLenBytes));
            ASSERT_STRICT(m_pdibBase->IsInRange(pDest + m_iLenBytes, m_xDibBytes));
            ASSERT_STRICT(m_pdibGround0->IsInRange(pSrc, m_iLenBytes));
            ASSERT_STRICT(m_pdibGround0->IsInRange(pSrc - m_xDibBytes, m_xDibBytes));

            memcpy(pDest, pSrc, m_iLenBytes);
            memcpy(pDest + m_iLenBytes, pSrc - m_xDibBytes, m_xDibBytes);

            pDest += m_pdibBase->GetDirPitch();
            pSrc += m_pdibGround0->GetDirPitch();

            if (pSrc + m_iLenBytes >= pMax)
                pSrc = dibitsGr0 + m_pdibGround0->GetOffset(m_xDib, 0);
        }
    }
    // radar over it
    m_pdibRadar->TranBlt(m_pdibBase, m_pdibBase->GetRect(), CPoint(0, 0));

    // buttons over it
    int iDown = m_pdibButtons->GetWidth() / 2;
    CRect rect(0, 0, m_pdibButtons->GetWidth() / 2, m_pdibButtons->GetHeight() / 4);
    rect.OffsetRect(m_xBtnUL, m_yBtnUL);
    m_pdibButtons->TranBlt(m_pdibBase, rect, CPoint(m_iMode & resources ? iDown : 0, 0));
    rect.OffsetRect(m_xBtnLR - m_xBtnUL, 0);
    m_pdibButtons->TranBlt(m_pdibBase, rect, CPoint(m_iMode & visible ? iDown : 0, m_pdibButtons->GetHeight() / 4));
    rect.OffsetRect(0, m_yBtnLR - m_yBtnUL);
    m_pdibButtons->TranBlt(m_pdibBase, rect,
                           CPoint(m_iMode & other_units ? iDown : 0, (3 * m_pdibButtons->GetHeight()) / 4));
    rect.OffsetRect(-m_xBtnLR + m_xBtnUL, 0);
    m_pdibButtons->TranBlt(m_pdibBase, rect, CPoint(m_iMode & my_units ? iDown : 0, m_pdibButtons->GetHeight() / 2));

    m_bUpdate = TRUE;
}

void CWndWorld::ReRender() {

    // crashes on load game on start, idk why
    // this is a tmp workaround
    // VTFIXME
   // if ( theGame.LoadGame)

    // redraw whatever needs to be redrawn
    if (m_bNewMode)
        _NewMode();

    if (m_bNewDir)
        _NewDir();

    if (m_bNewLocation)
        _NewLocation();

    // unit may have moved under (or been created)
    CPoint pt;
    ::GetCursorPos(&pt);
    if (::WindowFromPoint(pt) == m_hWnd) {
        // make sure in client area
        ScreenToClient(&pt);
        if ((pt.x >= 0) && (pt.y >= 0)) {
            CRect rect;
            GetClientRect(&rect);
            if ((pt.x < rect.right) && (pt.y < rect.bottom))
                OutputDebugString("CWndWorld::ReRender: before SetMouseState\n");
                SetMouseState();
        }
    }

    // trick for pentium pipeline in tests below
    DWORD bdwUnits = (m_iMode & (my_units | other_units)) ? -1 : 0;
    DWORD bdwRes = (m_iMode & resources) ? -1 : 0;
    DWORD bdwVis = (m_iMode & visible) ? -1 : 0;
    DWORD bdwCopper = (bdwRes & theGame.GetMe()->CanCopper()) ? -1 : 0;

    // repaint resources
    if (bdwRes) {
        // hold for 1/3 of a second
        m_iFrameOn += theGame.GetFramesElapsed();
        if (m_iFrameOn >= NUM_FRAMES_SHOW_RES) {
            m_bUpdate = TRUE;
            m_iFrameOn = 0;
            m_iResOn++;
            if (m_iResOn > 7)
                m_iResOn = 0;
        }
    }

    // only repaint if dirty
    if (!m_bUpdate)
        return;
    m_bUpdate = FALSE;

    ASSERT_STRICT_VALID (this);

    // we get called on CWndArea before we're ready
    if ((m_pdibGround0 == NULL) || (m_pWndArea == NULL))
        return;

    // put up everything except vehicles & visibility
    m_pdibBase->BitBlt(m_dibwnd.GetDIB(), m_pdibBase->GetRect(), CPoint(0, 0));

    CDIB *pdib = m_dibwnd.GetDIB();

    // is it a radar or a map
    DWORD bRadar = m_bIsRadar ? -1 : 0;
    DWORD bdwRadUni = bRadar & bdwUnits;

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

    CHexCoord hexcoord(m_pWndArea->GetAA().GetCenter());

    
#ifdef LOGGINGON
    char    buf[128];
    sprintf_s( buf, "MapCoors X=%d Y=%d\n", hexcoord.X( ), hexcoord.Y( ) );
    OutputDebugStringA( buf );
#endif

    CSubHex _sub(hexcoord);
    CSubHex _subStrt(_sub);
    int iOdd = 1;

    // * 2 cause sub-hex
    int yAdd = m_yAdd * 2;
    int yRem = m_yRem * 2;
    if (yRem >= m_cy) {
        yAdd++;
        yRem -= m_cy;
    }
    int yAcc = 0;
    int xAdd = m_xAdd * 2;
    int xRem = m_xRem * 2;
    if (xRem >= m_cx) {
        xAdd++;
        xRem -= m_cx;
    }

    // if there's no DD device, just retunr?
    if ( !pdib->HasDDSurface( ) )
    {
#ifdef LOGGINGON
        int  type = pdib->GetType( );
        buf[128];
        sprintf_s( buf, "no HasDDSurface!! type=%d\n", type );
        OutputDebugStringA( buf );
#endif

        return;
    }

    // dest is where we write, radar tells us if we should write
    BYTE *pDibDest, *pDibDestLine;
    CDIBits dibits = pdib->GetBits(); // exception, gfx surface missing? GetDDSurface null
    pDibDest = pDibDestLine = dibits + pdib->GetOffset(0, 0);
    int iDestPitch = pdib->GetDirPitch();

    int *piEdge = m_piRadarEdges;

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

    // is this the (broken) radar?
    for (int y = 0; y < m_cy; y++) {

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

        _subStrt.x += iSkip * aInc[2];
        _subStrt.y += iSkip * aInc[1];
        _subStrt.Wrap();

        // skip initial non-transparent
        int x = (*piEdge++) + 1;

        pDibDest += x * iBytesPerPixel;

        // inc sub-hex to match
        int iJmp = xRem * x;
        int iAdd = xAdd * x;
        div_t dtNum = div(iJmp + xAcc, m_cx);
        xAcc = dtNum.rem;
        _sub.x += (iAdd + dtNum.quot) * aInc[4];

        dtNum = div(iJmp + _yAcc, m_cx);
        _yAcc = dtNum.rem;
        _sub.y += (iAdd + dtNum.quot) * aInc[5];

        // cause we don't do first pixel

        // cause we don't do first pixel
        if ((x < m_cx) && (bdwVis) && (!((x + y) & 1))) {
            CHexCoord _hex(_sub);
            if (!theMap.GetHex(_hex.X(), _hex.Y())->GetVisible()) {
                DWORD dwClr = 0;
                ( *fnSetPixel )( pDibDest - 1, dwClr ); // draw the radar "fog"
            }
        }

        // do the transparent part
        int iMax = (*piEdge++) - 1;
        for (; x < iMax; x++) {
            // handle wrap
            _sub.Wrap();
            CHexCoord _hex(_sub);

            CHex *pHex = theMap._GetHex(_hex);
            DWORD dwClr;
            CBuilding *pBldg;

            // our buildings - never draw over those
            if (pHex->GetUnits() & CHex::bldg) {
                pBldg = theBuildingHex._GetBuilding(_hex);
                if (pBldg->GetOwner()->IsMe()) {
                    // if it was hit we need to draw it (this may be the one that just went to 0)
                    if (m_bBldgHit) {
                        if (pBldg->m_iFrameHit != 0)
                            (*fnSetPixel)(pDibDest, m_clrHit);
                        else
                            (*fnSetPixel)(pDibDest, pBldg->GetOwner()->GetPalColor());
                    }
                    goto PixelDrawn;
                }
            }

            // show resources
            if (bdwRes & (pHex->GetUnits() & CHex::minerals)) {
                CMinerals *pMn;
                if (theMinerals.Lookup(_hex, pMn)) {
                    switch (pMn->GetType()) {
                        case CMaterialTypes::copper :
                            if (bdwCopper != 0) {
                                dwClr = m_iResOn == 0 ? m_clrResHigh[0] : m_clrResources[0];
                                break;
                            }
                            goto NotMinerals;
                        case CMaterialTypes::oil :
                            dwClr = m_iResOn == 2 ? m_clrResHigh[1] : m_clrResources[1];
                            break;
                        case CMaterialTypes::coal :
                            dwClr = m_iResOn == 4 ? m_clrResHigh[2] : m_clrResources[2];
                            break;
                        case CMaterialTypes::iron :
                            dwClr = m_iResOn == 6 ? m_clrResHigh[3] : m_clrResources[3];
                            break;
                    }
                    (*fnSetPixel)(pDibDest, dwClr);
                }
            } else {

                NotMinerals:;
                // visible (can only see vehicles on visible hexes)
                // cheat - visible only on if radar
                if (bdwVis & (!pHex->GetVisible())) {
                    // we can't draw over buildings that are visible (even though the hex isn't)
                    BOOL bOkDraw = TRUE;
                    if (pHex->GetUnits() & CHex::bldg) {
                        // got this above -- CBuilding * pBldg = theBuildingHex.GetBuilding (_hex);
                        if (pBldg->IsVisible())
                            bOkDraw = FALSE;
                    }

                    if (bOkDraw && ((x + y) & 1))
                        (*fnSetPixel)(pDibDest, 0);
                } else

                    // show vehicles
                if (bdwRadUni & (pHex->GetUnits() & CHex::veh)) {
                    CVehicle *pVeh = theVehicleHex._GetVehicle(_sub);

#ifdef _CHEAT
                    if ( (pVeh != NULL) && ( (pVeh->IsVisible () && pHex->GetVisible ()) || _bShowWorld ) )
#else
                    if ((pVeh != NULL) && pVeh->IsVisible() && pHex->GetVisible())
#endif

                        if (((pVeh->GetOwner()->IsMe()) && (m_iMode & my_units)) ||
                            ((!pVeh->GetOwner()->IsMe()) && (m_iMode & other_units))) {
                            if ((pVeh->GetOwner()->IsMe()) && (pVeh->m_iFrameHit != 0)) {
                                pVeh->m_iFrameHit -= theGame.GetFramesElapsed();
                                pVeh->m_iFrameHit = __max (0, pVeh->m_iFrameHit);
                                dwClr = m_clrHit;
                            } else
                                dwClr = pVeh->GetOwner()->GetPalColor();
                            // we can do this because the radar screen means we are not on an edge
                            (*fnSetPixel)(pDibDest - iDestPitch, dwClr);
                            (*fnSetPixel)(pDibDest - 1, dwClr);
                            (*fnSetPixel)(pDibDest, dwClr);
                            (*fnSetPixel)(pDibDest + 1, dwClr);
                            (*fnSetPixel)(pDibDest + iDestPitch, dwClr);
                        }
                }
            }

            PixelDrawn:
            pDibDest += iBytesPerPixel;

            // inc to next
            int iSkip = xAdd;
            xAcc += xRem;
            if (xAcc >= m_cx) {
                iSkip++;
                xAcc -= m_cx;
            }
            _sub.x += iSkip * aInc[4];

            iSkip = xAdd;
            _yAcc += xRem;
            if (_yAcc >= m_cx) {
                iSkip++;
                _yAcc -= m_cx;
            }
            _sub.y += iSkip * aInc[5];
        }

        // cause we don't do last pixel
        if ((x < m_cx) && (bdwVis) && ((x + y) & 1))
            if (!theMap.GetHex(_sub)->GetVisible())
                (*fnSetPixel)(pDibDest, 0); // draw radar fog again?

        pDibDest = (pDibDestLine += iDestPitch);
        _sub = _subStrt;
    }

#ifdef BUGBUG
    // each vehicle needs at least 1 pixel
    if ( m_bRadar & bdwUnits )
        {
        TRAP ();
        pos = theVehicleMap.GetStartPosition ();
        while (pos != NULL)
            {
            DWORD dwID;
            CVehicle *pVeh;
            theVehicleMap.GetNextAssoc (pos, dwID, pVeh);
            if ( pVeh->GetHexOwnership () )
                {
                if ( (m_iMode & my_units) && pVeh->GetOwner ()->IsMe () )
                    {
                    TRAP ();
                    }
                else
                    if ( (m_iMode & other_units) && ( ! pVeh->GetOwner ()->IsMe () ) )
                        if ( theMap.GetHex (pVeh->GetPtHead ())->GetVisible () )
                            {
                            TRAP ();
                            }
                }
            }
        }
#endif

    // draw a square showing the Area Window
    CRect rect;
    m_pWndArea->GetClientRect(&rect);

    int _iZoom = m_pWndArea->GetAA().m_iZoom;
    int iWid = (m_cx * rect.Width()) / (CGameMap::HexWid(_iZoom) << theMap.GetSideShift()) + 2;
    int iWidBytes = ptrthebltformat->GetBytesPerPixel() * iWid;
    if (iWid > m_cx)
        iWid = m_cx;
    int iHt = (m_cy * rect.Height()) / (CGameMap::HexHt(_iZoom) << theMap.GetSideShift()) + 2;
    if (iHt > m_cy)
        iHt = m_cy;

    BYTE *pDib = dibits + pdib->GetOffset((m_cx - iWid) / 2, (m_cy - iHt) / 2);
#ifdef BUGBUG
    if ( pdib->IsTopDown () )
        pDib = dibits + pdib->GetPitch() * (m_cy - iHt) / 2;
    else
        pDib = dibits + pdib->GetPitch() * ( m_cy - 1 - (m_cy - iHt) / 2);
    pDib += ptrthebltformat->GetBytesPerPixel() * (m_cx - iWid) / 2;
#endif

    int iSkipBytes;

    iSkipBytes = pdib->GetDirPitch() - iWidBytes;

    iHt -= 2;
    if (iHt < 0)
        iHt = 0;

    for (int iOn = 0; iOn < iWid; iOn++, pDib += iBytesPerPixel)
        (*fnSetPixel)(pDib, m_clrLocation);

    pDib += iSkipBytes;

    while (iHt--) {
        (*fnSetPixel)(pDib, m_clrLocation);

        pDib += iWidBytes;

        (*fnSetPixel)(pDib, m_clrLocation);

        pDib += iSkipBytes;
    }

    for (int iOn = 0; iOn < iWid; iOn++, pDib += iBytesPerPixel)
        (*fnSetPixel)(pDib, m_clrLocation);

    // when anyone zeros it'll get set again
    m_bBldgHit = NULL;

    InvalidateRect(NULL, FALSE);
}


void CWndWorld::PaletteChange() {

    ApplyColors(m_pdibGround0);
    NewDir();
}

BOOL CWndWorld::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message) {

    if ((pWnd != this) || (nHitTest != HTCLIENT))
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

