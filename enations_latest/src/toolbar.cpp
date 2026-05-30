//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// main.cpp : implementation file
//

#include "toolbar.h"

#include "area.h"
#include "bitmaps.h"
#include "building.inl"
#include "minerals.h"
#include "unit.h"
#include "event.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "SDL2FileDialog.h"
#include "SDL2GameDialogs.h"
#include "SDL2Panel.h"
#include "SDL2Toolbar.h"
#include "SDL2UnitList.h"
#include "lastplnt.h"
#include <SDL.h>


#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

// Static SDL2 unit list instances (persist across open/close)
static SDL2UnitList* s_sdlVehicleList = nullptr;
static SDL2UnitList* s_sdlBuildingList = nullptr;
#include "relation.h"
#include "stdafx.h"
#include "terrain.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


std::string CWndBar::m_sChat1;
std::string CWndBar::m_sChat2;
std::string CWndBar::m_sScience;
std::string CWndBar::m_sRelations;


/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat window

CWndStatLine::CWndStatLine( )
{

    m_fnStatus = NULL;
    m_pFnData  = NULL;
}

void CWndStatLine::SetText( char const* pText, CStatInst::IMPORTANCE iImp )
{

    // CWndStatBar::SetText won't Invalidate if it's identical to the old value (usually NULL)
    if ( m_fnStatus != NULL )
    {
        InvalidateRect( NULL );

        m_fnStatus = NULL;
        m_pFnData  = NULL;
    }

    CWndStatBar::SetText( pText, iImp );
}

void CWndStatLine::SetStatusFunc( FNSTATUSLINE fnStat, void* pData )
{

    // if the same - exit
    if ( ( fnStat == m_fnStatus ) && ( pData == m_pFnData ) )
        return;

    if ( ( fnStat == NULL ) || ( pData == NULL ) )
    {
        m_fnStatus = NULL;
        m_pFnData  = NULL;
        InvalidateRect( NULL );
        return;
    }

    // turn off text
    CWndStatBar::SetText( NULL );

    m_fnStatus = fnStat;
    m_pFnData  = pData;
    InvalidateRect( NULL );
}

BEGIN_MESSAGE_MAP( CWndStatLine, CWndStatBar )
//{{AFX_MSG_MAP(CWndStatLine)
ON_WM_PAINT( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

void CWndStatLine::OnPaint( )
{

    if ( m_fnStatus == NULL )
    {
        CWndStatBar::OnPaint( );
        return;
    }

    CPaintDC dc( this );
    thePal.Paint( dc.m_hDC );

    CRect rect;
    GetClientRect( &rect );

    // get the offset of the background
    CPoint pt( 0, 0 );
    ::MapWindowPoints( m_hWnd, GetParent( )->m_hWnd, &pt, 1 );
    ( *m_fnStatus )( m_pFnData, (CDC*)dc, rect, theBitmaps.GetByIndex( DIB_TOOLBAR ), pt );

    thePal.EndPaint( dc.m_hDC );
}


/////////////////////////////////////////////////////////////////////////////
// CWndBar

CWndBar::CWndBar( )
{
}

const int CWndBar::aID[NUM_BAR_BTNS]   = { IDC_BAR_AREA,     IDC_BAR_WORLD,     IDC_BAR_CHAT,    IDC_BAR_ADVISOR,
                                           IDC_BAR_VEHICLES, IDC_BAR_BUILDINGS, IDC_BAR_SCIENCE, IDC_BAR_FILE };
const int CWndBar::aBtn[NUM_BAR_BTNS]  = { 43, 17, 15, 31, 19, 18, 0, 27 };
const int CWndBar::aHelp[NUM_BAR_BTNS] = { IDH_BAR_AREA,     IDH_BAR_WORLD,     IDH_BAR_CHAT,    IDH_BAR_ADVISOR,
                                           IDH_BAR_VEHICLES, IDH_BAR_BUILDINGS, IDH_BAR_SCIENCE, IDH_BAR_FILE };

void fnMouseMove( CWndStub* pWnd, UINT nFlags, int x, int y )
{

    theApp.m_wndBar.SetStatusText( 1, "" );
}

void CWndBar::Create( )
{
    // load the strings
    m_sChat1 = EnLoadStdString( IDS_NO_CHAT1 );
    m_sChat2 = EnLoadStdString( IDS_NO_CHAT2 );
    m_sScience = EnLoadStdString( IDS_NO_SCIENCE );
    m_sRelations = EnLoadStdString( IDS_NO_EMBASSY );

    // Bottom of the main window — no Windows-taskbar dodging since the bar
    // lives inside the SDL2 main window's client area now.
    CRect rect;
    theApp.m_pMainWnd->GetClientRect( &rect );
    theApp.m_iRow3 = rect.Height( ) - TOOLBAR_HT;

    // Phase A.1: no MFC HWND. Create the SDL panel + SDL2Toolbar directly,
    // skip the legacy CBmButton / CWndStatBar / CWndStatLine children — they
    // were invisible (parent layered alpha=1) and SDL2Toolbar already does
    // all rendering, hit-testing, hover state, and animation.
    if ( theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(
            "toolbar", 0, theApp.m_iRow3, rect.Width(), TOOLBAR_HT, 30 );

        static SDL2Toolbar s_toolbar;
        s_toolbar.Init( m_sdlPanel, theApp.m_gameWindow.get() );
        theApp.m_gameWindow->SetSDL2Toolbar( &s_toolbar );

        CWndBar* pThis = this;
        s_toolbar.SetButtonHandler(0, [pThis]() { pThis->GotoArea(); });
        s_toolbar.SetButtonHandler(1, [pThis]() { pThis->GotoWorld(); });
        s_toolbar.SetButtonHandler(2, [pThis]() { pThis->GotoChat(); });
        s_toolbar.SetButtonHandler(3, [pThis]() { pThis->GotoRelations(); });
        s_toolbar.SetButtonHandler(4, [pThis]() { pThis->GotoVehicles(); });
        s_toolbar.SetButtonHandler(5, [pThis]() { pThis->GotoBuildings(); });
        s_toolbar.SetButtonHandler(6, [pThis]() { pThis->GotoScience(); });
        s_toolbar.SetButtonHandler(7, [pThis]() { pThis->GotoFile(); });

        // Populate the sentence-length hover text from the legacy IDH_BAR_*
        // string resources (matches what the original MFC toolbar showed in
        // the status strip when the cursor hovered a button).
        for ( int i = 0; i < NUM_BAR_BTNS; i++ )
            s_toolbar.SetButtonHelpText( i, EnLoadStdString( aHelp[ i ] ) );

        if ( !theGame.IsNetGame() )
            s_toolbar.EnableButton(2, false);

        m_sdlPanel->SetEventCallback(
            []( SDL_Event& event, int localX, int localY ) -> bool {
                SDL2Toolbar* tb = theApp.m_gameWindow->GetSDL2Toolbar();
                if ( tb ) return tb->HandleEvent( event, localX, localY );
                return false;
            });
    }
}

// --- Geometry / visibility shadows (route to SDL panel post-collapse) -------

BOOL CWndBar::SetWindowPos( HWND /*hwndAfter*/, int x, int y, int cx, int cy, UINT flags )
{
    if ( !m_sdlPanel )
        return FALSE;
    if ( !( flags & SWP_NOMOVE ) )
        m_sdlPanel->SetPosition( x, y );
    if ( !( flags & SWP_NOSIZE ) )
        m_sdlPanel->SetSize( cx, cy );
    return TRUE;
}

BOOL CWndBar::ShowWindow( int nCmdShow )
{
    if ( !m_sdlPanel )
        return FALSE;
    m_sdlPanel->SetVisible( nCmdShow != SW_HIDE );
    return TRUE;
}

BOOL CWndBar::GetWindowRect( RECT* pRect ) const
{
    if ( !m_sdlPanel || !pRect )
        return FALSE;
    // SDL panel coords are screen-relative inside the main window (which is
    // borderless fullscreen), so x/y already match screen coords.
    pRect->left   = m_sdlPanel->GetX();
    pRect->top    = m_sdlPanel->GetY();
    pRect->right  = pRect->left + m_sdlPanel->GetWidth();
    pRect->bottom = pRect->top + m_sdlPanel->GetHeight();
    return TRUE;
}

BOOL CWndBar::DestroyWindow()
{
    // MFC path: real HWND → ::DestroyWindow → WM_DESTROY → OnDestroy() →
    // panel removal. Delegate to the base implementation.
    if ( m_hWnd )
        return CWndStub::DestroyWindow();

    // Collapsed-panel mode (no MFC HWND): OnDestroy would never fire on its
    // own because CWndStub::DestroyWindow short-circuits when m_hWnd is NULL.
    // Call OnDestroy directly so the SDL panel and the SDL2Toolbar reference
    // on GameWindow get cleaned up.
    OnDestroy();
    if ( theApp.m_gameWindow )
        theApp.m_gameWindow->SetSDL2Toolbar( nullptr );
    return TRUE;
}

int CWndBar::OnCreate( LPCREATESTRUCT lpCS )
{

    if ( CWndAnim::OnCreate( lpCS ) == -1 )
        return -1;

    // create the buttons
    CRect rect( BAR_BTN_X_SKIP, BAR_BTN_Y_START, BAR_BTN_X_SKIP + theBmBtnData.Width( ),
                BAR_BTN_Y_START + theBmBtnData.Height( ) );

    CBmButton* pBtn = m_BmBtns;
    for ( int iOn = 0; iOn < NUM_BAR_BTNS; iOn++, pBtn++ )
    {
        pBtn->Create( aBtn[iOn], aHelp[iOn], &theBmBtnData, rect, theBitmaps.GetByIndex( DIB_TOOLBAR ), this,
                      aID[iOn] );
        rect.OffsetRect( theBmBtnData.Width( ) + BAR_BTN_X_SKIP, 0 );
    }
    rect.left += BAR_BTN_X_SKIP;

    // status bars
    // time goes on the right
    CStatData* pSb = theIcons.GetByIndex( ICON_CLOCK );
    CRect      rTime;
    GetClientRect( &rTime );
    rTime.right -= BAR_BTN_X_SKIP;
    rTime.top    = rect.top;
    rTime.bottom = rect.bottom;

    // we size this based on fitting the text in
    CClientDC dc( this );
    CFont*    pOld = NULL;
    if ( pSb->m_pFnt != NULL )
        pOld = dc.SelectObject( pSb->m_pFnt );
    CRect rText( rTime );
    dc.DrawText( "999:99:99", -1, rText, DT_CALCRECT | DT_RIGHT | DT_SINGLELINE | DT_VCENTER );
    rTime.left = rTime.right - rText.Width( ) - pSb->m_leftOff - pSb->m_rightOff;

    m_wndTime.Create( &theIcons, ICON_CLOCK, rTime, this, theBitmaps.GetByIndex( DIB_TOOLBAR ) );
    m_wndTime.SetText( "0:00:00" );

    // rect now goes from last btn + skip to time (4 * status + skip)
    int aiStat[4] = { ICON_GAS, ICON_POWER, ICON_PEOPLE, ICON_FOOD };
    rect.right    = rTime.left;
    int iWid      = ( rect.Width( ) / 4 ) - BAR_BTN_X_SKIP;
    rect.right    = rect.left + iWid;
    for ( int iInd = 0; iInd < 4; iInd++ )
    {
        m_wndStat[iInd].Create( &theIcons, aiStat[iInd], rect, this, theBitmaps.GetByIndex( DIB_TOOLBAR ) );
        rect.OffsetRect( iWid + BAR_BTN_X_SKIP, 0 );
    }

    // two text windows
    pSb = theIcons.GetByIndex( ICON_BAR_TEXT );
    GetClientRect( &rect );
    iWid = ( rect.Width( ) - 3 * BAR_BTN_X_SKIP ) / 2;
    rect.top += BAR_BTN_HT + ( BAR_TEXT_HT - pSb->m_cyBack ) / 2;
    rect.bottom = rect.top + pSb->m_cyBack;
    rect.left += BAR_BTN_X_SKIP;
    rect.right = rect.left + iWid;

    m_wndText[0].Create( &theIcons, ICON_BAR_TEXT, rect, this, theBitmaps.GetByIndex( DIB_TOOLBAR ) );
    rect.OffsetRect( iWid + BAR_BTN_X_SKIP, 0 );
    m_wndText[1].Create( &theIcons, ICON_BAR_TEXT, rect, this, theBitmaps.GetByIndex( DIB_TOOLBAR ) );

    CheckButtons( );
    if ( pOld != NULL )
        dc.SelectObject( pOld );

    // Create native SDL2 toolbar (no PrintWindow capture needed)
    if ( theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        CRect screenRect;
        GetWindowRect( &screenRect );
        CRect clientRect;
        GetClientRect( &clientRect );

        m_sdlPanel = theApp.m_gameWindow->GetCompositor()->AddPanel(
            "toolbar", screenRect.left, screenRect.top,
            clientRect.Width(), clientRect.Height(), 30 );

        // Create the native SDL2 toolbar renderer
        static SDL2Toolbar s_toolbar;
        s_toolbar.Init( m_sdlPanel, theApp.m_gameWindow.get() );
        theApp.m_gameWindow->SetSDL2Toolbar( &s_toolbar );

        // Wire up button handlers
        CWndBar* pThis = this;
        s_toolbar.SetButtonHandler(0, [pThis]() { pThis->GotoArea(); });
        s_toolbar.SetButtonHandler(1, [pThis]() { pThis->GotoWorld(); });
        s_toolbar.SetButtonHandler(2, [pThis]() { pThis->GotoChat(); });
        s_toolbar.SetButtonHandler(3, [pThis]() { pThis->GotoRelations(); });
        s_toolbar.SetButtonHandler(4, [pThis]() { pThis->GotoVehicles(); });
        s_toolbar.SetButtonHandler(5, [pThis]() { pThis->GotoBuildings(); });
        s_toolbar.SetButtonHandler(6, [pThis]() { pThis->GotoScience(); });
        s_toolbar.SetButtonHandler(7, [pThis]() { pThis->GotoFile(); });

        // Disable chat if not net play
        if ( !theGame.IsNetGame() )
            s_toolbar.EnableButton(2, false);

        // Route events to SDL2Toolbar
        m_sdlPanel->SetEventCallback(
            [](SDL_Event& event, int localX, int localY) -> bool {
                SDL2Toolbar* tb = theApp.m_gameWindow->GetSDL2Toolbar();
                if (tb) return tb->HandleEvent(event, localX, localY);
                return false;
            });

        ::SetWindowLong( m_hWnd, GWL_EXSTYLE,
            ::GetWindowLong( m_hWnd, GWL_EXSTYLE ) | WS_EX_LAYERED | WS_EX_TRANSPARENT );
        ::SetLayeredWindowAttributes( m_hWnd, 0, 1, LWA_ALPHA );
    }

    return 0;
}

void CWndBar::Draw() {
    // Use native SDL2 toolbar rendering — no PrintWindow needed
    SDL2Toolbar* tb = nullptr;
    if ( theApp.m_gameWindow )
        tb = theApp.m_gameWindow->GetSDL2Toolbar();
    if ( tb )
        tb->Render();

    // Render SDL2 unit list panels
    if ( s_sdlVehicleList )
        s_sdlVehicleList->Render();
    if ( s_sdlBuildingList )
        s_sdlBuildingList->Render();
}

void CWndBar::OnClose( )
{
    // don't do anything
}

// --- Legacy-child wrappers (insulate callers from MFC widgets) ---------------

void CWndBar::FlashLowIcon( int idx )
{
    // Legacy: flash the MFC stat-bar icon when a resource is low.
    // SDL2Toolbar already animates at ~1Hz via m_animFrame, so the SDL bar
    // doesn't need an extra kick. Keep the MFC call for now until the legacy
    // children are removed wholesale.
    if ( 0 <= idx && idx < 4 && m_wndStat[idx].m_hWnd != NULL )
        m_wndStat[idx].IncIcon( );
}

void* CWndBar::GetStatusLineData( int line )
{
    if ( line < 0 || line > 1 || m_wndText[line].m_hWnd == NULL )
        return NULL;
    return m_wndText[line].GetStatusData( );
}

void CWndBar::ClearStatusFunc( int line )
{
    if ( line < 0 || line > 1 )
        return;
    if ( m_wndText[line].m_hWnd != NULL )
        m_wndText[line].SetStatusFunc( NULL );
}

void CWndBar::InvalidateStatusLine( int line )
{
    if ( line < 0 || line > 1 )
        return;
    if ( m_wndText[line].m_hWnd != NULL )
        m_wndText[line].InvalidateRect( NULL );
}

void CWndBar::EnableButton( int ID, BOOL bEnable )
{

    if ( theApp.m_gameWindow )
    {
        SDL2Toolbar* tb = theApp.m_gameWindow->GetSDL2Toolbar();
        if ( tb )
            for ( int i = 0; i < NUM_BAR_BTNS; ++i )
                if ( aID[i] == ID )
                {
                    tb->EnableButton( i, bEnable != FALSE );
                    break;
                }

        HWND hBtn = GetDlgItemHwnd( ID );
        if ( hBtn )
            ::EnableWindow( hBtn, bEnable );
        return;
    }

    CBmButton* pBtn = (CBmButton*)GetDlgItem( ID );
    ASSERT_VALID( pBtn );
    if ( pBtn )
        pBtn->EnableWindow( bEnable );
}

void CWndBar::OnPaint( )
{

    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    CRect rect;
    CWndBase::GetClientRect( &rect );
    int iWid = rect.Width( );

    CDIB* pDib = theBitmaps.GetByIndex( DIB_TOOLBAR );

    rect.bottom = rect.top + pDib->GetHeight( );

    for ( int x = 0; x < iWid; x += pDib->GetWidth( ) )
    {
        rect.left  = x;
        rect.right = __min( x + pDib->GetWidth( ), iWid );
        pDib->BitBlt( dc, rect, CPoint( 0, 0 ) );
    }

    // second row
    rect.top += pDib->GetHeight( );
    rect.bottom += pDib->GetHeight( );
    for ( int x = 0; x < iWid; x += pDib->GetWidth( ) )
    {
        rect.left  = x;
        rect.right = __min( x + pDib->GetWidth( ), iWid );
        pDib->BitBlt( dc, rect, CPoint( 0, 3 ) );
    }

    thePal.EndPaint( dc.m_hDC );
    // Do not call CWndAnim::OnPaint() for painting messages
}

void CWndBar::SetStatusText( int iLine, const char* psText, CStatInst::IMPORTANCE iImp )
{

    if ( psText == NULL )
        psText = "";

    ASSERT_VALID( this );
    ASSERT( ( 0 <= iLine ) && ( iLine <= 1 ) );
    ASSERT( AfxIsValidString( psText ) );

    // Forward to native SDL2 toolbar
    if ( theApp.m_gameWindow ) {
        SDL2Toolbar* tb = theApp.m_gameWindow->GetSDL2Toolbar();
        if ( tb ) tb->SetStatusText( iLine, psText, (int)iImp );
    }

    m_wndText[iLine].SetText( psText, iImp );
}

void CWndBar::SetStatusFunc( int iLine, FNSTATUSLINE fnStat, void* pData )
{

    ASSERT_VALID( this );
    ASSERT( ( 0 <= iLine ) && ( iLine <= 1 ) );

    m_wndText[iLine].SetStatusFunc( fnStat, pData );

    // Forward a text summary to the SDL2 toolbar.
    // The MFC status callback paints directly to a DC — the SDL toolbar can't
    // use that, so we build a concise text version of whatever the callback
    // would have shown. Two cases — unit hover (a CUnit*) and terrain hover
    // (a CHex*) — match what CWndArea::OnMouseMove forwards.
    SDL2Toolbar* tb = theApp.m_gameWindow ? theApp.m_gameWindow->GetSDL2Toolbar() : nullptr;
    if ( !tb )
        return;

    if ( fnStat == NULL || pData == NULL )
    {
        // Clearing the status callback — wipe the world-hover text so the
        // toolbar doesn't show stale info after the cursor leaves the map.
        tb->SetStatusText( iLine, "", 0 );
        return;
    }

    if ( fnStat == UnitShowStatus )
    {
        CUnit* pUnit = (CUnit*)pData;
        std::string text = pUnit->GetData()->GetDesc();

        // If it's our unit, append materials
        if ( pUnit->GetOwner()->IsMe() )
        {
            for ( int i = 0; i < CMaterialTypes::GetNumTypes(); i++ )
            {
                int have = pUnit->GetStore( i );
                int need = 0;
                if ( i < CMaterialTypes::GetNumBuildTypes() && pUnit->GetUnitType() == CUnit::building )
                    need = ( (CBuilding*)pUnit )->GetBldgResReq( i, FALSE );
                if ( have > 0 || need > 0 )
                {
                    text += "  " + CMaterialTypes::GetDesc( i ) + ":" + std::to_string( have );
                    if ( need > 0 )
                        text += "(" + std::to_string( need ) + ")";
                }
            }
            // Damage
            int dmg = 100 - pUnit->GetDamagePer();
            if ( dmg > 0 )
                text += "  Dmg:" + std::to_string( __min( 99, dmg ) ) + "%";
        }
        else
        {
            text += " [" + std::string( (const char*)pUnit->GetOwner()->GetName() ) + "]";
        }

        tb->SetStatusText( iLine, text, 0 );
        return;
    }

    if ( fnStat == TerrainShowStatus )
    {
        // Terrain name + (optional) mineral type and remaining quantity %,
        // mirroring what TerrainShowStatus paints in MFC.
        CHex* pHex = (CHex*)pData;
        std::string text = pHex->GetStatus();

        if ( ( pHex->GetUnits() & CHex::minerals ) != 0 )
        {
            CMinerals* pMn = NULL;
            if ( theMinerals.Lookup( pHex->GetHex(), pMn ) && pMn )
            {
                // Hide xilitium / copper until the player has researched it
                bool show = !( pMn->GetType() == CMaterialTypes::copper &&
                               !theGame.GetMe()->CanCopper() );
                if ( show )
                {
                    int iMax = MAX_MINERAL_QUANTITY;
                    switch ( pMn->GetType() )
                    {
                    case CMaterialTypes::coal:   iMax = MAX_MINERAL_COAL_QUANTITY;   break;
                    case CMaterialTypes::iron:   iMax = MAX_MINERAL_IRON_QUANTITY;   break;
                    case CMaterialTypes::oil:    iMax = MAX_MINERAL_OIL_QUANTITY;    break;
                    case CMaterialTypes::copper: iMax = MAX_MINERAL_XIL_QUANTITY;    break;
                    }
                    int pctQty = pMn->GetQuantity() == 0
                        ? 0 : __max( 1, pMn->GetQuantity() / ( iMax / 100 ) );
                    int pctDen = __max( 1,
                        ( 100 * pMn->GetDensity() ) / MAX_MINERAL_DENSITY );
                    text += "  " + CMaterialTypes::GetDesc( pMn->GetType() ) +
                            "  Qty:" + std::to_string( pctQty ) + "%" +
                            "  Density:" + std::to_string( pctDen ) + "%";
                }
            }
        }

        tb->SetStatusText( iLine, text, 0 );
        return;
    }
}

#ifdef _CHEAT
void CWndBar::SetDebugText( int iLine, const char* psText )
{

    // draw it
    CClientDC dc( &( m_wndText[iLine] ) );
    HGDIOBJ   pOld = ::SelectObject( dc.m_hDC, theApp.TextFont( ) );  // Phase 4c prep
    dc.SetBkColor( RGB( 0, 0, 0 ) );
    dc.SetTextColor( RGB( 255, 255, 255 ) );
    dc.SetBkMode( OPAQUE );
    CRect rect;
    m_wndText[iLine].GetClientRect( rect );
    rect.right -= 6;

    dc.DrawText( psText, -1, &rect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER );
    ::SelectObject( dc.m_hDC, pOld );
}
#endif

void CWndBar::CheckButtons( )
{
    if ( !IsCreated() )
        return;
    EnableButton( IDC_BAR_SCIENCE, theGame.GetMe( )->GetExists( CStructureData::research ) );
    EnableButton( IDC_BAR_ADVISOR, theGame.GetMe( )->GetExists( CStructureData::embassy ) );
}

#ifdef BUGBUG
void CWndBar::OnActivateApp( BOOL bActive, HTASK hTask )
{

    CWndBtnStatusBar::OnActivateApp( bActive, hTask );

    if ( bActive )
        SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
    else
        SetWindowPos( &CWnd::wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}
#endif

BEGIN_MESSAGE_MAP( CWndBar, CWndAnim )
//{{AFX_MSG_MAP(CWndBar)
ON_BN_CLICKED( IDC_BAR_AREA, GotoArea )
ON_BN_CLICKED( IDC_BAR_WORLD, GotoWorld )
ON_BN_CLICKED( IDC_BAR_CHAT, GotoChat )
ON_BN_CLICKED( IDC_BAR_VEHICLES, GotoVehicles )
ON_BN_CLICKED( IDC_BAR_BUILDINGS, GotoBuildings )
ON_BN_CLICKED( IDC_BAR_ADVISOR, GotoRelations )
ON_BN_CLICKED( IDC_BAR_SCIENCE, GotoScience )
ON_BN_CLICKED( IDC_BAR_FILE, GotoFile )
ON_WM_CREATE( )
ON_WM_TIMER( )
ON_WM_PAINT( )
ON_WM_CLOSE( )
ON_WM_SIZE( )
ON_MESSAGE( WM_BUTTONMOUSEMOVE, OnButtonMouseMove )
ON_MESSAGE( WM_ICONMOUSEMOVE, OnStatusMouseMove )
ON_WM_DESTROY( )
ON_WM_MOUSEMOVE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CWndBar message handlers

void CWndBar::OnSize( UINT nType, int cx, int cy )
{

    CWndAnim::OnSize( nType, cx, cy );

    // if no time window we haven't been built yet
    if ( m_wndTime.m_hWnd == NULL )
        return;

    // create the buttons
    int   iAdd = NUM_BAR_BTNS * ( theBmBtnData.Width( ) + BAR_BTN_X_SKIP );
    CRect rect( 2 * BAR_BTN_X_SKIP + iAdd, BAR_BTN_Y_START, BAR_BTN_X_SKIP + theBmBtnData.Width( ) + iAdd,
                BAR_BTN_Y_START + theBmBtnData.Height( ) );

    // status bars
    // time goes on the right
    CStatData* pSb = theIcons.GetByIndex( ICON_CLOCK );
    CRect      rTime;
    GetClientRect( &rTime );
    rTime.right -= BAR_BTN_X_SKIP;
    rTime.top    = rect.top;
    rTime.bottom = rect.bottom;

    // we size this based on fitting the text in
    CClientDC dc( this );
    CFont*    pOld;
    if ( pSb->m_pFnt != NULL )
        pOld = dc.SelectObject( pSb->m_pFnt );
    CRect rText( rTime );
    dc.DrawText( "999:99:99", -1, rText, DT_CALCRECT | DT_RIGHT | DT_SINGLELINE | DT_VCENTER );
    rTime.left = rTime.right - rText.Width( ) - pSb->m_leftOff - pSb->m_rightOff;

    m_wndTime.SetWindowPos( NULL, rTime.left, rTime.top, rTime.Width( ), rTime.Height( ), SWP_NOZORDER );

    // rect now goes from last btn + skip to time (4 * status + skip)
    int aiStat[4] = { ICON_GAS, ICON_POWER, ICON_PEOPLE, ICON_FOOD };
    rect.right    = rTime.left;
    int iWid      = ( rect.Width( ) / 4 ) - BAR_BTN_X_SKIP;
    rect.right    = rect.left + iWid;
    for ( int iInd = 0; iInd < 4; iInd++ )
    {
        m_wndStat[iInd].SetWindowPos( NULL, rect.left, rect.top, rect.Width( ), rect.Height( ), SWP_NOZORDER );
        rect.OffsetRect( iWid + BAR_BTN_X_SKIP, 0 );
    }

    // two text windows
    pSb = theIcons.GetByIndex( ICON_BAR_TEXT );
    GetClientRect( &rect );
    iWid = ( rect.Width( ) - 3 * BAR_BTN_X_SKIP ) / 2;
    rect.top += BAR_BTN_HT + ( BAR_TEXT_HT - pSb->m_cyBack ) / 2;
    rect.bottom = rect.top + pSb->m_cyBack;
    rect.left += BAR_BTN_X_SKIP;
    rect.right = rect.left + iWid;

    m_wndText[0].SetWindowPos( NULL, rect.left, rect.top, rect.Width( ), rect.Height( ), SWP_NOZORDER );
    rect.OffsetRect( iWid + BAR_BTN_X_SKIP, 0 );
    m_wndText[1].SetWindowPos( NULL, rect.left, rect.top, rect.Width( ), rect.Height( ), SWP_NOZORDER );

    if ( pSb->m_pFnt != NULL )
        dc.SelectObject( pOld );
}

LRESULT CWndBar::OnButtonMouseMove( WPARAM, LPARAM lParam )
{

    m_wndText[1].SetText( (char*)lParam );
    return ( 0 );
}

LRESULT CWndBar::OnStatusMouseMove( WPARAM, LPARAM lParam )
{

    std::string sText;  // default to blank
    if ( (CWndBaseSuper*)lParam == &m_wndStat[gas] )
    {
        std::string sNum1 = IntToStr( theGame.GetMe( )->GetGasNeed( ) );
        std::string sNum2 = IntToStr( theGame.GetMe( )->GetGasHave( ) );
        sText = strPrintf( EnLoadStdString( IDH_STAT_GAS ).c_str(), sNum1.c_str(), sNum2.c_str() );
    }
    else if ( (CWndBaseSuper*)lParam == &m_wndStat[power] )
    {
        std::string sNum1 = IntToStr( theGame.GetMe( )->GetPwrNeed( ) );
        std::string sNum2 = IntToStr( theGame.GetMe( )->GetPwrHave( ) );
        sText = strPrintf( EnLoadStdString( IDH_STAT_POWER ).c_str(), sNum1.c_str(), sNum2.c_str() );
    }
    else if ( (CWndBaseSuper*)lParam == &m_wndStat[people] )
    {
        std::string sNum1 = IntToStr( theGame.GetMe( )->GetPplTotal( ) );
        std::string sNum3 = IntToStr( theGame.GetMe( )->GetPplVeh( ) );
        if ( theGame.GetMe( )->GetPplBldg( ) >= theGame.GetMe( )->GetPplNeedBldg( ) )
        {
            std::string sNum2 = IntToStr( theGame.GetMe( )->GetPplNeedBldg( ) );
            std::string sNum4 = IntToStr( theGame.GetMe( )->GetPplBldg( ) - theGame.GetMe( )->GetPplNeedBldg( ) );
            sText = strPrintf( EnLoadStdString( IDH_STAT_PEOPLE ).c_str(),
                               sNum1.c_str(), sNum2.c_str(), sNum3.c_str(), sNum4.c_str() );
        }
        else
        {
            std::string sNum2 = IntToStr( theGame.GetMe( )->GetPplBldg( ) );
            std::string sNum4 = IntToStr( theGame.GetMe( )->GetPplNeedBldg( ) - theGame.GetMe( )->GetPplBldg( ) );
            sText = strPrintf( EnLoadStdString( IDH_STAT_PEOPLE2 ).c_str(),
                               sNum1.c_str(), sNum2.c_str(), sNum3.c_str(), sNum4.c_str() );
        }
    }
    else if ( (CWndBaseSuper*)lParam == &m_wndStat[food] )
    {
        std::string sNum1 = IntToStr( theGame.GetMe( )->GetFoodNeed( ) );
        std::string sNum2 = IntToStr( theGame.GetMe( )->GetFood( ) );
        sText = strPrintf( EnLoadStdString( IDH_STAT_FOOD ).c_str(), sNum1.c_str(), sNum2.c_str() );
    }
    else if ( (CWndBaseSuper*)lParam == &m_wndTime )
        sText = EnLoadStdString( IDH_STAT_CLOCK );

    m_wndText[1].SetText( sText.c_str() );
    return ( 0 );
}

void CWndBar::GotoArea( )
{

    CWndArea* pTop = theAreaList.BringToTop( );
    if ( pTop != NULL )
    {
        // Already exists — un-hide its SDL window if the user had closed it.
        if ( pTop->GetAA( ).m_sdlPanel )
            pTop->GetAA( ).m_sdlPanel->SetVisible( true );
        return;
    }

    // need to create one
    CWndArea* pWndArea = new CWndArea( );
    pWndArea->Create( theGame.GetMe( )->m_hexMapStart, NULL, FALSE );
}

void CWndBar::GotoWorld( )
{

    if ( theApp.m_wndWorld.m_hWnd == NULL )
        theApp.m_wndWorld.Create( );  // world must come after area

    theApp.m_wndWorld.ShowWindow( theApp.m_wndMain.IsIconic( ) ? SW_SHOW : SW_RESTORE );
    theApp.m_wndWorld.SetFocus( );

    // Un-hide the SDL World Map window if the user had closed it via the [X].
    if ( theApp.m_wndWorld.GetMapPanel( ) )
        theApp.m_wndWorld.GetMapPanel( )->SetVisible( true );
}

void CWndBar::GotoChat( )
{

    if ( !theGame.IsNetGame( ) )
        return;

    SDL2ChatWindow dlg( theApp.m_gameWindow.get() );
    dlg.DoModal();
}

// (moved to top of file)

static void ToggleUnitListPanel(SDL2UnitList*& pList, SDL2UnitList::ListType type, const char* name) {
    if (!theApp.m_gameWindow || !theApp.m_gameWindow->GetCompositor())
        return;

    // If already open, toggle visibility
    if (pList) {
        // Find the panel and toggle
        SDL2Panel* panel = theApp.m_gameWindow->GetCompositor()->FindPanel(name);
        if (panel) {
            if (panel->IsVisible()) {
                panel->SetVisible(false);
                return;
            }
            panel->SetVisible(true);
            pList->Rebuild();
            return;
        }
    }

    // Create new panel
    int screenW = theApp.m_gameWindow->GetWidth();
    int screenH = theApp.m_gameWindow->GetHeight();
    int panelW = 340, panelH = 400;
    int panelX = (type == SDL2UnitList::VEHICLES) ? screenW - panelW - 10 : screenW - panelW * 2 - 20;
    int panelY = SDL2Panel::TITLE_BAR_HT + 10;

    SDL2Panel* panel = theApp.m_gameWindow->GetCompositor()->AddPanel(
        name, panelX, panelY, panelW, panelH, 25);
    panel->SetMovable(true);
    panel->SetResizable(true);
    panel->SetClosable(true);
    panel->SetTitle((type == SDL2UnitList::VEHICLES) ? "Vehicles" : "Buildings");
    panel->SetCloseCallback([panel]() { panel->SetVisible(false); });

    delete pList;
    pList = new SDL2UnitList(type);
    pList->Init(panel, theApp.m_gameWindow.get());

    SDL2UnitList* pL = pList;
    panel->SetEventCallback(
        [pL](SDL_Event& event, int localX, int localY) -> bool {
            return pL->HandleEvent(event, localX, localY);
        });
}

void CWndBar::GotoVehicles( )
{
    ToggleUnitListPanel(s_sdlVehicleList, SDL2UnitList::VEHICLES, "vehicles");
}

void CWndBar::GotoBuildings( )
{
    ToggleUnitListPanel(s_sdlBuildingList, SDL2UnitList::BUILDINGS, "buildings");
}

void CWndBar::GotoRelations( )
{
    if ( theGame.GetMe( )->GetExists( CStructureData::embassy ) )
        EnableButton( IDC_BAR_ADVISOR, TRUE );
    else
        return;

    if ( theApp.m_gameWindow ) {
        SDL2RelationsDialog dlg( theApp.m_gameWindow.get() );
        dlg.DoModal();
    }
}

void CWndBar::GotoScience( )
{

    _GotoScience( );
}

void CWndBar::_GotoScience( )
{
    if ( theGame.GetMe( )->GetExists( CStructureData::research ) )
        EnableButton( IDC_BAR_SCIENCE, TRUE );
    else
        return;

    if ( theApp.m_gameWindow ) {
        SDL2ResearchDialog dlg( theApp.m_gameWindow.get() );
        dlg.DoModal();
    }
}

void CWndBar::GotoFile( )
{
    if ( theApp.m_gameWindow ) {
        SDL2FileDialog dlg( theApp.m_gameWindow.get() );
        dlg.DoModal();
    }
}

// if the curosr is over us - update it
void CWndBar::UpdateHelp( CWnd* pWnd )
{

    CPoint pt;
    ::GetCursorPos( &pt );
    if ( ::WindowFromPoint( pt ) == pWnd->GetSafeHwnd() )
        SendMessage( WM_ICONMOUSEMOVE, 0, (LPARAM)pWnd );
}

void CWndBar::UpdateGas( )
{
    static int iLastStat = 2;

    if ( theGame.GetMe( )->GetGasHave( ) < theGame.GetMe( )->GetGasNeed( ) / 2 )
    {
        if ( theGame.GetMe( )->GetGasHave( ) <= 0 )
        {
            if ( iLastStat != 0 )
            {
                theGame.Event( EVENT_GAS_OUT, EVENT_BAD );
                iLastStat = 0;
            }
        }
        else if ( iLastStat != 1 )
        {
            theGame.Event( EVENT_GAS_LOW, EVENT_BAD );
            iLastStat = 1;
        }
    }
    else if ( theGame.GetMe( )->GetGasHave( ) < ( theGame.GetMe( )->GetGasNeed( ) * 3 ) / 4 )
    {
        if ( iLastStat != 1 )
        {
            theGame.Event( EVENT_GAS_LOW, EVENT_BAD );
            iLastStat = 1;
        }
        else if ( iLastStat == 0 )
            theGame.Event( EVENT_GAS_OUT, EVENT_OFF );
    }
    else
    {
        if ( iLastStat != 2 )
        {
            theGame.Event( EVENT_GAS_OUT, EVENT_OFF );
            theGame.Event( EVENT_GAS_LOW, EVENT_OFF );
            iLastStat = 2;
        }
    }

    m_wndStat[gas].SetHaveNeed( theGame.GetMe( )->GetGasHave( ), __max( 1, theGame.GetMe( )->GetGasNeed( ) ) );

    // if the cursor is over us - update it
    UpdateHelp( &m_wndStat[gas] );
}

void CWndBar::UpdatePower( )
{
    static int iLastStat = 0;  // we start with no power

    ASSERT_VALID( this );

    // not enough capacity
    if ( theGame.GetMe( )->GetPwrNeed( ) > 0 )
    {
        if ( theGame.GetMe( )->GetPwrNeed( ) > theGame.GetMe( )->GetPwrHave( ) )
        {
            if ( iLastStat != 0 )
            {
                theGame.Event( EVENT_POWER_LOW, EVENT_BAD );
                iLastStat = 0;
            }
        }
        else

        // ok capacity
        {
            if ( iLastStat != 1 )
            {
                theGame.Event( EVENT_POWER_LOW, EVENT_OFF );
                iLastStat = 1;
            }
        }
    }

    m_wndStat[power].SetHaveNeed( theGame.GetMe( )->GetPwrHave( ), theGame.GetMe( )->GetPwrNeed( ) );

    // if the curosr is over us - update it
    UpdateHelp( &m_wndStat[power] );
}

void CWndBar::UpdatePeople( )
{
    static int iLastStat = 1;

    ASSERT_STRICT_VALID( this );

    // not enough capacity
    if ( theGame.GetMe( )->GetPplNeedBldg( ) >= theGame.GetMe( )->GetPplBldg( ) )
    {
        if ( iLastStat != 0 )
        {
            theGame.Event( EVENT_POP_LOW, EVENT_BAD );
            iLastStat = 0;
        }
    }
    else

    // ok
    {
        if ( iLastStat != 1 )
            if ( theGame.GetMe( )->GetPplBldg( ) >=
                 theGame.GetMe( )->GetPplNeedBldg( ) + theGame.GetMe( )->GetPplNeedBldg( ) / 10 )
            {
                theGame.Event( EVENT_POP_LOW, EVENT_OFF );
                iLastStat = 1;
            }
    }

    m_wndStat[people].SetHaveNeed( theGame.GetMe( )->GetPplBldg( ), theGame.GetMe( )->GetPplNeedBldg( ) );

    // if the curosr is over us - update it
    UpdateHelp( &m_wndStat[people] );
}

void CWndBar::UpdateFood( )
{
    static int iLastStat = 2;

    ASSERT_VALID( this );

    if ( theGame.GetMe( )->GetFood( ) <= theGame.GetMe( )->GetFoodNeed( ) / 8 )
    {
        if ( ( theGame.GetMe( )->GetFood( ) <= 0 ) && ( iLastStat != 0 ) )
        {
            theGame.Event( EVENT_FOOD_OUT, EVENT_BAD );
            iLastStat = 0;
        }
    }
    else if ( theGame.GetMe( )->GetFood( ) < theGame.GetMe( )->GetFoodNeed( ) / 2 )
    {
        if ( iLastStat != 1 )
        {
            theGame.Event( EVENT_FOOD_LOW, EVENT_BAD );
            iLastStat = 1;
        }
        else if ( iLastStat == 0 )
            theGame.Event( EVENT_FOOD_OUT, EVENT_OFF );
    }
    else
    {
        if ( iLastStat != 2 )
        {
            theGame.Event( EVENT_FOOD_OUT, EVENT_OFF );
            theGame.Event( EVENT_FOOD_LOW, EVENT_OFF );
            iLastStat = 2;
        }
    }

    m_wndStat[food].SetHaveNeed( theGame.GetMe( )->GetFood( ), theGame.GetMe( )->GetFoodNeed( ) );

    // if the curosr is over us - update it
    UpdateHelp( &m_wndStat[food] );
}

void CWndBar::UpdateTime( )
{

    int  iTime = theGame.GetElapsedSeconds( );
    char sTime[24];

    int iHour = iTime / ( 60 * 60 );
    iTime     = iTime % ( 60 * 60 );
    itoa( iHour, sTime, 10 );
    int iLen = strlen( sTime );

    int iMinute = iTime / 60;
    iTime       = iTime % 60;
    if ( iMinute <= 9 )
    {
        strcpy( &sTime[iLen], ":0" );
        sTime[iLen + 2] = '0' + iMinute;
    }
    else
    {
        strcpy( &sTime[iLen], ":" );
        itoa( iMinute, &sTime[iLen + 1], 10 );
    }

    if ( iTime <= 9 )
    {
        strcpy( &sTime[iLen + 3], ":0" );
        sTime[iLen + 5] = '0' + iTime;
    }
    else
    {
        strcpy( &sTime[iLen + 3], ":" );
        itoa( iTime, &sTime[iLen + 4], 10 );
    }
    sTime[iLen + 6] = 0;

    m_wndTime.SetText( sTime );

#ifdef _LOGOUT
    static int iLastMinute = -1;
    if ( iLastMinute != iMinute )
    {
        iLastMinute = iMinute;
        logPrintf( LOG_PRI_USEFUL, LOG_TIME, "time: %d:%d:%d", iHour, iMinute, iTime );
    }
#endif
}

void CWndBar::OnDestroy( )
{
    if ( m_sdlPanel && theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor() )
    {
        theApp.m_gameWindow->GetCompositor()->RemovePanel( m_sdlPanel );
        m_sdlPanel = nullptr;
    }

    CWndBase::SetFnMouseMove( NULL );

    CWndAnim::OnDestroy( );
}

void CWndBar::OnMouseMove( UINT nFlags, CPoint point )
{

    // are we on the Comm button?
    auto pChild = ChildWindowFromPoint( point );
    if ( pChild == GetDlgItem( IDC_BAR_CHAT ) )
    {
        if ( ( theGame.GetMe( ) != NULL ) && ( theGame.GetMe( )->GetNetNum( ) != 0 ) )
            m_wndText[1].SetText( m_sChat2.c_str( ) );
        else
            m_wndText[1].SetText( m_sChat1.c_str( ) );
    }
    else if ( pChild == GetDlgItem( IDC_BAR_SCIENCE ) )
        m_wndText[1].SetText( m_sScience.c_str( ) );
    else if ( pChild == GetDlgItem( IDC_BAR_ADVISOR ) )
        m_wndText[1].SetText( m_sRelations.c_str( ) );
    else
        m_wndText[1].SetText( "" );
}

void CWndBar::InvalidateStatus( void* pData )
{

    for ( int iInd = 0; iInd < 2; iInd++ )
        if ( m_wndText[iInd].GetStatusData( ) == pData )
            m_wndText[iInd].InvalidateRect( NULL );
}
