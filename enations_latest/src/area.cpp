//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// area.cpp : area window
//

#include "area.h"

#include "bitmaps.h"
#include "bmbutton.h"
#include "bridge.h"
#include "building.inl"
#include "chproute.hpp"
#include "error.h"
#include "event.h"
#include "lastplnt.h"
#include "minerals.inl"
#include "player.h"
#include "relation.h"
#include "sfx.h"
#include "sprite.h"
#include "stdafx.h"
#include "terrain.inl"
#include "ui.inl"
#include "unit.inl"
#include "vehicle.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

CAreaList theAreaList;

CString CWndArea::sWndCls;

const int SEL_WIDTH = 2;


int     CWndArea::m_iCount = 0;
CString CWndArea::m_sHelp;
CString CWndArea::m_sHelpBuild;
CString CWndArea::m_sHelpRoad;
CString CWndArea::m_sHelpCantBuild[9];
CString CWndArea::m_sHelpRMB;
CString CWndArea::m_sHelpOkFarm;
CString CWndArea::m_sHelpBadFarm;
CString CWndArea::m_sHelpNoFarm;
CString CWndArea::m_sHelpOkMine;
CString CWndArea::m_sHelpBadMine;
CString CWndArea::m_sHelpNoMine;

HCURSOR CWndArea::m_hCurReg;
HCURSOR CWndArea::m_hCurGoto[4];
HCURSOR CWndArea::m_hCurWait;
HCURSOR CWndArea::m_hCurStart;
HCURSOR CWndArea::m_hCurRoadBgn[4];
HCURSOR CWndArea::m_hCurRoadSet[4];
HCURSOR CWndArea::m_hCurTarget[4];
HCURSOR CWndArea::m_hCurSelect[4];
HCURSOR CWndArea::m_hCurRoute;
HCURSOR CWndArea::m_hCurMove[9];
HCURSOR CWndArea::m_hCurLoad[4];
HCURSOR CWndArea::m_hCurUnload[4];
HCURSOR CWndArea::m_hCurRepair;
HCURSOR CWndArea::m_hCurNoRepair;

static BOOL _bShowPos = 1;  // FIXME: Hack, temporary static variables to get rid of compile errors
static BOOL _bClickAny = 1;

// accumulator for mouse wheel deltas
static int s_areaWheelAccum = 0;

// 1 if inc to next, 0 if not
const int abPos[NUM_AREA_BUTTONS] = { 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1 };
const int abID[NUM_AREA_BUTTONS]  = { IDC_AREA_COMBAT,  IDC_AREA_CLOCK,        IDC_AREA_COUNTER,
                                     IDC_AREA_ZOOM_IN, IDC_AREA_ZOOM_OUT,     IDC_AREA_RES,

                                     IDC_UNIT_STOP,    IDC_UNIT_RESUME,

                                     IDC_UNIT_BUILD,   IDC_UNIT_CANCEL_BUILD, IDC_UNIT_ROUTE,
                                     IDC_UNIT_UNLOAD,  IDC_UNIT_RETREAT,

                                     IDC_UNIT_ROAD,    IDC_UNIT_CANCEL_ROAD,

                                     IDC_UNIT_REPAIR,  IDC_UNIT_CANCEL_REPAIR };

const int abBtn[NUM_AREA_BUTTONS] = { 54, 37, 36, 25, 26, 14, 39, 34, 4, 22, 47, 40, 35, 5, 23, 32, 45 };

const int abHelp[NUM_AREA_BUTTONS] = { IDH_AREA_COMBAT,      IDH_AREA_CLOCK,  IDH_AREA_COUNTER, IDH_AREA_ZOOM_IN,
                                       IDH_AREA_ZOOM_OUT,    IDH_AREA_RES,

                                       IDH_UNIT_STOP,        IDH_UNIT_RESUME, IDH_UNIT_BUILD,   IDH_UNIT_CANCEL_BUILD,
                                       IDH_UNIT_ROUTE,       IDH_UNIT_UNLOAD, IDH_UNIT_RETREAT, IDH_UNIT_ROAD,
                                       IDH_UNIT_CANCEL_ROAD, IDH_UNIT_REPAIR, IDH_UNIT_CANCEL };

const int NUM_ORD_BUTTONS = 11;
const int ORD_OFFSET      = 6;
enum
{
    enableID,
    disableID,
    hideID
};
// buttons on if nothing selected
const int asNoneID[NUM_ORD_BUTTONS] = { disableID, hideID, disableID, hideID, hideID, hideID,
                                        hideID,    hideID, hideID,    hideID, hideID };
// buttons on if cranes only
const int asCraneID[NUM_ORD_BUTTONS] = { enableID, hideID,   enableID, hideID,   hideID, hideID,
                                         hideID,   enableID, hideID,   enableID, hideID };
// buttons on if trucks only
const int asTruckID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, enableID, hideID,
                                         hideID,   hideID, hideID, hideID, hideID };
// buttons on if unloadable carriers
const int asUnloadID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, hideID, enableID,
                                          hideID,   hideID, hideID, hideID, hideID };
// buttons on if vehicles
const int asVehID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, enableID, hideID,
                                       hideID,   hideID, hideID, hideID, hideID };
// buttons on if bldgs
const int asBldgID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, hideID, hideID,
                                        hideID,   hideID, hideID, hideID, hideID };
// buttons on if 1 factory
const int asFacID[NUM_ORD_BUTTONS] = { enableID, hideID, enableID, hideID, hideID, hideID,
                                       hideID,   hideID, hideID,   hideID, hideID };
// buttons on if bldgs & vehicles
const int asUnitID[NUM_ORD_BUTTONS] = { enableID, hideID, hideID, hideID, hideID, hideID,
                                        hideID,   hideID, hideID, hideID, hideID };


CHexCoord CWndArea::ToBuildUL( CHexCoord& hexCur )
{

    CStructureData const* pData = theStructures.GetData( m_iBuild );
    CHexCoord             _hex( hexCur );
    switch ( m_aa.m_iDir )
    {
    case 0:
        _hex.Y( ) -= pData->GetCY( );
        break;
    case 2:
        _hex.X( ) -= pData->GetCX( );
        break;
    case 3:
        _hex.X( ) -= pData->GetCX( );
        _hex.Y( ) -= pData->GetCY( );
        break;
    }
    _hex.Wrap( );
    return ( _hex );
}

/////////////////////////////////////////////////////////////////////////////
// MouseHook - used to turn off our cursors when the mouse leaves the window

HHOOK CWndArea::m_hhk = NULL;

LRESULT CALLBACK CWndArea::MouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{

    // we have to be over an area map
    if ( ( nCode == HC_ACTION ) && ( theMap.HaveBldgCur( ) ) )
    {
        POSITION pos;
        for ( pos = theAreaList.GetHeadPosition( ); pos != NULL; )
        {
            CWndArea* pWndArea = theAreaList.GetNext( pos );
            // it's going to an area map
            if ( pWndArea->m_hWnd == ( (MOUSEHOOKSTRUCT*)lParam )->hwnd )
                return ( CallNextHookEx( m_hhk, nCode, wParam, lParam ) );
        }

        // it's not on an area map - kill the cursor
        theMap.ClrBldgCur( );
    }

    return ( CallNextHookEx( m_hhk, nCode, wParam, lParam ) );
}


/////////////////////////////////////////////////////////////////////////////
// CAreaList

CAreaList::CAreaList( )
{

    m_hexLastCombat = CHexCoord( 0, 0 );
    m_bLcSet        = FALSE;
}

void CAreaList::MoveSizeToNew( int xOld, int yOld )
{
#ifdef LOGGINGON
    OutputDebugStringA( "MoveSizeToNew" );
#endif

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWnd = GetNext( pos );
        CRect     rect;
        pWnd->GetWindowRect( &rect );
        pWnd->SetWindowPos( NULL, ( rect.left * theApp.m_iScrnX ) / xOld, ( rect.top * theApp.m_iScrnY ) / yOld,
                            ( rect.Width( ) * theApp.m_iScrnX ) / xOld, ( rect.Height( ) * theApp.m_iScrnY ) / yOld,
                            SWP_NOZORDER );
    }
}

void CAreaList::SetLastAttack( CHexCoord const& _hex )
{

    m_hexLastCombat = _hex;

    if ( m_bLcSet )
        return;

    // enable the button
    m_bLcSet = TRUE;

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->EnableButton( IDC_AREA_COMBAT, TRUE );
    }
}

void CAreaList::AddWindow( CWndArea* pWnd )
{

    ASSERT_VALID( this );
    ASSERT_VALID( pWnd );

    AddTail( pWnd );
}

void CAreaList::DestroyAllWindows( )
{

    while ( GetCount( ) > 0 )
    {
        CWndArea* pWnd = GetHead( );
        RemoveHead( );
        pWnd->DestroyWindow( );
    }

    RemoveAll( );
    theApp.m_wndWorld.NewAreaMap( NULL );
}

void CAreaList::EnableWindows( BOOL bEnable )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea ); // what's wrong here?
        pWndArea->EnableWindow( bEnable );
    }
}

CWndArea* CAreaList::GetTop( )
{

    if ( GetCount( ) < 1 )
        return ( NULL );

    if ( GetCount( ) == 1 )
        return ( GetHead( ) );

    if ( CWndArea::sWndCls.GetLength( ) == 0 )
        return ( NULL );

    return ( (CWndArea*)CWndBase::FindWindow( CWndArea::sWndCls, NULL ) );
}

CWndArea* CAreaList::BringToTop( )
{

    CWndArea* pWnd = GetTop( );
    if ( pWnd == NULL )
        return ( NULL );

    pWnd->ShowWindow( SW_RESTORE );
    pWnd->SetFocus( );

    return ( pWnd );
}

void CAreaList::MaterialChange( CUnit const* pUnit ) const
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->MaterialChange( pUnit );
    }
}

void CAreaList::InvalidateStatus( CUnit const* pUnit ) const
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        if ( ( pWndArea->m_pUnit == pUnit ) || ( pWndArea->GetStaticUnit( ) == pUnit ) )
            pWndArea->InvalidateStatus( );
    }
}

void CAreaList::UnitDying( CUnit* pUnit )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->UnitDying( pUnit );
    }
}

void CAreaList::SelectOff( )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        pWndArea->SelectOff( );
    }
}

void CAreaList::XilDiscovered( )
{

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CWndArea* pWndArea = GetNext( pos );
        ASSERT_STRICT_VALID( pWndArea );
        if ( pWndArea->m_bShowRes )
        {
            pWndArea->ResClicked( );
            pWndArea->ResClicked( );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CListUnits

void CListUnits::AddUnit( CUnit* pUnit, BOOL bDoList )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pUnit );

    // add if not already in the list
    if ( Find( pUnit ) == NULL )
    {
        pUnit->SetSelected( bDoList );
        AddHead( pUnit );
    }

    // if we changed selection we turn off whatever
    theAreaList.SelectOff( );
}

void CListUnits::RemoveUnit( CUnit* pUnit )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( pUnit );

    POSITION pos = Find( pUnit );
    if ( pos != NULL )
        RemoveAt( pos );

    pUnit->SetUnselected( TRUE );
}

void CListUnits::RemoveAllUnits( BOOL bDoList )
{

    ASSERT_STRICT_VALID( this );

    if ( bDoList )
    {
        theApp.m_wndBldgs.m_ListBox.SetRedraw( FALSE );
        theApp.m_wndVehicles.m_ListBox.SetRedraw( FALSE );
    }

    POSITION pos;
    for ( pos = GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = GetNext( pos );
        ASSERT_STRICT_VALID( pUnit ); // hmm
        pUnit->SetUnselected( bDoList );
    }

    if ( bDoList )
    {
        theApp.m_wndBldgs.m_ListBox.SetRedraw( TRUE );
        theApp.m_wndVehicles.m_ListBox.SetRedraw( TRUE );
        theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL, FALSE );
        theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL, FALSE );
    }

    RemoveAll( );
}


/////////////////////////////////////////////////////////////////////////////
// CWndUnitStat

CWndUnitStat::CWndUnitStat( )
{

    m_pUnit = NULL;
}

void CWndUnitStat::SetUnit( CUnit* pUnit )
{

    m_pUnit = pUnit;
}

BEGIN_MESSAGE_MAP( CWndUnitStat, CWndStatBar )
//{{AFX_MSG_MAP(CWndUnitStat)
ON_WM_PAINT( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

void CWndUnitStat::SetText( char const* pStr, CStatInst::IMPORTANCE iImp )
{

    CWndStatBar::SetText( pStr, iImp );
}

// for UpdateMaterials
void CWndUnitStat::UpdateStat( )
{

    CPoint pt;
    ::GetCursorPos( &pt );
    if ( CWnd::WindowFromPoint( pt ) == this )
        OnMouseMove( 0, pt );
}

// display info about this stuff
void CWndUnitStat::OnMouseMove( UINT, CPoint )
{

    if ( m_pUnit == NULL )
        CWndStatBar::SetText( NULL );
    else
    {
        CString str;
        ::UnitStatusText( m_pUnit, str );
        theApp.m_wndBar.SetStatusText( 1, str );
    }
}

void CWndUnitStat::OnPaint( )
{

    if ( m_pUnit == NULL )
        CWndStatBar::OnPaint( );
    else
    {
        CPaintDC dc( this );
        thePal.Paint( dc.m_hDC );

        CRect rect;
        GetClientRect( &rect );
        CPoint pt( 0, 0 );
        MapWindowPoints( GetParent( ), &pt, 1 );

        ::UnitShowStatus( m_pUnit, &dc, rect, theBitmaps.GetByIndex( DIB_AREA_BAR ), pt );

        thePal.EndPaint( dc.m_hDC );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CWndAreaStatic

CWndAreaStatic::CWndAreaStatic( )
{

    m_iNumStatusText = NUM_STATUS_TEXT;
    m_iStatusStrt = m_iStatusNoCraneStrt = 7 * ( theBmBtnData.Width( ) + AREA_BTN_X_SKIP );
    m_iStatusCraneStrt                   = 9 * ( theBmBtnData.Width( ) + AREA_BTN_X_SKIP );
}

CWndAreaStatic::~CWndAreaStatic( )
{
}

BOOL CWndAreaStatic::PreCreate( )
{

    // make sure we have enough room for all the buttons & status lines
    CRect rect( 0, 0, m_iStatusStrt + theApp.TextWid( ) * MIN_TEXT_WID * 2, AREA_BTN_HT );

    AdjustWindowRect( &rect, dwStatusWndStyle, FALSE );
    m_iXmin = rect.Width( );
    m_iYmin = rect.Height( );

    // finally, if its wider than the screen - we bring it in
    m_iXmin = __min( m_iXmin, theApp.m_iScrnX );

    return ( TRUE );
}

BEGIN_MESSAGE_MAP( CWndAreaStatic, CWndBase )
//{{AFX_MSG_MAP(CWndAreaStatic)
ON_WM_CREATE( )
ON_WM_PAINT( )
ON_WM_SIZE( )
ON_WM_DESTROY( )
ON_WM_ERASEBKGND( )
ON_MESSAGE( WM_BUTTONMOUSEMOVE, OnChildMouseMove )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CWndAreaStatic message handlers

LRESULT CWndAreaStatic::OnChildMouseMove( WPARAM, LPARAM lParam )
{

    theApp.m_wndBar.SetStatusText( 1, (char*)lParam );
    return ( 0 );
}

int CWndAreaStatic::OnCreate( LPCREATESTRUCT lpCS )
{

    if ( CWndBase::OnCreate( lpCS ) == -1 )
        return -1;

    // we had to start with the load icon to get a different class
    ::SetClassLong( m_hWnd, GCL_HCURSOR, NULL );

    // create the position buttons
    CRect rect( AREA_BTN_X_SKIP, AREA_BTN_Y_START, AREA_BTN_X_SKIP + theBmBtnData.Width( ),
                AREA_BTN_Y_START + theBmBtnData.Height( ) );

    // create the buttons
    CBmButton* pBtn = m_Btns;
    for ( int iOn = 0; iOn < NUM_AREA_BUTTONS; iOn++, pBtn++ )
    {
        pBtn->Create( abBtn[iOn], abHelp[iOn], &theBmBtnData, rect, theBitmaps.GetByIndex( DIB_AREA_BAR ), this,
                      abID[iOn] );

        // how we show it
        if ( ( ORD_OFFSET <= iOn ) && ( iOn < NUM_ORD_BUTTONS + ORD_OFFSET ) )
        {
            pBtn->EnableWindow( FALSE );
            if ( asNoneID[iOn - ORD_OFFSET] == hideID )
                pBtn->ShowWindow( SW_HIDE );
        }

        if ( abPos[iOn] )
            rect.OffsetRect( theBmBtnData.Width( ) + AREA_BTN_X_SKIP, 0 );

        if ( abID[iOn] == IDC_UNIT_ROAD )
            m_iStatusNoCraneStrt = m_iStatusStrt = rect.left + AREA_BTN_X_SKIP;
    }
    rect.left += AREA_BTN_X_SKIP;
    m_iStatusCraneStrt = rect.left;

    // create the status windows
    m_wndStat.Create( &theIcons, ICON_BAR_TEXT, rect, this, theBitmaps.GetByIndex( DIB_AREA_BAR ) );
    SizeStatus( );

    return 0;
}

BOOL CWndAreaStatic::OnCommand( WPARAM wParam, LPARAM lParam )
{

    if ( HIWORD( wParam ) == BN_CLICKED )
    {
        GetParent( )->SendMessage( WM_COMMAND, wParam, lParam );
        return ( TRUE );
    }

    return ( CWndBase::OnCommand( wParam, lParam ) );
}

BOOL CWndAreaStatic::OnEraseBkgnd( CDC* )
{

    return TRUE;
}

void CWndAreaStatic::OnPaint( )
{

    ASSERT_STRICT_VALID( this );
    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    CRect rect;
    CWndBase::GetClientRect( &rect );

    theBitmaps.GetByIndex( DIB_AREA_BAR )->Tile( dc, rect );

    thePal.EndPaint( dc.m_hDC );
    // Do not call CWndBase::OnPaint() for painting messages
}

void CWndAreaStatic::OnSize( UINT nType, int cx, int cy )
{
#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "CWndAreaStatic::OnSize cx=%d cy=%d\n", cx, cy );
    OutputDebugStringA( buf );
#endif


    CWndBase::OnSize( nType, cx, cy );

    SizeStatus( );
}

void CWndAreaStatic::SizeStatus( )
{

    // adjust status window rects
    CRect rect;
    CWndBase::GetClientRect( &rect );

    // adjust the text windows
    rect.left = m_iStatusStrt;
    rect.right -= AREA_BTN_X_SKIP;
    rect.top += ( AREA_BTN_HT - AREA_TEXT_HT ) / 2;
    rect.bottom = rect.top + AREA_TEXT_HT;

    m_wndStat.SetWindowPos( NULL, rect.left, rect.top, rect.Width( ), rect.Height( ), SWP_NOZORDER );
}

BOOL CWndArea::IsButtonEnabled( int ID ) const
{

    CWnd* pBtn = m_WndStatic.GetDlgItem( ID );
    ASSERT_STRICT_VALID( pBtn );
    return ( pBtn->IsWindowEnabled( ) );
}

void CWndAreaStatic::EnableButton( int ID, BOOL bEnable )
{

    CWnd* pBtn = GetDlgItem( ID );
    ASSERT_STRICT_VALID( pBtn );
    pBtn->EnableWindow( bEnable );
}

void CWndAreaStatic::ShowButton( int ID, BOOL bShow )
{

    CWnd* pBtn = GetDlgItem( ID );
    ASSERT_STRICT_VALID( pBtn );
    pBtn->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
}


/////////////////////////////////////////////////////////////////////////////
// CWndArea

void CWndArea::LoadStaticResources( )
{

    m_iCount++;
    if ( m_iCount != 1 )
        return;

    // set the mouse hook func
    m_hhk = SetWindowsHookEx( WH_MOUSE, MouseProc, NULL, theApp.m_nThreadID );

    // load them
    m_sHelp.LoadString( IDH_AREA_WIN );
    m_sHelpBuild.LoadString( IDH_AREA_WIN_BUILD );
    m_sHelpRoad.LoadString( IDH_AREA_WIN_ROAD );
    m_sHelpCantBuild[0].LoadString( IDH_AREA_CANT_BLDG_NEXT );
    m_sHelpCantBuild[1].LoadString( IDH_AREA_CANT_WATER_NEXT );
    m_sHelpCantBuild[2].LoadString( IDH_AREA_CANT_BLDG_RIVER_NEXT );
    m_sHelpCantBuild[3].LoadString( IDH_AREA_CANT_VEH_IN_WAY );
    m_sHelpCantBuild[4].LoadString( IDH_AREA_CANT_ON_WATER );
    m_sHelpCantBuild[5].LoadString( IDH_AREA_CANT_NO_WATER );
    m_sHelpCantBuild[6].LoadString( IDH_AREA_CANT_NO_LAND_EXIT );
    m_sHelpCantBuild[7].LoadString( IDH_AREA_CANT_NO_WATER_EXIT );
    m_sHelpCantBuild[8].LoadString( IDH_AREA_CANT_TOO_STEEP );
    m_sHelpRMB.LoadString( IDH_AREA_WIN_RMB );
    m_sHelpOkFarm.LoadString( IDH_AREA_OK_FARM );
    m_sHelpBadFarm.LoadString( IDH_AREA_BAD_FARM );
    m_sHelpNoFarm.LoadString( IDH_AREA_NO_FARM );
    m_sHelpOkMine.LoadString( IDH_AREA_OK_MINE );
    m_sHelpBadMine.LoadString( IDH_AREA_BAD_MINE );
    m_sHelpNoMine.LoadString( IDH_AREA_NO_MINE );

    // need our own class so we can change the cursor
    m_hCurReg        = theApp.LoadStandardCursor( IDC_ARROW );
    m_hCurGoto[0]    = theApp.LoadCursor( IDC_GOTO0 );
    m_hCurGoto[1]    = theApp.LoadCursor( IDC_GOTO1 );
    m_hCurGoto[2]    = theApp.LoadCursor( IDC_GOTO2 );
    m_hCurGoto[3]    = theApp.LoadCursor( IDC_GOTO3 );
    m_hCurWait       = theApp.LoadStandardCursor( IDC_WAIT );
    m_hCurRoadBgn[0] = theApp.LoadCursor( IDC_ROAD_BEGIN0 );
    m_hCurRoadBgn[1] = theApp.LoadCursor( IDC_ROAD_BEGIN1 );
    m_hCurRoadBgn[2] = theApp.LoadCursor( IDC_ROAD_BEGIN2 );
    m_hCurRoadBgn[3] = theApp.LoadCursor( IDC_ROAD_BEGIN3 );
    m_hCurRoadSet[0] = theApp.LoadCursor( IDC_ROAD_SET0 );
    m_hCurRoadSet[1] = theApp.LoadCursor( IDC_ROAD_SET1 );
    m_hCurRoadSet[2] = theApp.LoadCursor( IDC_ROAD_SET2 );
    m_hCurRoadSet[3] = theApp.LoadCursor( IDC_ROAD_SET3 );
    m_hCurStart      = theApp.LoadStandardCursor( IDC_APPSTARTING );
    m_hCurTarget[0]  = theApp.LoadCursor( IDC_TARGET0 );
    m_hCurTarget[1]  = theApp.LoadCursor( IDC_TARGET1 );
    m_hCurTarget[2]  = theApp.LoadCursor( IDC_TARGET2 );
    m_hCurTarget[3]  = theApp.LoadCursor( IDC_TARGET3 );
    m_hCurSelect[0]  = theApp.LoadCursor( IDC_SELECT0 );
    m_hCurSelect[1]  = theApp.LoadCursor( IDC_SELECT1 );
    m_hCurSelect[2]  = theApp.LoadCursor( IDC_SELECT2 );
    m_hCurSelect[3]  = theApp.LoadCursor( IDC_SELECT3 );
    m_hCurRoute      = theApp.LoadCursor( IDC_ROUTE );
    m_hCurMove[0]    = theApp.LoadCursor( IDC_MOVE0 );
    m_hCurMove[1]    = theApp.LoadCursor( IDC_MOVE1 );
    m_hCurMove[2]    = theApp.LoadCursor( IDC_MOVE2 );
    m_hCurMove[3]    = theApp.LoadCursor( IDC_MOVE3 );
    m_hCurMove[4]    = theApp.LoadCursor( IDC_MOVE4 );
    m_hCurMove[5]    = theApp.LoadCursor( IDC_MOVE5 );
    m_hCurMove[6]    = theApp.LoadCursor( IDC_MOVE6 );
    m_hCurMove[7]    = theApp.LoadCursor( IDC_MOVE7 );
    m_hCurMove[8]    = theApp.LoadStandardCursor( IDC_SIZEALL );
    m_hCurLoad[0]    = theApp.LoadCursor( IDC_LOAD0 );
    m_hCurLoad[1]    = theApp.LoadCursor( IDC_LOAD1 );
    m_hCurLoad[2]    = theApp.LoadCursor( IDC_LOAD2 );
    m_hCurLoad[3]    = theApp.LoadCursor( IDC_LOAD3 );
    m_hCurUnload[0]  = theApp.LoadCursor( IDC_UNLOAD0 );
    m_hCurUnload[1]  = theApp.LoadCursor( IDC_UNLOAD1 );
    m_hCurUnload[2]  = theApp.LoadCursor( IDC_UNLOAD2 );
    m_hCurUnload[3]  = theApp.LoadCursor( IDC_UNLOAD3 );
    m_hCurRepair     = theApp.LoadCursor( IDC_REPAIR );
    m_hCurNoRepair   = theApp.LoadCursor( IDC_NO_REPAIR );
}

void CWndArea::UnloadStaticResources( )
{

    m_iCount--;
    if ( m_iCount > 0 )
        return;

    // undo the hook
    UnhookWindowsHookEx( m_hhk );

    // unload them
    m_sHelp.Empty( );
    m_sHelpBuild.Empty( );
    m_sHelpRoad.Empty( );
    m_sHelpRMB.Empty( );
    for ( int iOn = 0; iOn < 9; iOn++ ) m_sHelpCantBuild[iOn].Empty( );

    // BUGBUG - is there no way to delete a cursor?
}

void CWndArea::InvalidateStatus( )
{

    ASSERT_STRICT_VALID( this );

    m_WndStatic.m_wndStat.InvalidateRect( NULL );
}

CWndArea::CWndArea( )
{
    m_aa.SetWnd( this );
    m_iMoveCur = 8;

    m_iFound      = 0;
    m_iBuild      = -1;
    m_iMode       = normal;
    m_bRBtnDown   = FALSE;
    m_bCapMouse   = FALSE;
    m_pWndInfo    = NULL;
    m_iBuildDir   = 0;
    m_bNewPos     = TRUE;
    m_uMouseMode  = lmb_nothing;
    m_bNewSound   = TRUE;
    m_bShowRes    = FALSE;
    m_pSelUnder   = NULL;
    m_bScrollBars = FALSE;

    m_phexRoadPath  = NULL;
    m_ppUnderSprite = NULL;
    m_iNumRoadHex   = 0;
}

void CWndArea::PostNcDestroy( )
{

    // remove it from the list
    POSITION pos = theAreaList.Find( this );
    if ( pos != NULL )
        theAreaList.RemoveAt( pos );

    ReleaseMouse( );
    ::ClipCursor( NULL );

    delete m_pWndInfo;

    UnloadStaticResources( );

    delete this;
}

CWndArea::~CWndArea( )
{

    delete[] m_pSelUnder;

    // remove it from the list
    POSITION pos = theAreaList.Find( this );
    ASSERT( pos == NULL );
    if ( pos != NULL )
        theAreaList.RemoveAt( pos );

    ReleaseMouse( );
    ::ClipCursor( NULL );
}

BOOL CWndArea::PreCreateWindow( CREATESTRUCT& cs )
{

    CWndAnim::PreCreateWindow( cs );

    // get the mins for the static window
    m_WndStatic.PreCreate( );

    CRect rect( 0, 0, m_WndStatic.m_iXmin, m_WndStatic.m_iYmin * 2 );
    AdjustWindowRect( &rect, dwPopWndStyle, FALSE );
    m_iXmin = rect.Width( );
    m_iYmin = rect.Height( );

    LoadStaticResources( );

    return ( TRUE );
}

void CWndArea::GetClientRect( LPRECT lpRect ) const
{

    CWndAnim::GetClientRect( lpRect );
    CRect rStatic;
    m_WndStatic.GetClientRect( &rStatic );
    if ( rStatic.Height( ) < lpRect->bottom )
        lpRect->bottom -= rStatic.Height( );
    else
        lpRect->bottom = 0;

    if ( m_bScrollBars )
    {
        lpRect->right -= GetSystemMetrics( SM_CXVSCROLL );
        lpRect->bottom -= GetSystemMetrics( SM_CYHSCROLL );
    }
}

#ifdef LOGGINGON
int  created = 0;
#endif

void CWndArea::Create( CMapLoc const& ml, CUnit* pUnit, BOOL bFirst )
{
#ifdef LOGGINGON
    created++;
    char buf[128];
    sprintf_s( buf, "CWndArea::Create (created=%d)\n", created );
    OutputDebugStringA( buf );
#endif

    ASSERT_VALID_OR_NULL( pUnit );

    if ( ( m_pUnit = pUnit ) != NULL )
    {
        m_lstUnits.AddUnit( pUnit, TRUE );
        ASSERT( m_lstUnits.GetCount( ) == 1 );
    }

    if ( sWndCls.GetLength( ) == 0 )
        sWndCls = AfxRegisterWndClass( CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, m_hCurMove[8], 0, 0 );

    m_aa.Set( ml, 0, max( 1, theApp.GetZoomData( )->GetFirstZoom( ) ) );

    // figure the size
    CRect     rect;
    CWndArea* pPrev = theAreaList.GetTop( );
    if ( pPrev == NULL )
        rect.SetRect( theApp.GetProfileInt( theApp.m_sResIni, "AreaX", theApp.m_iCol1 ),
                      theApp.GetProfileInt( theApp.m_sResIni, "AreaY", 0 ),
                      theApp.GetProfileInt( theApp.m_sResIni, "AreaEX", theApp.m_iScrnX ),
                      theApp.GetProfileInt( theApp.m_sResIni, "AreaEY", theApp.m_iRow3 ) );
    else
    {
        pPrev->GetWindowRect( &rect );
        CRect rectClient;
        pPrev->GetClientRect( &rectClient );
        pPrev->ClientToScreen( &rectClient );
        rect.left = rectClient.left;
        rect.top  = rectClient.top;

        // set to the same dir & zoom
        m_aa.m_iDir  = pPrev->m_aa.m_iDir;
        m_aa.m_iZoom = pPrev->m_aa.m_iZoom;
    }

    CString sTitle;
    sTitle.LoadString( IDS_TITLE_AREA_MAP );
    DWORD dwStyle = dwPopWndStyle;

    if ( CreateEx( 0, sWndCls, sTitle, dwStyle, rect.left, rect.top, rect.Width( ), rect.Height( ),
                   theApp.m_pMainWnd->m_hWnd, NULL, NULL ) == 0 )
        throw( ERR_RES_CREATE_WND );

    if ( m_bScrollBars )
    {
        CRect rectClient;

        GetClientRect( &rectClient );

        CRect rectH( 0, rectClient.Height( ), rectClient.Width( ),
                     rectClient.Height( ) + GetSystemMetrics( SM_CYHSCROLL ) );
        CRect rectV( rectClient.Width( ), 0, rectClient.Width( ) + GetSystemMetrics( SM_CXVSCROLL ),
                     rectClient.Height( ) );

        if ( !m_scrollbarH.Create( SBS_HORZ | WS_CHILD | WS_VISIBLE, rectH, this, unsigned( -1 ) ) )
            throw( ERR_RES_CREATE_WND );

        if ( !m_scrollbarV.Create( SBS_VERT | WS_CHILD | WS_VISIBLE, rectV, this, unsigned( -1 ) ) )
            throw( ERR_RES_CREATE_WND );

        // set up the scroll bars
        //   note: the button is always in the middle and the range is always the map size
        CSize szMap = theMap.GetSize( );
        m_scrollbarH.SetScrollRange( 0, szMap.cx - 1, FALSE );
        m_scrollbarH.SetScrollPos( ( szMap.cx - 1 ) / 2, FALSE );
        m_scrollbarV.SetScrollRange( 0, szMap.cy - 1, FALSE );
        m_scrollbarV.SetScrollPos( ( szMap.cy - 1 ) / 2, FALSE );
    }

    m_colorbuffer.SetColor( 255, rect.Width( ) );

    // add us to the list of area windows
    theAreaList.AddWindow( this );

    // the area map is always added at the head, everything else at the tail
    theAnimList.AddHead( this );

    // set the cursor of starting
    if ( bFirst )
        SetupStart( );  // first one - rocket
    else
        SetButtonState( );

    // set it up
    InvalidateWindow( );
    InvalidateSound( );
    ReRender( );

    EnableButton( IDC_AREA_COMBAT, theAreaList.HaveAttack( ) );
    CWndArea::CheckZoomBtns( );

    if ( !bFirst )
        ShowWindow( SW_SHOW );
}

BEGIN_MESSAGE_MAP( CWndArea, CWndAnim )
//{{AFX_MSG_MAP(CWndArea)
ON_WM_GETMINMAXINFO( )
ON_WM_SYSCOMMAND( )
ON_WM_HSCROLL( )
ON_WM_MOUSEMOVE( )
ON_WM_PAINT( )
ON_WM_VSCROLL( )
ON_WM_CREATE( )
ON_WM_SIZE( )
ON_WM_DESTROY( )
ON_BN_CLICKED( IDC_AREA_COMBAT, LastCombat )
ON_BN_CLICKED( IDC_AREA_ZOOM_IN, ZoomIn )
ON_BN_CLICKED( IDC_AREA_ZOOM_OUT, ZoomOut )
ON_BN_CLICKED( IDC_AREA_CLOCK, TurnClock )
ON_BN_CLICKED( IDC_AREA_COUNTER, TurnCounter )
ON_BN_CLICKED( IDC_AREA_RES, ResClicked )
ON_BN_CLICKED( IDC_UNIT_STOP, StopUnit )
ON_BN_CLICKED( IDC_UNIT_RESUME, ResumeUnit )
ON_COMMAND( IDA_CENTER, CenterUnit )
ON_COMMAND( IDA_DESTROY, DestroyUnit )
ON_COMMAND( IDA_STOP_DESTROY, StopDestroyUnit )
ON_COMMAND( IDA_CUR_UP, CurUp )
ON_COMMAND( IDA_CUR_RIGHT, CurRight )
ON_COMMAND( IDA_CUR_DOWN, CurDown )
ON_COMMAND( IDA_CUR_LEFT, CurLeft )
ON_COMMAND( IDA_OPPO, OppoUnit )
ON_BN_CLICKED( IDC_UNIT_ROAD, RoadUnit )
ON_BN_CLICKED( IDC_UNIT_CANCEL_ROAD, CancelRoadUnit )
ON_BN_CLICKED( IDC_UNIT_BUILD, BuildUnit )
ON_BN_CLICKED( IDC_UNIT_CANCEL_BUILD, CancelBuildUnit )
ON_BN_CLICKED( IDC_UNIT_ROUTE, RouteUnit )
ON_BN_CLICKED( IDC_UNIT_UNLOAD, UnloadUnit )
ON_BN_CLICKED( IDC_UNIT_RETREAT, RetreatUnit )
ON_BN_CLICKED( IDC_UNIT_REPAIR, RepairUnit )
ON_BN_CLICKED( IDC_UNIT_CANCEL_REPAIR, CancelRepairUnit )
ON_WM_LBUTTONDOWN( )
ON_WM_RBUTTONDOWN( )
ON_WM_RBUTTONUP( )
ON_WM_RBUTTONDBLCLK( )
ON_WM_ACTIVATE( )
ON_WM_LBUTTONDBLCLK( )
ON_WM_LBUTTONUP( )
ON_WM_SETCURSOR( )
ON_WM_KEYUP( )
ON_WM_KEYDOWN( )
ON_COMMAND( IDA_CLOSE_WIN, OnCloseWin )
ON_COMMAND( IDA_DESELECT, OnDeselect )
ON_WM_ERASEBKGND( )
ON_COMMAND( IDA_BUILD, BuildUnit )
ON_COMMAND( IDA_RETREAT, RetreatUnit )
ON_COMMAND( IDA_ROUTE, RouteUnit )
ON_COMMAND( IDA_UNLOAD, UnloadUnit )
ON_WM_CTLCOLOR( )
ON_WM_MOUSEWHEEL( )
ON_WM_MOUSEMOVE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CWndArea message handlers

BOOL CWndArea::OnEraseBkgnd( CDC* )
{

    return TRUE;
}

BOOL CWndArea::OnSetCursor( CWnd* pWnd, UINT nHitTest, UINT message )
{

    if ( ( pWnd != this ) || ( nHitTest != HTCLIENT ) )
        return CWndAnim::OnSetCursor( pWnd, nHitTest, message );

    SetMouseState( );
    return ( TRUE );
}

void CWndArea::CurUp( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( 0, -rect.Height( ) / 4 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::CurRight( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( rect.Width( ) / 4, 0 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::CurDown( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( 0, rect.Height( ) / 4 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::CurLeft( )
{

    CRect rect;
    GetClientRect( &rect );

    m_aa.MoveCenterPixels( -rect.Width( ) / 4, 0 );
    theApp.m_wndWorld.NewLocation( );
    m_bUpdateAll = TRUE;
    // GGTESTING	InvalidateWindow();
}

void CWndArea::ReRender( )
{

    CDIB* pdib = m_aa.m_dibwnd.GetDIB( );
    CRect rect;
    GetClientRect( &rect );

    // if RMB & near the edge then we continuously scroll
    if ( m_bRBtnDown )
    {
        CPoint pt;
        ::GetCursorPos( &pt );
        ScreenToClient( &pt );

        int x, y;
        int iWid = rect.Width( );
        if ( pt.x < iWid / 8 )
            x = -( iWid / 8 - pt.x );
        else
        {
            int iTmp = iWid - iWid / 8;
            if ( pt.x > iTmp )
                x = pt.x - iTmp;
            else
                x = 0;
        }
        int iHt = rect.Height( );
        if ( pt.y < iHt / 8 )
            y = -( iHt / 8 - pt.y );
        else
        {
            int iTmp = iHt - iHt / 8;
            if ( pt.y > iTmp )
                y = pt.y - iTmp;
            else
                y = 0;
        }

        // which cursor?
        switch ( ( __minmax( -1, 1, x ) + 2 ) | ( ( __minmax( -1, 1, y ) + 2 ) << 2 ) )
        {
        case 0x06:  // up
            m_iMoveCur = 0;
            break;
        case 0x07:  // UR
            m_iMoveCur = 1;
            break;
        case 0x0B:  // right
            m_iMoveCur = 2;
            break;
        case 0x0F:  // LR
            m_iMoveCur = 3;
            break;
        case 0x0E:  // down
            m_iMoveCur = 4;
            break;
        case 0x0D:  // LL
            m_iMoveCur = 5;
            break;
        case 0x09:  // left
            m_iMoveCur = 6;
            break;
        case 0x05:  // UL
            m_iMoveCur = 7;
            break;
        default:
            m_iMoveCur = 8;
            break;
        }
        ::SetCursor( m_hCurMove[m_iMoveCur] );

        if ( ( x != 0 ) || ( y != 0 ) )
        {
            m_aa.MoveCenterPixels( x * 2, y * 2 );
            // GGTESTING			InvalidateWindow();
            theApp.m_wndWorld.NewLocation( );
        }
    }

    bInvAmb = FALSE;

    BOOL bSave;
    if ( m_bUpdateAll )
    {
        bSave      = bForceDraw;
        bForceDraw = TRUE;
    }

    theMap.Update( m_aa );

    // Generate a paint message for each coalesced dirty rect

    // Render each rect in the current dirty rect list and add it to
    // the list of rects to be blitted

    // GGTESTING	m_aa.Render( this );

    if ( m_bUpdateAll )
    {
        bForceDraw   = bSave;
        m_bUpdateAll = FALSE;
    }

    // unit may have moved under (or been created)
    CPoint pt;
    ::GetCursorPos( &pt );
    if ( ::WindowFromPoint( pt ) == m_hWnd )
    {
        // make sure in client area
        ScreenToClient( &pt );
        if ( ( pt.x >= 0 ) && ( pt.y >= 0 ) && ( pt.x < rect.right ) && ( pt.y < rect.bottom ) )
            SetMouseState( );
    }

    // we handle new sound here since paint's are defered
    if ( m_bNewSound )
    {
        m_bNewSound = FALSE;
        if ( theAreaList.GetTop( ) == this )
            UpdateSound( );
    }
}

// when on (or NULL if off) a unit - shows
void CWndArea::StatUnit( CUnit* pUnit )
{

    // we only see our own
    if ( ( pUnit != NULL ) && ( !pUnit->GetOwner( )->IsMe( ) ) )
        pUnit = NULL;

    if ( pUnit == GetStaticUnit( ) )
        return;

    m_WndStatic.m_wndStat.SetUnit( pUnit );
    m_WndStatic.m_wndStat.InvalidateRect( NULL );
}

static int fnEnumIsVisNoVeh( CHex* pHex, CHexCoord, void* pData )
{

    // must be visible
    if ( ( !pHex->GetVisibility( ) ) || ( pHex->GetUnits( ) & CHex::unit ) )
    {
        *( (BOOL*)pData ) = FALSE;
        return ( TRUE );
    }

    return ( FALSE );
}

void CWndArea::MaterialChange( CUnit const* pUnit )
{

    // see if this is displayed in the tooltip
    CWndInfo* pWndInfo = GetInfo( );
    ASSERT_STRICT_VALID_OR_NULL( pWndInfo );
    if ( ( pWndInfo != NULL ) && ( pWndInfo->m_hWnd != NULL ) && ( pWndInfo->GetUnit( ) == pUnit ) )
        pWndInfo->Refigure( );

    // update buttons
    if ( m_pUnit == pUnit )
        SetButtonState( );

    if ( pUnit != GetStaticUnit( ) )
        return;

    // update the status bars
    m_WndStatic.UpdateStat( );
}

void CWndArea::OnMouseMove( UINT nFlags, CPoint point )
{

    // kill the info window - if they move > 4 pixels
    if ( ( m_pWndInfo ) && ( m_pWndInfo->m_hWnd != NULL ) )
        if ( ( abs( point.x - m_ptRMDN.x ) > theApp.m_iScrnX / 160 ) ||
             ( abs( point.y - m_ptRMDN.y ) > theApp.m_iScrnX / 160 ) )
            m_pWndInfo->DestroyWindow( );

    CRect rect;
    GetClientRect( &rect );

    // sometimes the point is outside the window
    if ( !rect.PtInRect( point ) )
    {
        CWndBase::OnMouseMove( nFlags, point );
        return;
    }

    // drawing selection box
    if ( m_iMode == normal_select )
    {
        m_selRect.SetRect( m_selOrig.x, m_selOrig.y, point.x, point.y );
        m_selRect.NormalizeRect( );
        m_selRect &= rect;
    }

    // if RMB then we scroll
    //   if in the NC area we continuously scroll (in update)
    if ( m_bRBtnDown )
    {
        ASSERT_STRICT_VALID_STRUCT( &m_aa );

#ifdef BUGBUG
        m_aa.MoveCenterPixels( point.x - m_ptRMB.x, point.y - m_ptRMB.y );

        // paint it
        // GGTESTING InvalidateWindow ();
#endif
        m_ptRMB = point;
        theApp.m_wndWorld.NewLocation( );
        CWndBase::OnMouseMove( nFlags, point );
        m_bNewPos = TRUE;

        return;
    }

    // where are we
    CHitInfo  hitinfo = m_aa.GetHit( point );
    CHexCoord hexcoord( hitinfo._GetHexCoord( ) );
    hexcoord.Wrap( );
    CHex*  pHex  = theMap._GetHex( hexcoord );
    CUnit* pUnit = hitinfo.GetUnit( );

    if ( m_iMode == road_set )
        SetRoadIcons( hexcoord );

    // if not visible then it's not there
    if ( pUnit != NULL )
        if ( ( ( pUnit->GetUnitType( ) == CUnit::building ) && ( !pUnit->IsVisible( ) ) ) ||
             ( ( pUnit->GetUnitType( ) != CUnit::building ) && ( !pHex->GetVisible( ) ) ) )
            pUnit = NULL;

#ifdef BUGBUG
    // if it's not ours it has to be at alliance
    if ( ( pUnit != NULL ) && ( !pUnit->GetOwner( )->IsMe( ) ) )
        if ( pUnit->GetOwner( )->GetRelations( ) != RELATIONS_ALLIANCE )
            pUnit = NULL;
#endif

#ifdef _CHEAT
    if ( _bShowPos )
    {
        CString sBuf;
        sBuf = IntToCString( hexcoord.X( ) ) + "," + IntToCString( hexcoord.Y( ) ) + " (" +
               IntToCString( pHex->GetAlt( ) ) + ") ";
        theApp.m_wndBar.SetStatusText( 0, sBuf );
    }
#endif

    // if no unit show the terrain
    if ( pUnit == NULL )
    {
        theApp.m_wndBar.SetStatusFunc( 1, TerrainShowStatus, pHex );

        // tell them what they can do
        SetStatusText( m_sHelp );
    }
    else
        // ok, we have a unit
        theApp.m_wndBar.SetStatusFunc( 1, UnitShowStatus, pUnit );

    if ( m_iMode == road_begin )
        theApp.m_wndBar.SetStatusText( 0, m_sHelpRoad );

    // rocket pos - special test
    BOOL bBuildOk = TRUE;
    if ( ( m_iMode == rocket_ready ) || ( m_iMode == rocket_pos ) )
    {
        // we test to make sure that all vehicles can get out of the rocket
        CHexCoord _hexBuild = ToBuildUL( hexcoord );
        if ( !CStructureData::CanBuild( _hexBuild, GetBuildDir( ), CStructureData::rocket, FALSE, TRUE ) )
            bBuildOk = FALSE;
        else
        {
            CStructureData const* pData = theStructures.GetData( m_iBuild );
            theMap.EnumHexes( _hexBuild, GetBuildDir( ) & 1 ? pData->GetCY( ) : pData->GetCX( ),
                              GetBuildDir( ) & 1 ? pData->GetCX( ) : pData->GetCY( ), fnEnumIsVisNoVeh, &bBuildOk );
        }
    }

    // if we are going to build we talk about the terrain
    if ( ( m_iMode == build_ready ) || ( m_iMode == build_loc ) || ( m_iMode == rocket_ready ) ||
         ( m_iMode == rocket_pos ) )
    {
        CHexCoord _hexBuild = ToBuildUL( hexcoord );
        int       iWhy;
        m_iFound = theMap.FoundationCost( _hexBuild, m_iBuild, GetBuildDir( ), NULL, NULL, &iWhy );
        if ( ( m_iFound < 0 ) || ( !bBuildOk ) )
        {
            theMap.SetBldgCur( _hexBuild, m_iBuild, GetBuildDir( ), 1 );
            if ( ( iWhy > 0 ) && ( iWhy <= 9 ) && ( !m_sHelpCantBuild[iWhy - 1].IsEmpty( ) ) )
            {
                TRAP( m_sHelpCantBuild[iWhy - 1].IsEmpty( ) );
                theApp.m_wndBar.SetStatusText( 0, m_sHelpCantBuild[iWhy - 1], CStatInst::critical );
            }
            else
                theApp.m_wndBar.SetStatusText( 0, m_sHelpCantBuild[6], CStatInst::critical );
            CWnd::OnMouseMove( nFlags, point );
            return;
        }

        int iCurType = m_iFound < 0 ? 1 : 0;
        switch ( m_iBuild )
        {
        case CStructureData::farm:
        case CStructureData::lumber: {
            int                   iMul = CFarmBuilding::LandMult( _hexBuild, m_iBuild, GetBuildDir( ) );
            CString               sText( "(" + IntToCString( iMul ) + ") " );
            CStatInst::IMPORTANCE iImp;
            if ( ( iMul < 2 ) || ( m_iFound < 0 ) )
            {
                m_iFound = -1;
                iCurType = 1;
                sText += m_sHelpNoFarm;
                iImp = CStatInst::critical;
            }
            else if ( iMul < 5 )
            {
                iCurType = 2;
                sText += m_sHelpBadFarm;
                iImp = CStatInst::warn;
            }
            else
            {
                sText += m_sHelpOkFarm;
                iImp = CStatInst::status;
            }
            theApp.m_wndBar.SetStatusText( 0, sText, iImp );
            break;
        }

        case CStructureData::coal:
        case CStructureData::iron:
        case CStructureData::oil_well:
        case CStructureData::copper: {
            CStructureData const* pData = theStructures.GetData( m_iBuild );
            int                   iSize = pData->GetCX( ) * pData->GetCY( );
            int                   qMul  = CMineBuilding::TotalQuantity( _hexBuild, m_iBuild, GetBuildDir( ) );
            int                   iDiv;
            switch ( m_iBuild )
            {
            case CStructureData::coal:
                iDiv = MAX_MINERAL_COAL_QUANTITY;
                break;
            case CStructureData::iron:
                iDiv = MAX_MINERAL_IRON_QUANTITY;
                break;
            case CStructureData::oil_well:
                iDiv = MAX_MINERAL_OIL_QUANTITY;
                break;
            case CStructureData::copper:
                iDiv = MAX_MINERAL_XIL_QUANTITY;
                break;
            default:
                iDiv = MAX_MINERAL_QUANTITY;
                break;
            }

            int iQuan = ( qMul * 1000 ) / ( iDiv * iSize );
            if ( qMul > 0 )
                iQuan = __max( 1, iQuan );
            int dMul = CMineBuilding::TotalDensity( _hexBuild, m_iBuild, GetBuildDir( ) );
            int iDen = ( dMul * 100 ) / ( MAX_MINERAL_DENSITY * iSize );
            if ( dMul > 0 )
                iDen = __max( 1, iDen );

            CString               sText( "(" + IntToCString( iQuan, 10, TRUE ) + ", " + IntToCString( iDen ) + ") " );
            CStatInst::IMPORTANCE iImp;
            if ( ( qMul < 2 ) || ( m_iFound < 0 ) || ( dMul < 1 ) )
            {
                m_iFound = -1;
                iCurType = 1;
                sText += m_sHelpNoMine;
                iImp = CStatInst::critical;
            }
            else if ( ( iQuan < 400 / ( iSize / 2 ) ) && ( iDen < 40 / ( iSize / 2 ) ) )
            {
                iCurType = 2;
                sText += m_sHelpBadMine;
                iImp = CStatInst::warn;
            }
            else
            {
                sText += m_sHelpOkMine;
                iImp = CStatInst::status;
            }
            theApp.m_wndBar.SetStatusText( 0, sText, iImp );
            break;
        }

        default:
            theApp.m_wndBar.SetStatusText( 0, m_sHelpBuild );
            break;
        }

        // set the building cursor
        theMap.SetBldgCur( _hexBuild, m_iBuild, GetBuildDir( ), iCurType );
    }

    CWnd::OnMouseMove( nFlags, point );
}

//--------------------------------------------------------------------------
// CWndArea::Draw
//--------------------------------------------------------------------------
void CWndArea::Draw( )
{
    m_aa.Render( );

    // Draw any overlays here

    // Draw new selection rect (note: doesn't force render of interior)

    if ( m_iMode == normal_select )
    {
        DrawSelectionRect( );

        // Add selection rectangle to the list of rects to get blted this frame

        m_aa.GetDirtyRects( )->AddRect( &m_selRect, CDirtyRects::RECT_LIST::LIST_BLT );
    }

    // Blt the dirty rects to the screen

    m_aa.GetDirtyRects( )->BltRects( );

    // Erase the selection rect (blted next frame)

    if ( m_iMode == normal_select )
    {
        RestoreSelectionRect( );  // note: doesn't force render of interior

        // Add selection rectangle to the list of rects to get blted next frame

        m_aa.GetDirtyRects( )->AddRect( &m_selRect, CDirtyRects::RECT_LIST::LIST_BLT );
    }

    // Fill the backbuffer with magenta (for testing)

#ifdef bugbug_CHEAT
    if ( _bClearWindow )
        m_aa.m_dibwnd.GetDIB( )->Clear( NULL, 253 );
#endif
}

void CWndArea::OnPaint( )
{
    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    // Render the rect

    CRect rect    = dc.m_ps.rcPaint;
    CRect rectDIB = rect & m_aa.m_dibwnd.GetRect( );

    if ( !rectDIB.IsRectEmpty( ) )
    {
        // Render the rect
        theMap.UpdateRect( m_aa, rectDIB, CDrawParms::draw );

        // Paint the rect
        m_aa.m_dibwnd.Paint( &rectDIB );
    }

    if ( m_bScrollBars )
    {
        // Paint the rect where the scroll bars meet

        CRect rectClient;

        GetClientRect( (RECT*)&rectClient );

        rect.SetRect( 0, 0, GetSystemMetrics( SM_CXVSCROLL ), GetSystemMetrics( SM_CYHSCROLL ) );
        rect += rectClient.BottomRight( );

        COLORREF colorref = PALETTERGB( 230, 180, 115 );

        dc.FillSolidRect( &rect, colorref );
    }

    // Do not call CWndBase::OnPaint() for painting messages

    thePal.EndPaint( dc.m_hDC );
}

//---------------------------------------------------------------------------
// CWndArea::DrawSelectionRect
// this now does an XOR. It can be called fresh on a newly rendered
// bitmap or it can be called with the old rect to undo and then the new rect
// to do it
//---------------------------------------------------------------------------

void CWndArea::DrawSelectionRect( )
{

    if ( m_pSelUnder == NULL )
    {
        TRAP( );
        return;
    }

    CDIB* pdib = m_aa.m_dibwnd.GetDIB( );

    // show the select rect
    int     iBytesPerPixel = pdib->GetBytesPerPixel( );
    CDIBits dibits         = pdib->GetBits( );

    BYTE* pbyDst         = dibits + pdib->GetOffset( m_selRect.left, m_selRect.top );
    int   iWid           = m_selRect.Width( );
    int   iWidBytes      = iWid * iBytesPerPixel;
    int   iSelWidthBytes = SEL_WIDTH * iBytesPerPixel;
    int   iHt            = m_selRect.Height( );
    int   iAdd           = pdib->GetDirPitch( );

    // grab underlying pixels as we go
    BYTE* pUnder        = m_pSelUnder;
    int   iAddLineUnder = iWid * iBytesPerPixel;

    BYTE const* pbyColorBuf = m_colorbuffer.GetBuffer( Max( iWid, SEL_WIDTH ) );

    int iNum = SEL_WIDTH;

    while ( ( iNum > 0 ) && ( iHt > 0 ) )
    {
        memcpy( pUnder, pbyDst, iAddLineUnder );
        pUnder += iAddLineUnder;

        memcpy( pbyDst, pbyColorBuf, iWidBytes );
        pbyDst += iAdd;
        iNum--;
        iHt--;
    }

    int iAddColUnderLeft  = iBytesPerPixel * Min( iWid, SEL_WIDTH );
    int iAddColUnderRight = iBytesPerPixel * Min( iWid - SEL_WIDTH, SEL_WIDTH );
    int iOff              = iWidBytes - iAddColUnderRight;

    while ( iHt-- > SEL_WIDTH )
    {
        memcpy( pUnder, pbyDst, iAddColUnderLeft );
        pUnder += iAddColUnderLeft;
        memcpy( pbyDst, pbyColorBuf, iAddColUnderLeft );

        if ( iAddColUnderRight > 0 )
        {
            memcpy( pUnder, pbyDst + iOff, iAddColUnderRight );
            pUnder += iAddColUnderRight;

            memcpy( pbyDst + iOff, pbyColorBuf, iAddColUnderRight );
        }

        pbyDst += iAdd;
    }

    while ( iHt-- >= 0 )
    {
        memcpy( pUnder, pbyDst, iAddLineUnder );
        pUnder += iAddLineUnder;

        memcpy( pbyDst, pbyColorBuf, iWidBytes );
        pbyDst += iAdd;
    }
}

// restore the pixels under the selection rectangle
void CWndArea::RestoreSelectionRect( )
{
    if ( m_pSelUnder == NULL )
    {
        TRAP( );
        return;
    }

    CDIB* pdib = m_aa.m_dibwnd.GetDIB( );

    int     iBytesPerPixel = pdib->GetBytesPerPixel( );
    CDIBits dibits         = pdib->GetBits( );

    BYTE* pbyDst         = dibits + pdib->GetOffset( m_selRect.left, m_selRect.top );
    int   iWid           = m_selRect.Width( );
    int   iHt            = m_selRect.Height( );
    int   iWidBytes      = iWid * iBytesPerPixel;
    int   iSelWidthBytes = SEL_WIDTH * iBytesPerPixel;
    int   iAdd           = pdib->GetDirPitch( );

    // restore the underlying pixels
    BYTE* pUnder        = m_pSelUnder;
    int   iAddLineUnder = iWidBytes;

    int iNum = SEL_WIDTH;
    while ( ( iNum > 0 ) && ( iHt > 0 ) )
    {
        memcpy( pbyDst, pUnder, iAddLineUnder );
        pUnder += iAddLineUnder;

        pbyDst += iAdd;
        iNum--;
        iHt--;
    }

    int iAddColUnderLeft  = iBytesPerPixel * Min( iWid, SEL_WIDTH );
    int iAddColUnderRight = iBytesPerPixel * Min( iWid - SEL_WIDTH, SEL_WIDTH );
    int iOff              = iWidBytes - iAddColUnderRight;

    while ( iHt-- > SEL_WIDTH )
    {
        memcpy( pbyDst, pUnder, iAddColUnderLeft );
        pUnder += iAddColUnderLeft;

        if ( iAddColUnderRight > 0 )
        {
            memcpy( pbyDst + iOff, pUnder, iAddColUnderRight );
            pUnder += iAddColUnderRight;
        }

        pbyDst += iAdd;
    }

    while ( iHt-- >= 0 )
    {
        memcpy( pbyDst, pUnder, iAddLineUnder );
        pUnder += iAddLineUnder;
        pbyDst += iAdd;
    }
}

void CWndArea::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{

    int X = 0;

    switch ( nSBCode )
    {
    // line - move 1 hex
    case SB_LINELEFT:
        X--;
        break;
    case SB_LINERIGHT:
        X++;
        break;

    // page - move 1/2 window
    case SB_PAGELEFT: {
        CRect rect;
        GetClientRect( &rect );
        X = -( ( rect.Width( ) + CGameMap::HexWid( m_aa.m_iZoom ) / 2 ) / CGameMap::HexWid( m_aa.m_iZoom ) + 1 ) / 2;
        if ( X > -2 )
            X = -2;
        break;
    }
    case SB_PAGERIGHT: {
        CRect rect;
        GetClientRect( &rect );
        X = ( ( rect.Width( ) + CGameMap::HexWid( m_aa.m_iZoom ) / 2 ) / CGameMap::HexWid( m_aa.m_iZoom ) + 1 ) / 2;
        if ( X < 2 )
            X = 2;
        break;
    }

        // BUGBUG		case SB_THUMBTRACK :
    case SB_THUMBPOSITION: {
        CSize szSize = theMap.GetSize( );
        X            = -(int)( ( szSize.cx - 1 ) / 2 - nPos );
        break;
    }

    // move to the end - go to the exact oppisate end of the map
    case SB_LEFT:
    case SB_RIGHT: {
        CSize szSize = theMap.GetSize( );
        X            = szSize.cx / 2;
        break;
    }
    }

    if ( X )
    {
        m_aa.MoveCenterViewHexes( X, 0 );
        // GGTESTING		InvalidateWindow ();
        theApp.m_wndWorld.NewLocation( );
    }

    CWndBase::OnHScroll( nSBCode, nPos, pScrollBar );
}

void CWndArea::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{

    int Y = 0;

    switch ( nSBCode )
    {
    // line - move 1 hex
    case SB_LINEDOWN:
        Y--;
        break;
    case SB_LINEUP:
        Y++;
        break;

    // page - move 1/2 window
    case SB_PAGEDOWN: {
        CRect rect;
        GetClientRect( &rect );
        Y = -( ( rect.Height( ) + CGameMap::HexHt( m_aa.m_iZoom ) / 2 ) / CGameMap::HexHt( m_aa.m_iZoom ) + 1 ) / 2;
        if ( Y > -2 )
            Y = -2;
        break;
    }
    case SB_PAGEUP: {
        CRect rect;
        GetClientRect( &rect );
        Y = ( ( rect.Height( ) + CGameMap::HexHt( m_aa.m_iZoom ) / 2 ) / CGameMap::HexHt( m_aa.m_iZoom ) + 1 ) / 2;
        if ( Y < 2 )
            Y = 2;
        break;
    }

        // BUGBUG		case SB_THUMBTRACK :
    case SB_THUMBPOSITION: {
        CSize szSize = theMap.GetSize( );
        Y            = (int)( ( szSize.cy - 1 ) / 2 - nPos );
        break;
    }

    // move to the end - go to the exact oppisate end of the map
    case SB_TOP:
    case SB_BOTTOM: {
        CSize szSize = theMap.GetSize( );
        Y            = szSize.cy / 2;
        break;
    }
    }

    if ( Y )
    {
        m_aa.MoveCenterViewHexes( 0, -Y );
        // GGTESTING		InvalidateWindow ();
        theApp.m_wndWorld.NewLocation( );
    }

    CWndBase::OnVScroll( nSBCode, nPos, pScrollBar );
}


int CWndArea::OnCreate( LPCREATESTRUCT lpCreateStruct )
{

    CWndAnim::OnCreate( lpCreateStruct );

    m_bScrollBars = theApp.GetProfileInt( "Advanced", "Scroll", 0 );

    // NOTE: this crashes on load game because ?something? isn't initialized
    // if first window AND have placement info - use it
    BOOL bPlaceIt = ( ( theAreaList.GetCount( ) == 0 ) 
        && ( theGame.m_wpArea.length != 0 ) );

#ifdef LOGGINGON
    char buf[128];
    sprintf_s( buf, "CWndArea::OnCreate: %d\n", bPlaceIt );
    OutputDebugStringA( buf );
#endif
    // override because its crashing on load:
   // bPlaceIt = FALSE;

    tShowStat.Init( );
    uShowStat.Init( );

    // accelerators for this window
    m_hAccel = ::LoadAccelerators( theApp.m_hInstance, MAKEINTRESOURCE( IDR_AREA ) );

    CRect rect;
    CWndAnim::GetClientRect( &rect );
    rect.top = rect.bottom - m_WndStatic.m_iYmin;
    CString sWndCls( AfxRegisterWndClass( CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, m_hCurLoad[0] ) );
    m_WndStatic.Create( sWndCls, NULL, dwStatusWndStyle, rect, this, 0, NULL );

    // we had to start with the build icon to get a different class
    ::SetClassLong( m_hWnd, GCL_HCURSOR, NULL );

    // set up a WinG DC for the client area
    GetClientRect( &rect );
    m_cx = rect.Width( );
    m_cy = rect.Height( );

    ASSERT( ptrthebltformat.Value( ) );

    // this is creating the CDIB for the main area
    // so like, the background for the gameplay window and mini map methinks!)
    // or maybe the entire window thing? only 1 of these exist?
  //  int rWidth = rect.Width( );
  //  int rHeight = rect.Height( );
#ifdef LOGGINGON
    {
        /*
        char buf[256];
        sprintf_s( buf,
                   "Main Area CDIB: Format=%d, Type=%d, Direction=%d, "
                   "Width(local)=%d, Height(local)=%d, Width(rect)=%d, Height(rect)=%d\n",
                   ptrthebltformat->GetColorFormat( ), ptrthebltformat->GetType( ), ptrthebltformat->GetDirection( ),
                   m_cx, m_cy, rect.Width( ), rect.Height( ) );
        OutputDebugStringA( buf );
        */
    }
#endif
    
    m_aa.m_dibwnd.Init(
        this->m_hWnd,
        new CDIB( ptrthebltformat->GetColorFormat( ), ptrthebltformat->GetType( ), 
            ptrthebltformat->GetDirection( ) ),
        rect.Width( ), rect.Height( ) );

    m_bUpdateAll = TRUE;

    if ( bPlaceIt ) // only done in loading? or maybe bringing it back up after closing it?
    {
#ifdef LOGGINGON
        OutputDebugStringA( "bPlaceIt!");
#endif
        bPlaceIt = TRUE;
        SetWindowPlacement( &( theGame.m_wpArea ) );
        Center( theGame.m_hexAreaCenter );

        // bring the world window back too
        if ( theApp.m_wndWorld.m_hWnd == NULL )
        {
            theApp.m_wndWorld.Create( );
            theApp.m_wndWorld.NewAreaMap( this );
        }
    }
    else
    {
        theGame.m_wpArea.length = sizeof( WINDOWPLACEMENT );
        GetWindowPlacement( &( theGame.m_wpArea ) );
    }

    return ( 0 );
}

void CWndArea::OnSize( UINT nType, int cx, int cy )
{

    CWndAnim::OnSize( nType, cx, cy );

    // move/re-size the status window
    m_WndStatic.SetWindowPos( NULL, 0, cy - m_WndStatic.m_iYmin, cx, m_WndStatic.m_iYmin,
                              SWP_NOACTIVATE | SWP_NOZORDER );

    CRect rect;
    GetClientRect( &rect );
    m_cx = rect.Width( );
    m_cy = rect.Height( );

    // create the bitmap for the new size
    m_aa.m_dibwnd.Size( MAKELPARAM( m_cx, m_cy ) );

    // cursor under buffer
    CDIB* pdib           = m_aa.m_dibwnd.GetDIB( );
    int   iBytesPerPixel = pdib->GetBytesPerPixel( );
    delete[] m_pSelUnder;
    m_pSelUnder = new BYTE[pdib->GetWidth( ) * iBytesPerPixel * SEL_WIDTH * 2 +
                           pdib->GetHeight( ) * iBytesPerPixel * SEL_WIDTH * 2 + iBytesPerPixel * SEL_WIDTH * 8];

    // re-center it
    m_aa.Resized( );

    // GGTESTING	m_aa.SetCenter( m_aa.GetCenter() );

    theGame.m_wpArea.length = sizeof( WINDOWPLACEMENT );
    GetWindowPlacement( &( theGame.m_wpArea ) );

    if ( m_bScrollBars )
    {
        if ( m_scrollbarH.m_hWnd )
            m_scrollbarH.MoveWindow( 0, rect.Height( ), rect.Width( ), GetSystemMetrics( SM_CYHSCROLL ) );

        if ( m_scrollbarV.m_hWnd )
            m_scrollbarV.MoveWindow( rect.Width( ), 0, GetSystemMetrics( SM_CXVSCROLL ), rect.Height( ) );
    }

    // redraw the map
    // GGTESTING InvalidateWindow ();
}

void CWndArea::OnGetMinMaxInfo( MINMAXINFO FAR* lpMMI )
{

    // we limit how small it can be
    if ( lpMMI->ptMinTrackSize.x < m_iXmin )
        lpMMI->ptMinTrackSize.x = m_iXmin;
    if ( lpMMI->ptMinTrackSize.y < m_iYmin )
        lpMMI->ptMinTrackSize.y = m_iYmin;

    if ( theApp.m_wndBar.m_hWnd != NULL )
    {
        CRect rect;
        theApp.m_wndBar.GetWindowRect( &rect );
        lpMMI->ptMaxTrackSize.y = __min( lpMMI->ptMaxTrackSize.y, rect.top );
        lpMMI->ptMaxSize.y      = __min( lpMMI->ptMaxSize.y, rect.top );
    }

    CWndAnim::OnGetMinMaxInfo( lpMMI );
}

void CWndArea::OnSysCommand( UINT nID, LPARAM lParam )
{

    // for minimize hide it - only if last
    if ( theAreaList.GetCount( ) <= 1 )
        if ( ( nID == SC_MINIMIZE ) || ( nID == SC_CLOSE ) )
        {
            ShowWindow( SW_HIDE );
            return;
        }

    CWndAnim::OnSysCommand( nID, lParam );
}

void CWndArea::OnDestroy( )
{

    // if we're the status line kill it
    if ( theApp.m_wndBar.m_wndText[1].GetStatusData( ) != NULL )
    {
        CPoint pt;
        ::GetCursorPos( &pt );
        theApp.m_wndBar.m_wndText[1].SetStatusFunc( NULL );
    }

    // give the world map a new area map
    CWndArea* pNewArea = theAreaList.GetTop( );
    if ( pNewArea == this )
    {
        POSITION pos;
        for ( pos = theAreaList.GetHeadPosition( ); pos != NULL; )
        {
            pNewArea = theAreaList.GetNext( pos );
            ASSERT_STRICT_VALID( pWndArea );
            if ( pNewArea != this )
                break;
        }
    }
    if ( pNewArea == this )
    {
        ASSERT( FALSE );
        pNewArea = NULL;
    }
    theApp.m_wndWorld.NewAreaMap( pNewArea );

    // free everything up
    ReleaseMouse( );
    ::ClipCursor( NULL );

    // save position
    theGame.m_hexAreaCenter = m_aa.GetCenter( );

    m_aa.m_dibwnd.Exit( );

    CWndAnim::OnDestroy( );

    if ( pNewArea == NULL )
    {
        uShowStat.Close( );
        tShowStat.Close( );
    }
}

void CWndArea::ZoomIn( )
{

    ASSERT_STRICT_VALID( this );

    if ( m_aa.m_iZoom > theApp.GetZoomData( )->GetFirstZoom( ) )
    {
        m_aa.Zoom( CAnimAtr::ZOOM_IN );

        ASSERT_STRICT_VALID( &theMap );
        
        InvalidateWindow ();
        InvalidateSound( );
    }

    CheckZoomBtns( );
}

void CWndArea::ZoomOut( )
{

    ASSERT_STRICT_VALID( this );

    if ( m_aa.m_iZoom < NUM_ZOOM_LEVELS - 1 )
    {
        m_aa.Zoom( CAnimAtr::ZOOM_OUT );

        ASSERT_STRICT_VALID( &theMap );
        
        InvalidateWindow ();
        InvalidateSound( );
    }

    CheckZoomBtns( );
}

void CWndArea::CheckZoomBtns( )
{

    if ( m_aa.m_iZoom <= theApp.GetZoomData( )->GetFirstZoom( ) )
        EnableButton( IDC_AREA_ZOOM_IN, FALSE );
    else
        EnableButton( IDC_AREA_ZOOM_IN, TRUE );
    if ( m_aa.m_iZoom >= NUM_ZOOM_LEVELS - 1 )
        EnableButton( IDC_AREA_ZOOM_OUT, FALSE );
    else
        EnableButton( IDC_AREA_ZOOM_OUT, TRUE );
}

void CWndArea::TurnClock( )
{
    ASSERT_STRICT_VALID( this );

    m_aa.Turn( CAnimAtr::TURN_CLOCKWISE );

    ASSERT_STRICT_VALID( &theMap );
    theApp.m_wndWorld.NewDir( );
    InvalidateWindow ();
    InvalidateSound( );
}

void CWndArea::TurnCounter( )
{
    ASSERT_STRICT_VALID( this );

    m_aa.Turn( CAnimAtr::TURN_COUNTERCLOCKWISE );

    ASSERT_STRICT_VALID( &theMap );
    theApp.m_wndWorld.NewDir( );
    InvalidateWindow ();
    InvalidateSound( );
}

void CWndArea::ResClicked( )
{
    static int aiRes[] = { -1, -1, 3, 3, -1, -1, 2, -1, 0, 1 };

    m_bShowRes = !m_bShowRes;

    BOOL bCopper = theGame.GetMe( )->CanCopper( );

    // walk through each mineral setting the CHex
    POSITION pos = theMinerals.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dw;
        CMinerals* pMn;
        theMinerals.GetNextAssoc( pos, dw, pMn );

        CHexCoord _hex( dw >> 16, dw & 0xFFFF );
        CHex*     pHex = theMap._GetHex( _hex );
        ASSERT_VALID( pHex );

        _hex.SetInvalidated( );

        // give it the resource terrain tile
        if ( bCopper || ( pMn->GetType( ) != CMaterialTypes::copper ) )
        {
            if ( m_bShowRes )
                pHex->m_psprite = theTerrain.GetSprite( CHex::resources, aiRes[pMn->GetType( )] );
            else
            {
                switch ( pHex->GetVisibleType( ) )
                {
                case CHex::road:
                    pHex->ChangeToRoad( _hex );
                    break;
                case CHex::city: {
                    int iIndex;
                    if ( pHex->GetUnits( ) & CHex::bldg )
                        iIndex = CITY_BUILD_OFF + RandNum( CITY_BUILD_NUM - 1 );
                    else
                        iIndex = CITY_DESTROYED_OFF + RandNum( CITY_DESTROYED_NUM - 1 );
                    pHex->m_psprite = theTerrain.GetSprite( CHex::city, iIndex );
                    break;
                }
                default:
                    pHex->m_psprite = theTerrain.GetSprite(
                        pHex->GetVisibleType( ), RandNum( theTerrain.GetCount( pHex->GetVisibleType( ) ) - 1 ) );
                    break;
                }
            }
        }
    }

    InvalidateWindow( );
}

void CWndArea::OnLButtonDown( UINT nFlags, CPoint point )
{

    ASSERT_STRICT_VALID( this );

    // if dir != 0 -> switch to proper x, y
    CHexCoord hexcoord = m_aa.WindowToHex( point );

    switch ( m_iMode )
    {
    case normal:
        m_ptLMB = point;
        m_iMode = normal_select;

        CaptureMouse( );
        m_selOrig = point;
        m_selRect.SetRectEmpty( );

        break;

    // set these up so we know the down came from here
    case build_ready:
        m_iMode = build_loc;
        break;
    case rocket_ready:
        m_iMode = rocket_pos;
        break;

    // set start of a road
    case road_begin:
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        CaptureMouse( );
        m_hexRoadStart = hexcoord;
        m_iMode        = road_set;
        ClrRoadIcons( );
        ::SetCursor( m_hCurRoadSet[m_aa.m_iZoom] );
        return;
    }

    // nothing to do
    CWndAnim::OnLButtonDown( nFlags, point );
}

// FIXIT: convert

void CWndArea::GetPanAndVol( CUnit const* pUnit, int& iPan, int& iVol )
{

    if ( ( m_cx <= 0 ) || ( m_cy <= 0 ) )
    {
        TRAP( );
        iPan = iVol = 0;
        return;
    }

    CPoint ptUnit = m_aa.WrapWorldToWindow( CMapLoc3D( pUnit->GetWorldPixels( ) ) );

    iVol = 100;

    // off the top?
    if ( ptUnit.y < 0 )
        // note - this is a subtraction because ptUnit.y < 0
        iVol = 100 + ( ptUnit.y * 100 ) / m_cy;

    // off the bottom
    if ( ptUnit.y >= m_cy )
        iVol = 100 - ( ( ptUnit.y - m_cy ) * 100 ) / m_cy;

    // off to the left?
    if ( ptUnit.x < 0 )
    {
        iPan = 0;
        // note - this is a subtraction because ptUnit.x < 0
        iVol = __min( iVol, 100 + ( ptUnit.x * 100 ) / m_cx );
        return;
    }

    // off to the right?
    if ( ptUnit.x >= m_cx )
    {
        iPan = 127;
        iVol = __min( iVol, 100 - ( ( ptUnit.x - m_cx ) * 100 ) / m_cx );
        return;
    }

    iPan = 128 - ( ( m_cx - ptUnit.x ) * 128 ) / m_cx;
}

void CWndArea::SetDestAndSfx( CVehicle* pVeh, CHexCoord const& hex )
{

    if ( ( theMusicPlayer.SoundsPlaying( ) ) && ( pVeh->GetRouteMode( ) == CVehicle::stop ) )
    {
        int iPan, iVol;
        GetPanAndVol( pVeh, iPan, iVol );
        theMusicPlayer.PlayForegroundSound( pVeh->GetData( )->GetSoundGo( ), iPan, iVol );
    }

    pVeh->SetDest( hex );
}

void CWndArea::SetDestAndSfx( CVehicle* pVeh, CSubHex const& sub )
{

    if ( ( theMusicPlayer.SoundsPlaying( ) ) && ( pVeh->GetRouteMode( ) == CVehicle::stop ) )
    {
        int iPan, iVol;
        GetPanAndVol( pVeh, iPan, iVol );
        theMusicPlayer.PlayForegroundSound( pVeh->GetData( )->GetSoundGo( ), iPan, iVol );
    }

    pVeh->SetDest( sub );
}

void CWndArea::OnLButtonUp( UINT nFlags, CPoint point )
{

    ASSERT_STRICT_VALID( this );

    ReleaseMouse( );

    // in case from a road
    ClrRoadIcons( );

    CSubHex _sub = m_aa.WindowToSubHex( point );
    _sub.Wrap( );
    CHexCoord hex( _sub );

    switch ( m_iMode )
    {
    // route goto?
    case veh_route: {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        ASSERT_STRICT_VALID( m_pUnit );
        ASSERT_STRICT( m_pUnit->GetUnitType( ) == CUnit::vehicle );
        ( (CVehicle*)m_pUnit )->SetLocation( hex, m_posRoute, m_iRouteType );
        if ( ( (CVehicle*)m_pUnit )->m_pWndRoute != NULL )
            ( ( (CVehicle*)m_pUnit )->m_pWndRoute )->NewRoute( (CVehicle*)m_pUnit );

        // may now allow resume
        SetButtonState( );
        return;
    }

    // build
    case build_loc:
    case rocket_pos: {
        ASSERT_STRICT( ( 0 < m_iBuild ) && ( m_iBuild <= theStructures.GetNumBuildings( ) ) );
        if ( ( nFlags & MK_SHIFT ) && ( m_pUnit != NULL ) && ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) )
        {
            TRAP( );  // BUGBUG - check what this does
            ( (CVehicle*)m_pUnit )->SetEvent( CVehicle::none );
            m_iMode = build_ready;
            return;
        }

        hex = ToBuildUL( hex );

        // make sure not on water or another city
        if ( m_iFound < 0 )
        {
        bad_loc:
            theGame.Event( m_iMode == rocket_pos ? EVENT_ROCKET_CANT : EVENT_CONST_CANT, EVENT_BAD, m_iBuild );
            m_iMode = ( m_iMode == rocket_pos ) ? rocket_ready : build_ready;
            return;
        }

        if ( m_iMode == rocket_pos )
        {
            // we test to make sure that all vehicles can get out of the rocket
            if ( !CStructureData::CanBuild( hex, GetBuildDir( ), CStructureData::rocket, FALSE, TRUE ) )
                goto bad_loc;

            // it must all be visible and no vehicle under
            BOOL                  bOk   = TRUE;
            CStructureData const* pData = theStructures.GetData( m_iBuild );
            theMap.EnumHexes( hex, GetBuildDir( ) & 1 ? pData->GetCY( ) : pData->GetCX( ),
                              GetBuildDir( ) & 1 ? pData->GetCX( ) : pData->GetCY( ), fnEnumIsVisNoVeh, &bOk );
            if ( !bOk )
                goto bad_loc;

            theMusicPlayer.PlayForegroundSound( SOUNDS::GetID( SOUNDS::rocket_landing ), SFXPRIORITY::selected_pri );

            theGame.Event( EVENT_ROCKET_CANT, EVENT_OFF );
            CString sMsg;
            sMsg.LoadString( IDS_MSG_ROCKET_WAIT );
            SetStatusText( sMsg );
            CMsgPlaceBldg msg( hex, GetBuildDir( ), CStructureData::rocket );
            msg.m_iPlyrNum = theGame.GetMe( )->GetPlyrNum( );
            msg.m_bShow    = theApp.IsShareware( );
            theGame.PostToServer( &msg, sizeof( msg ) );
            BldgCurOff( );
            m_iMode = rocket_wait;
            ::SetCursor( m_hCurStart );
            return;
        }

        // had GPF of no crane
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) ||
             ( ( (CVehicle*)m_pUnit )->GetData( )->GetType( ) != CTransportData::construction ) )
        {
            TRAP( );
            SetButtonState( );
            BldgCurOff( );
            ::SetCursor( m_hCurStart );
            SelectOff( );
            return;
        }

        // send the vehicle there
        // find the closest surrounding hex
        theGame.Event( EVENT_CONST_CANT, EVENT_OFF );

        CHexCoord hexDest( hex );
        BuildBldgDest( (CVehicle*)m_pUnit, m_iBuild, GetBuildDir( ), hexDest );

        // lets build here
        m_pUnit->ResumeUnit( );
        ( (CVehicle*)m_pUnit )->SetBuilding( hex, m_iBuild, GetBuildDir( ) );
        ( (CVehicle*)m_pUnit )->SetEvent( CVehicle::build );
        ( (CVehicle*)m_pUnit )->SetDestAndMode( hexDest, CVehicle::full );

        // we loose selection of the crane so we don't change the orders
        m_lstUnits.RemoveAllUnits( TRUE );
        m_pUnit = NULL;
        SetButtonState( );
        BldgCurOff( );
        ::SetCursor( m_hCurStart );
        SelectOff( );
        return;
    }

    // selecting unit(s)
    case normal_select: {
        m_iMode = normal;

        CHitInfo     hitinfo = m_aa.GetHit( point );
        CUnit*       pUnitOn = hitinfo.GetUnit( );
        CBridgeUnit* pBu     = hitinfo.GetBridge( );
        // if not visible then it's not there
        if ( pUnitOn != NULL )
            if ( !pUnitOn->IsVisible( ) )
                pUnitOn = NULL;

        CUnit* pPrevSel = m_pUnit;

        ASSERT_STRICT_VALID_OR_NULL( pUnitOn );
        BOOL bSelected = FALSE;

        // step 1 - if ours & not shift or ctrl - deselect all
        //   and not loading on a carrier or repairing
        if ( ( ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) == 0 ) && ( m_uMouseMode == lmb_select ) )
        {
            m_lstUnits.RemoveAllUnits( TRUE );
            m_pUnit = NULL;
        }

        // step 2 - if we dragged then we are selecting
        if ( ( abs( point.x - m_ptLMB.x ) >= theMap.HexWid( m_aa.m_iZoom ) / 2 ) ||
             ( abs( point.y - m_ptLMB.y ) >= theMap.HexHt( m_aa.m_iZoom ) / 2 ) )
        {
            if ( ( nFlags & MK_SHIFT ) == 0 )
            {
                m_lstUnits.RemoveAllUnits( TRUE );
                m_pUnit = NULL;
            }
            bSelected = TRUE;

            // get all units in the box
            POSITION pos;
            pos                 = theVehicleMap.GetStartPosition( );
            int       iNumInBox = 0;
            CVehicle* pVehLastInBox;
            while ( pos != NULL )
            {
                DWORD     dwID;
                CVehicle* pVeh;
                theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                ASSERT_STRICT_VALID( pVeh );
                if ( pVeh->GetOwner( )->IsMe( ) )
                {
                    CPoint pt = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( pVeh->GetWorldPixels( ) ) );
                    if ( ( m_selRect.PtInRect( pt ) ) && ( pVeh->GetTransport( ) == NULL ) )
                    {
                        iNumInBox++;
                        pVehLastInBox = pVeh;
                        // if not ctrl then only combat vehicles
                        if ( ( ( nFlags & MK_CONTROL ) != 0 ) ||
                             ( !( pVeh->GetData( )->GetVehFlags( ) & CTransportData::FLcivilian ) ) )
                            m_lstUnits.AddUnit( pVeh, TRUE );
                    }
                }
#ifdef _CHEAT
                else if ( _bClickAny )
                {
                    CPoint pt = m_aa.WrapWorldToWindow( m_aa.WorldToCenterWorld( pVeh->GetWorldPixels( ) ) );
                    if ( m_selRect.PtInRect( pt ) )
                        m_lstUnits.AddUnit( pVeh, TRUE );
                }
#endif
            }

            // if we got none and there was only 1 - then we select it (drag to get a lone crane)
            if ( ( iNumInBox == 1 ) && ( m_lstUnits.GetCount( ) == 0 ) )
                m_lstUnits.AddUnit( pVehLastInBox, TRUE );

            // if we got any then ignore what we are over
            if ( m_lstUnits.GetCount( ) >= 0 )
                pUnitOn = NULL;
        }

        // step 3 - if ours and not force - toggle it's selection
        //   OR un/load carrier /or/ send to repair facility
        if ( ( pBu != NULL ) ||
             ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->IsMe( ) ) && ( !( nFlags & MK_CONTROL ) ) ) )
        {
            // if we have SHIFT down we just select - none of the special stuff
            if ( ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) == 0 )
            {
                // repair bldg?
                if ( ( m_uMouseMode == lmb_repair_bldg ) &&
                     ( ( pBu != NULL ) || ( pUnitOn->GetUnitType( ) == CUnit::building ) ) )
                {
                    CHexCoord _hexDest;
                    if ( pBu != NULL )
                        _hexDest = pBu->GetHex( );
                    else
                        _hexDest = ( (CBuilding*)pUnitOn )->GetExitHex( );
                    POSITION pos;
                    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                    {
                        POSITION prev_pos = pos;
                        CUnit*   pUnit    = m_lstUnits.GetNext( pos );
                        ASSERT_STRICT_VALID( pUnit );
                        if ( ( pUnit->GetUnitType( ) == CUnit::vehicle ) &&
                             ( ( (CVehicle*)pUnit )->GetData( )->IsCrane( ) ) )
                        {
                            pUnit->ResumeUnit( );
                            ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_bldg );
                            SetDestAndSfx( (CVehicle*)pUnit, _hexDest );

                            // deselect it if it's going to be repaired
                            pUnit->SetUnselected( TRUE );
                            m_lstUnits.RemoveAt( prev_pos );
                        }
                    }
                    goto done_step_3;
                }

                // repair self?
                if ( ( m_uMouseMode == lmb_repair_self ) && ( pUnitOn->GetUnitType( ) == CUnit::building ) )
                {
                    POSITION pos;
                    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                    {
                        POSITION prev_pos = pos;
                        CUnit*   pUnit    = m_lstUnits.GetNext( pos );
                        ASSERT_STRICT_VALID( pUnit );
                        if ( ( pUnit->GetUnitType( ) == CUnit::vehicle ) &&
                             ( ( (CVehicle*)pUnit )->GetData( )->IsRepairable( ) ) )
                        {
                            pUnit->ResumeUnit( );
                            ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_self );
                            if ( ( (CVehicle*)pUnit )->GetData( )->IsBoat( ) )
                                SetDestAndSfx( (CVehicle*)pUnit, ( (CBuilding*)pUnitOn )->GetShipHex( ) );
                            else
                                SetDestAndSfx( (CVehicle*)pUnit, ( (CBuilding*)pUnitOn )->GetExitHex( ) );

                            // deselect it if it's going to be repaired
                            pUnit->SetUnselected( TRUE );
                            m_lstUnits.RemoveAt( prev_pos );
                        }
                    }
                    goto done_step_3;
                }

                // load?
                if ( ( m_uMouseMode == lmb_load ) && ( ( (CVehicle*)pUnitOn )->GetData( )->IsCarrier( ) ) )
                {
                    CSubHex _sub;
                    if ( ( (CVehicle*)pUnitOn )->GetData( )->GetVehFlags( ) & CTransportData::FLload_front )
                        _sub = ( (CVehicle*)pUnitOn )->GetPtHead( );
                    else
                        _sub = ( (CVehicle*)pUnitOn )->GetPtTail( );

                    POSITION pos;
                    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                    {
                        CUnit* pUnit = m_lstUnits.GetNext( pos );
                        ASSERT_STRICT_VALID( pUnit );
                        if ( ( ( pUnit->GetUnitType( ) == CUnit::vehicle ) &&
                               ( ( (CVehicle*)pUnit )->GetData( )->IsCarryable( ) ) ) ||
                             ( ( ( (CVehicle*)pUnitOn )->GetData( )->IsBoat( ) ) &&
                               ( ( (CVehicle*)pUnit )->GetData( )->IsLcCarryable( ) ) ) )
                        {
                            pUnit->ResumeUnit( );
                            ( (CVehicle*)pUnit )->SetEvent( CVehicle::load );
                            SetDestAndSfx( (CVehicle*)pUnit, _sub );
                            ( (CVehicle*)pUnit )->SetLoadOn( (CVehicle*)pUnitOn );

                            // deselect it
                            m_lstUnits.RemoveUnit( pUnit );
                        }
                    }
                    goto done_step_3;
                }

                // unload?
                if ( ( m_uMouseMode == lmb_unload ) && ( pPrevSel ) && ( pPrevSel->GetUnitType( ) == CUnit::vehicle ) &&
                     ( ( (CVehicle*)pPrevSel )->GetCargoCount( ) > 0 ) )
                {
                    CMsgUnloadCarrier _msg( (CVehicle*)pUnitOn );
                    theGame.PostToClient( theGame.GetMe( ), &_msg, sizeof( _msg ) );
                    goto done_step_3;
                }
            }  // (CONTROL | SHIFT) == 0

            // regular command
            if ( m_uMouseMode == lmb_select )
            {
                bSelected = TRUE;
                if ( pUnitOn->GetFlags( ) & CUnit::selected )
                    m_lstUnits.RemoveUnit( pUnitOn );
                else
                    m_lstUnits.AddUnit( pUnitOn, TRUE );
            }
        }
    done_step_3:;

        // we want to spread out the vehicles dest if there are a lot of them
        //   (unless we're going to a unit)
        int     iDestRand = 0;
        CSubHex _subDest( _sub );
        // see if going to a bridge
        if ( pBu != NULL )
            _subDest = pBu->GetHex( );

        if ( ( pUnitOn == NULL ) && ( m_lstUnits.GetCount( ) > 2 ) )
        {
            iDestRand = (int)sqrt( (float)m_lstUnits.GetCount( ) ) + 1;
            _subDest.x -= iDestRand;
            _subDest.y -= iDestRand;
            _subDest.Wrap( );
            iDestRand *= 2;
        }

        // have to have units selected to do something
        if ( ( !bSelected ) && ( m_uMouseMode == lmb_attack ) && ( pUnitOn != NULL ) )
        {
            ASSERT( ( !bSelected ) && ( m_lstUnits.GetCount( ) > 0 ) );
            ASSERT( ( ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) == ( MK_CONTROL | MK_SHIFT ) ) ||
                    ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->GetRelations( ) >= RELATIONS_NEUTRAL ) ) );
            POSITION pos;
            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                pUnit->ResumeUnit( );

                // if it can attack we set it to attack. Otherwise we set it to go there
                if ( pUnit->GetData( )->_GetFireRate( ) > 0 )
                {
                    // get it going if too far away or dest not visible
                    if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                    {
                        CVehicle* pVeh = ( (CVehicle*)pUnit );
                        pVeh->TempTargetOff( );
                        CSubHex _subAtk;
                        // get closest point
                        if ( pUnitOn->GetUnitType( ) == CUnit::vehicle )
                            _subAtk = ( (CVehicle*)pUnitOn )->GetPtHead( );
                        else if ( pUnitOn->GetUnitType( ) == CUnit::building )
                        {
                            CHexCoord _hex;
                            _hex = pVeh->GetPtHead( );

                            CBuilding* pBldg = (CBuilding*)pUnitOn;
                            if ( _hex.X( ) < pBldg->GetHex( ).X( ) )
                                _hex.X( ) = pBldg->GetHex( ).X( ) - 1;
                            else if ( _hex.X( ) > pBldg->GetHex( ).X( ) + pBldg->GetCX( ) )
                                _hex.X( ) = pBldg->GetHex( ).X( ) + pBldg->GetCX( );
                            if ( _hex.Y( ) < pBldg->GetHex( ).Y( ) )
                                _hex.Y( ) = pBldg->GetHex( ).Y( ) - 1;
                            else if ( _hex.Y( ) > pBldg->GetHex( ).Y( ) + pBldg->GetCY( ) )
                                _hex.Y( ) = pBldg->GetHex( ).Y( ) + pBldg->GetCY( );
                            _hex.Wrap( );
                            _subAtk = _hex;
                        }

                        // if not visible - go toward it
                        if ( theMap._GetHex( _subAtk )->GetVisible( ) == 0 )
                            SetDestAndSfx( pVeh, _subAtk );
                        else
                        {
                            // too far away - go for it
                            int iLOS = theMap.LineOfSight( pVeh, pUnitOn );
                            if ( ( ( iLOS < 0 ) &&
                                   ( pVeh->GetData( )->GetBaseType( ) != CTransportData::artillery ) ) ||
                                 ( abs( iLOS ) > pVeh->GetRange( ) - 1 ) )
                                SetDestAndSfx( pVeh, _subAtk );
                        }
                    }

                    pUnit->MsgSetTarget( pUnitOn );
                    CDlgRelations::NewRelations( pUnitOn->GetOwner( ), RELATIONS_WAR );
                }

                else if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                {
                    CVehicle* pVeh = ( (CVehicle*)pUnit );
                    pVeh->SetEvent( CVehicle::none );
                    CSubHex _subVeh( _subDest.x + RandNum( iDestRand ), _subDest.y + RandNum( iDestRand ) );
                    _subVeh.Wrap( );
                    int iCost = theMap.GetTerrainCost( _subVeh, _subVeh, 0, pVeh->GetData( )->GetWheelType( ) );
                    if ( ( iCost == 0 ) ||
                         ( iCost > theMap.GetTerrainCost( _sub, _sub, 0, pVeh->GetData( )->GetWheelType( ) ) * 2 ) )
                        _subVeh = _sub;
                    SetDestAndSfx( pVeh, _subVeh );
                }
            }
        }

        // step 5 - a goto?
        else if ( ( !bSelected ) && ( m_uMouseMode == lmb_goto ) )
        {
#ifndef _GG
            ASSERT( ( ( nFlags & ( MK_CONTROL | MK_SHIFT ) ) == MK_CONTROL ) || ( pUnitOn == NULL ) ||
                    ( pUnitOn->GetOwner( )->IsMe( ) ) );
#endif

            // if we have 1 unit or are going to a building - send them direct
            BOOL bDestIsBldg = theBuildingHex._GetBuilding( _sub ) != NULL;
            if ( ( m_lstUnits.GetCount( ) <= 1 ) || bDestIsBldg )
            {
                POSITION pos;
                for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                {
                    CUnit* pUnit = m_lstUnits.GetNext( pos );
                    ASSERT_STRICT_VALID( pUnit );
                    if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                    {
                        CVehicle* pVeh = ( (CVehicle*)pUnit );

                        // send it
                        pVeh->TempTargetOff( );
                        pVeh->SetEvent( CVehicle::none );
                        pVeh->ResumeUnit( );
                        SetDestAndSfx( pVeh, _sub );
                        pVeh->_SetTarget( NULL );

                        // goto building to pick up goods
                        if ( ( pVeh->GetData( )->IsTransport( ) ) && bDestIsBldg )
                        {
                            if ( !pVeh->IsHpControl( ) )
                            {
                                theGame.m_pHpRtr->MsgTakeVeh( (CVehicle*)pUnit );
                                pVeh->HpControlOn( );
                            }
                        }
                    }
                }
            }
            else

            // we want to hold formation but maybe bring it in and add some randomnesses
            {
                // so first we find the center of where we are
                int      x = 0, y = 0;
                POSITION pos;
                for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                {
                    CUnit* pUnit = m_lstUnits.GetNext( pos );
                    ASSERT_STRICT_VALID( pUnit );
                    x += pUnit->GetWorldPixels( ).x;
                    y += pUnit->GetWorldPixels( ).y;
                }
                CSubHex _subSrc( CMapLoc( x / m_lstUnits.GetCount( ), y / m_lstUnits.GetCount( ) ) );

                // now we find the furthest away from that center (for proportional dist at dest)
                int xMaxDist = 1, yMaxDist = 1;
                for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                {
                    CUnit* pUnit = m_lstUnits.GetNext( pos );
                    if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                    {
                        CVehicle* pVeh  = ( (CVehicle*)pUnit );
                        int       iDist = abs( _subSrc.x - pVeh->GetPtHead( ).x );
                        xMaxDist        = __max( xMaxDist, iDist );
                        iDist           = abs( _subSrc.y - pVeh->GetPtHead( ).y );
                        yMaxDist        = __max( yMaxDist, iDist );
                    }
                }

                // and the furthest we want them apart is
                int iDestDist = (int)sqrt( (float)m_lstUnits.GetCount( ) ) + 1;
                iDestDist += iDestDist / 2;
                int iRandDist = iDestDist / 4;

                for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                {
                    CUnit* pUnit = m_lstUnits.GetNext( pos );
                    ASSERT_STRICT_VALID( pUnit );
                    if ( pUnit->GetUnitType( ) == CUnit::vehicle )
                    {
                        CVehicle* pVeh = ( (CVehicle*)pUnit );

                        CSubHex _subVeh;
                        _subVeh.x = _sub.x +
                                    ( ( pVeh->GetPtHead( ).x - _subSrc.x ) * iDestDist + xMaxDist / 2 ) / xMaxDist +
                                    RandNum( iRandDist ) - iRandDist / 2;
                        _subVeh.y = _sub.y +
                                    ( ( pVeh->GetPtHead( ).y - _subSrc.y ) * iDestDist + yMaxDist / 2 ) / yMaxDist +
                                    RandNum( iRandDist ) - iRandDist / 2;
                        _subVeh.Wrap( );
                        int iCost    = theMap.GetTerrainCost( _subVeh, _subVeh, 0, pVeh->GetData( )->GetWheelType( ) );
                        int iMaxCost = 3 * theMap.GetTerrainCost( _sub, _sub, 0, pVeh->GetData( )->GetWheelType( ) );
                        if ( ( iCost == 0 ) || ( iCost > iMaxCost ) )
                            _subVeh = _sub;

                        // send it
                        pVeh->TempTargetOff( );
                        pVeh->SetEvent( CVehicle::none );
                        pVeh->ResumeUnit( );
                        SetDestAndSfx( pVeh, _subVeh );
                        pVeh->_SetTarget( NULL );

                        // goto building to pick up goods
                        if ( ( pVeh->GetData( )->IsTransport( ) ) && bDestIsBldg )
                        {
                            if ( !pVeh->IsHpControl( ) )
                            {
                                theGame.m_pHpRtr->MsgTakeVeh( pVeh );
                                pVeh->HpControlOn( );
                            }
                        }
                    }
                }
            }
        }

        // if just one select it
        // set up the status bar
        if ( m_lstUnits.GetCount( ) == 1 )
            m_pUnit = m_lstUnits.GetHead( );
        else
            m_pUnit = NULL;

        // set button states
        SetButtonState( );

        // repaint it
        InvalidateStatus( );
        InvalidateSound( );

        // voices?
        // say ok
        if ( ( !bSelected ) && ( ( m_uMouseMode == lmb_attack ) || ( m_uMouseMode == lmb_goto ) ) )
        {
            if ( m_uFlags & crane )
                theGame.MulEvent( MEVENT_GO_CRANE, m_pUnit );
            else if ( m_uFlags & veh )
                theGame.MulEvent( MEVENT_GO_COMBAT, m_pUnit );
        }
        else if ( ( bSelected ) && ( m_lstUnits.GetCount( ) > 0 ) )
        {
            if ( m_uFlags & crane )
                theGame.MulEvent( MEVENT_SELECT_CRANE, m_pUnit );
            else if ( m_uFlags & veh )
                theGame.MulEvent( MEVENT_SELECT_COMBAT, m_pUnit );
            if ( m_uFlags & fac )
                theGame.MulEvent( MEVENT_SELECT_FACTORY, m_pUnit );

            // building trigger
            if ( ( theMusicPlayer.SoundsPlaying( ) ) && ( m_pUnit != NULL ) &&
                 ( m_pUnit->GetUnitType( ) == CUnit::building ) && ( !( (CBuilding*)m_pUnit )->IsConstructing( ) ) )
            {
                int iSound = m_pUnit->IsFlag( ( CUnit::UNIT_FLAGS )( CUnit::stopped | CUnit::event |
                                                                     CUnit::repair_stop | CUnit::abandoned ) )
                                 ? m_pUnit->GetData( )->GetSoundIdle( )
                                 : m_pUnit->GetData( )->GetSoundRun( );
                int iPan, iVol;
                GetPanAndVol( m_pUnit, iPan, iVol );
                theMusicPlayer.PlayForegroundSound( iSound, iPan, iVol );
            }
        }

        break;
    }

    case road_set: {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        SelectOff( );
        if ( nFlags & MK_SHIFT )
        {
            SetButtonState( );
            return;
        }

        CHexCoord hex = m_aa.WindowToHex( point );

        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            SetDestAndSfx( (CVehicle*)pUnit, m_hexRoadStart );
            ( (CVehicle*)pUnit )->SetRoad( m_hexRoadStart, hex );
        }

        // deselect all
        m_lstUnits.RemoveAllUnits( TRUE );
        m_pUnit = NULL;
        SetButtonState( );
        InvalidateSound( );
        break;
    }

    case repair_bldg: {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
            break;

        SelectOff( );

        CHexCoord hex = m_aa.WindowToHex( point );
        hex.Wrap( );
        CBuilding* pBldg = theBuildingHex._GetBuilding( hex );

        if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) )
        {
            POSITION pos;
            for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
            {
                CUnit* pUnit = m_lstUnits.GetNext( pos );
                ASSERT_STRICT_VALID( pUnit );
                if ( ( (CVehicle*)pUnit )->GetData( )->GetType( ) == CTransportData::construction )
                {
                    pUnit->ResumeUnit( );
                    ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_bldg );
                    SetDestAndSfx( (CVehicle*)pUnit, hex );
                }
            }
        }

        // deselect all
        m_lstUnits.RemoveAllUnits( TRUE );
        m_pUnit = NULL;
        SetButtonState( );
        InvalidateSound( );
        break;
    }

    default:
        //			CWndAnim::OnLButtonUp(nFlags, point);
        break;
    }
}

typedef struct tagCLOSEST_HEX
{
    CTransportData const* pTd;
    int                   iTime;
    int                   iWheelType;
    int                   eX;
    int                   eY;
    CHexCoord             hexAt;
    CHexCoord             hexBldg;
    CHexCoord             hexClosest;
} CLOSEST_HEX;

static int fnEnumClosestHex( CHex* pHex, CHexCoord hex, void* pData )
{

    CLOSEST_HEX* pCh = (CLOSEST_HEX*)pData;
    if ( !pCh->pTd->CanTravelHex( pHex ) )
        return ( FALSE );

    int iTime = theMap.GetRangeDistance( pCh->hexAt, hex ) + theMap.GetTerrainCost( hex, hex, 0, pCh->iWheelType );

    if ( iTime < pCh->iTime )
    {
        // we only allow hexes on the edge
        if ( ( hex.X( ) == pCh->hexBldg.X( ) ) || ( hex.Y( ) == pCh->hexBldg.Y( ) ) ||
             ( hex.X( ) == pCh->hexBldg.X( ) + pCh->eX ) || ( hex.Y( ) == pCh->hexBldg.Y( ) + pCh->eY ) )
        {
            pCh->hexClosest = hex;
            pCh->iTime      = iTime;
            if ( pCh->hexAt == hex )
                return ( TRUE );
        }
    }

    return ( FALSE );
}

void BuildBldgDest( CVehicle* pVeh, int iBldg, int iDir, CHexCoord& hex )
{

    ASSERT_STRICT_VALID( pVeh );
    CLOSEST_HEX ch;
    ch.iTime      = INT_MAX;
    ch.iWheelType = pVeh->GetData( )->GetWheelType( );
    ch.hexAt      = pVeh->GetHexHead( );
    ch.eX         = iDir & 1 ? theStructures.GetData( iBldg )->GetCY( ) : theStructures.GetData( iBldg )->GetCX( );
    ch.eY         = iDir & 1 ? theStructures.GetData( iBldg )->GetCX( ) : theStructures.GetData( iBldg )->GetCY( );
    ch.hexBldg = ch.hexClosest = CHexCoord( hex.X( ), hex.Y( ) );
    ch.hexBldg.Wrap( );
    ch.pTd = pVeh->GetData( );

    theMap.EnumHexes( CHexCoord( hex.X( ), hex.Y( ) ), ch.eX, ch.eY, fnEnumClosestHex, &ch );
    hex = ch.hexClosest;
}

void CWndArea::SetupStart( )
{

    ASSERT_STRICT_VALID( this );

    m_iMode     = rocket_ready;
    m_iBuild    = CStructureData::rocket;
    m_iBuildDir = ( theStructures.GetData( CStructureData::rocket )->GetExitDir( ) - 2 ) & 0x03;

    ::SetCursor( NULL );
    CString sMsg;
    sMsg.LoadString( IDS_MSG_ROCKET_START );
    SetStatusText( sMsg );

    // start with resources showing
    ResClicked( );

    if ( ( theApp.m_pdlgFile != NULL ) && ( theApp.m_pdlgFile->m_hWnd != NULL ) )
        theApp.m_pdlgFile->SetState( );
}

void CWndArea::SetupDone( )
{

    ASSERT_STRICT_VALID( this );

    BldgCurOff( );
    CString sMsg;
    sMsg.LoadString( IDS_MSG_ROCKET_DONE );
    SetStatusText( sMsg );
    InvalidateStatus( );

    if ( ( theApp.m_pdlgFile != NULL ) && ( theApp.m_pdlgFile->m_hWnd != NULL ) )
        theApp.m_pdlgFile->SetState( );
}

void CWndArea::SelectOff( )
{

    ASSERT_STRICT_VALID( this );

    BldgCurOff( );

    m_uFlags     = 0;
    m_uMouseMode = lmb_nothing;
    m_iMode      = normal;
    m_iBuild     = 0;

    SetStatusText( "" );

    theGame.Event( EVENT_CONST_LOC, EVENT_OFF );

    InvalidateStatus( );
}

void CWndArea::BuildOn( int iIndex )
{

    if ( m_pUnit == NULL )
    {
        ASSERT( FALSE );
        return;
    }

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );
    ASSERT_STRICT( ( 0 <= iIndex ) && ( iIndex < theStructures.GetNumBuildings( ) ) );

    // for events
    ( (CVehicle*)m_pUnit )->SetEvent( CVehicle::none );
    ( (CVehicle*)m_pUnit )->SetBldgType( iIndex );

    CStructureData const* pData = theStructures.GetData( iIndex );

    CString sText = m_pUnit->GetData( )->GetDesc( ) + " - [" + pData->GetDesc( ) + "]";
    SetWindowText( sText );

    theGame.Event( EVENT_CONST_LOC, EVENT_NOTIFY, m_pUnit );

    m_iMode = build_ready;
    ::SetCursor( NULL );
    m_iBuild    = iIndex;
    m_iBuildDir = ( pData->GetExitDir( ) - 2 ) & 0x03;
    SetButtonState( );

    CPoint point;
    ::GetCursorPos( &point );
    ScreenToClient( &point );
    OnMouseMove( 0, point );
}

void CWndArea::GotoOn( CVehicle* pUnit, int iMode, int iRouteType, POSITION posRoute )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT( m_pUnit == pUnit );
    ASSERT_STRICT_VALID( pUnit );
    ASSERT_STRICT( iMode == veh_route );

    CString sMsg;
    sMsg.LoadString( IDS_MSG_UNIT_GOTO );
    csPrintf( &sMsg, (char const*)pUnit->GetData( )->GetDesc( ) );
    SetStatusText( sMsg );

    m_lstUnits.RemoveAllUnits( TRUE );
    m_lstUnits.AddUnit( pUnit, TRUE );
    m_pUnit = pUnit;

    m_iMode      = iMode;
    m_iRouteType = iRouteType;
    m_posRoute   = posRoute;

    SetButtonState( );

    ::SetCursor( m_hCurGoto[m_aa.m_iZoom] );
}

void CWndArea::Center( CMapLoc maploc )
{
    m_aa.SetCenter( maploc, CAnimAtr::SET_CENTER_SCROLL );

    ASSERT_STRICT_VALID( this );

    m_bUpdateAll = TRUE;
    // GGTESTING InvalidateWindow ();

    theApp.m_wndWorld.NewLocation( );
}

void CWndArea::Center( CUnit* pUnit )
{
    Center( pUnit->GetWorldPixels( ) );

    // GGTESTING	m_aa.SetCenter( pUnit->GetWorldPixels () );

    ASSERT_STRICT_VALID( this );

    // GGTESTING	InvalidateWindow ();
    theApp.m_wndWorld.NewLocation( );
}

void CWndArea::LastCombat( )
{

    Center( theAreaList.GetLastAttack( ) );
}

void CWndArea::CenterUnit( )
{
    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // if no units center on rocket
    if ( m_lstUnits.GetCount( ) <= 0 )
    {
        CBuilding* pBldg = theBuildingMap.GetBldg( theGame.GetMe( )->m_dwIDRocket );
        if ( pBldg != NULL )
            Center( pBldg->GetWorldPixels( ) );
        return;
    }

    // average all selected
    int      x = 0, y = 0;
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        x += pUnit->GetWorldPixels( ).x;
        y += pUnit->GetWorldPixels( ).y;
    }

    Center( CMapLoc( x / m_lstUnits.GetCount( ), y / m_lstUnits.GetCount( ) ) );

    // GGTESTING	InvalidateWindow ();
}

void CWndArea::DestroyUnit( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // destroy all selected
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        pUnit->SetDestroyUnit( );
    }
}

void CWndArea::StopDestroyUnit( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // destroy all selected
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        pUnit->StopDestroyUnit( );
    }
}

void CWndArea::OppoUnit( )
{

    ASSERT_STRICT_VALID( this );
    ASSERT_STRICT_VALID( m_pUnit );

    // set all selected to oppo fire
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        pUnit->_SetTarget( NULL );
        pUnit->SetOppo( NULL );
    }
}

void CWndArea::OnRButtonDown( UINT nFlags, CPoint point )
{

    // if its CTRL & a building we change it's facing
    if ( nFlags & MK_CONTROL )
    {
        if ( ( m_iMode == build_ready ) || ( m_iMode == rocket_ready ) )
        {
            m_iBuildDir = ( m_iBuildDir - 1 ) & 0x03;
            SetMouseState( );
            OnMouseMove( nFlags, point );
        }
        return;
    }

    if ( nFlags & MK_SHIFT )
    {
        CHitInfo hitinfo = m_aa.GetHit( point );
        CUnit*   pUnitOn = hitinfo.GetUnit( );
        ASSERT_STRICT_VALID_OR_NULL( pUnitOn );
        if ( pUnitOn == NULL )
            return;

#ifdef _CHEAT
        if ( !_bClickAny )
#endif
            // not us and not alliance
            if ( ( !pUnitOn->GetOwner( )->IsMe( ) ) &&
                 ( pUnitOn->GetOwner( )->GetTheirRelations( ) != RELATIONS_ALLIANCE ) )
                return;

        if ( m_pWndInfo == NULL )
            m_pWndInfo = new CWndInfo( );

        if ( m_pWndInfo->m_hWnd == NULL )
            m_pWndInfo->Create( point, pUnitOn, this );

        m_pWndInfo->ShowWindow( SW_SHOW );
        m_pWndInfo->UpdateWindow( );
        return;
    }

    // we are moving the window
    m_bRBtnDown = TRUE;
    m_ptRMB = m_ptRMDN = point;

    CaptureMouse( );
    theApp.m_wndBar.SetStatusText( 1, m_sHelpRMB );

    CRect rect;
    GetClientRect( &rect );

    // figure out the cursor
    CPoint pt;
    ::GetCursorPos( &pt );
    ScreenToClient( &pt );

    int x, y;
    int iWid = rect.Width( );
    if ( pt.x < iWid / 8 )
        x = -1;
    else
    {
        int iTmp = iWid - iWid / 8;
        if ( pt.x > iTmp )
            x = 1;
        else
            x = 0;
    }
    int iHt = rect.Height( );
    if ( pt.y < iHt / 8 )
        y = -1;
    else
    {
        int iTmp = iHt - iHt / 8;
        if ( pt.y > iTmp )
            y = 1;
        else
            y = 0;
    }

    // which cursor?
    switch ( ( x + 2 ) | ( ( y + 2 ) << 2 ) )
    {
    case 0x06:  // up
        m_iMoveCur = 0;
        break;
    case 0x07:  // UR
        m_iMoveCur = 1;
        break;
    case 0x0B:  // right
        m_iMoveCur = 2;
        break;
    case 0x0F:  // LR
        m_iMoveCur = 3;
        break;
    case 0x0E:  // down
        m_iMoveCur = 4;
        break;
    case 0x0D:  // LL
        m_iMoveCur = 5;
        break;
    case 0x09:  // left
        m_iMoveCur = 6;
        break;
    case 0x05:  // UL
        m_iMoveCur = 7;
        break;
    default:
        m_iMoveCur = 8;
        break;
    }

    ::SetCursor( m_hCurMove[m_iMoveCur] );

    ClientToScreen( &rect );
    ::ClipCursor( &rect );
}

void CWndArea::OnRButtonUp( UINT, CPoint )
{

    m_bRBtnDown = FALSE;
    ::ClipCursor( NULL );
    ReleaseMouse( );
    theApp.m_wndWorld.NewLocation( );

    if ( m_bNewPos )
        InvalidateSound( );

    theApp.m_wndBar.SetStatusText( 1, m_sHelp );
}

// center on this location on the screen
void CWndArea::OnRButtonDblClk( UINT nFlags, CPoint pt )
{

    // if we're modifying we don't do this
    if ( nFlags & ( MK_CONTROL | MK_SHIFT ) )
    {
        OnRButtonDown( nFlags, pt );
        return;
    }

    m_bRBtnDown = FALSE;
    ::ClipCursor( NULL );
    ReleaseMouse( );

    CHexCoord hexcoord = m_aa.WindowToHex( pt );

    Center( CMapLoc( hexcoord ) );

    ASSERT_STRICT_VALID( &theMap );
    // GGTESTING	InvalidateWindow ();

    m_bNewPos = TRUE;
}

void CWndArea::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
{

    // on loosing activation we give up the cursor
    if ( m_bRBtnDown )
        m_bRBtnDown = FALSE;
    ReleaseMouse( );
    ::ClipCursor( NULL );

    theApp.m_wndBldgs.m_ListBox.SetRedraw( FALSE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( FALSE );

    // if we loose activation we undo our selections
    if ( nState == WA_INACTIVE )
    {
        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            pUnit->SetUnselected( TRUE );
        }
    }

    // if we gain activation we set our selections
    else
    {
        POSITION pos;
        for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            ASSERT_STRICT_VALID( pUnit );
            pUnit->SetSelected( TRUE );
        }
        theApp.m_wndWorld.NewAreaMap( this );
    }

    theApp.m_wndBldgs.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL, FALSE );
    theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL, FALSE );

    CWndAnim::OnActivate( nState, pWndOther, bMinimized );
}

void CWndArea::OnLButtonDblClk( UINT nFlags, CPoint point )
{

    ReleaseMouse( );

    CHitInfo hitinfo = m_aa.GetHit( point );
    CUnit*   punit   = hitinfo.GetUnit( );
    // if not visible then it's not there
    if ( punit != NULL )
        if ( !punit->IsVisible( ) )
            punit = NULL;

    ASSERT_STRICT_VALID_OR_NULL( punit );

    // if ctrl is down we bring up a new window
    if ( nFlags & MK_CONTROL )
    {
        if ( theGame.GetMe( )->CanMultiArea( ) )
        {
            CWndArea* pWndArea = new CWndArea( );
            pWndArea->Create( hitinfo._GetHexCoord( ), punit, FALSE );
        }
        return;
    }

    if ( punit == NULL )
        return;

    // if it's not ours we return
    if ( !punit->GetOwner( )->IsMe( ) )
        return;

    // this has to be our selected unit to work
    if ( punit != m_pUnit )
        return;

    // we bring up the control window for this unit
    //   it's a building
    if ( punit->GetUnitType( ) == CUnit::building )
    {
        if ( ( (CBuilding*)punit )->IsConstructing( ) )
            return;

        switch ( ( (CBuilding*)punit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UTshipyard:
            ( (CVehicleBuilding*)punit )->GetDlgBuild( );
            return;
        case CStructureData::UTresearch:
            theApp.m_wndBar._GotoScience( );
            return;
        case CStructureData::UTembassy:
            theApp.m_wndBar.GotoRelations( );
            return;
        }

        // if a truck is in there (not on auto) bring up the load dialog
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( !pVeh->GetHexOwnership( ) ) &&
                 ( pVeh->GetData( )->IsTransport( ) ) && ( pVeh->IsHpControl( ) ) &&
                 ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == punit ) )
            {
                pVeh->GetDlgLoad( );
                break;
            }
        }

        return;
    }

    if ( punit->GetUnitType( ) != CUnit::vehicle )
        return;

    // it's a vehicle - we do crane & other (route)
    if ( ( (CVehicle*)punit )->GetData( )->IsCrane( ) )
    {
        ( (CVehicle*)punit )->GetDlgBuild( );
        return;
    }

    // if someone wants to route a non-truck - fine
    CVehicle* pVeh = (CVehicle*)punit;
    if ( pVeh->m_pWndRoute == NULL )
        pVeh->m_pWndRoute = new CWndRoute( pVeh );

    if ( pVeh->m_pWndRoute->m_hWnd == NULL )
        pVeh->m_pWndRoute->Create( this );

    pVeh->m_pWndRoute->ShowWindow( SW_SHOWNORMAL );
    pVeh->m_pWndRoute->SetFocus( );
    SetButtonState( );
}

void CWndArea::InvalidateWindow( RECT* )
{

    m_bUpdateAll = TRUE;

    m_aa.GetDirtyRects( )->AddRect( NULL );

    if ( !m_bRBtnDown )
        InvalidateSound( );

    theApp.m_wndWorld.NewLocation( );
}

void CWndArea::CancelBuildUnit( )
{

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        pUnit->CancelUnit( );
    }

    SelectOff( );
    SetButtonState( );
}

void CWndArea::StopUnit( )
{

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        pUnit->StopUnit( );
    }
    SetButtonState( );
}

void CWndArea::ResumeUnit( )
{

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );
        pUnit->ResumeUnit( );
    }
    SetButtonState( );
}

void CWndArea::BuildUnit( )
{

    ASSERT_STRICT_VALID( this );
    if ( m_pUnit == NULL )
    {
        ASSERT_STRICT( FALSE );
        return;
    }
    ASSERT_STRICT_VALID( m_pUnit );

    if ( m_pUnit->GetUnitType( ) == CUnit::building )
    {
        // can't do this if still under construction
        if ( ( (CBuilding*)m_pUnit )->IsConstructing( ) )
            return;

        switch ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UTshipyard:
            ( (CVehicleBuilding*)m_pUnit )->GetDlgBuild( );
            return;

        case CStructureData::UTresearch:
            TRAP( );
            theApp.m_wndBar._GotoScience( );
            return;

#ifdef _DEBUG
        default:
            ASSERT_STRICT( FALSE );
            return;
#endif
        }
    }

    ASSERT_STRICT( m_pUnit->GetUnitType( ) == CUnit::vehicle );

    // have to be a construction vehicle
    if ( !( ( (CVehicle*)m_pUnit )->GetData( )->IsCrane( ) ) )
    {
        ASSERT_STRICT( FALSE );
        return;
    }
    ( (CVehicle*)m_pUnit )->GetDlgBuild( );
}

void CWndArea::RouteUnit( )
{

    ASSERT_STRICT_VALID( this );

    if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
        return;
    if ( m_pUnit->GetFlags( ) & CUnit::dying )
        return;
    CVehicle* pVeh = (CVehicle*)m_pUnit;
    ASSERT_STRICT_VALID( pVeh );

    if ( pVeh->m_pWndRoute == NULL )
        pVeh->m_pWndRoute = new CWndRoute( pVeh );

    if ( pVeh->m_pWndRoute->m_hWnd == NULL )
        pVeh->m_pWndRoute->Create( this );

    pVeh->m_pWndRoute->ShowWindow( SW_SHOWNORMAL );
    pVeh->m_pWndRoute->SetFocus( );
    SetButtonState( );
}

void CWndArea::RoadUnit( )
{

    ASSERT_VALID( this );

    if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
        return;

    m_iMode = road_begin;

    ::SetCursor( m_hCurRoadBgn[m_aa.m_iZoom] );
    SetButtonState( );
}

void CWndArea::CancelRoadUnit( )
{

    ReleaseMouse( );
    SelectOff( );
    SetButtonState( );
}

void CWndArea::RepairUnit( )
{

    ASSERT_VALID( this );

    if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::vehicle ) )
        return;

    m_iMode = repair_bldg;

    ::SetCursor( m_hCurNoRepair );
    SetButtonState( );
}

void CWndArea::CancelRepairUnit( )
{

    ReleaseMouse( );
    SelectOff( );
    SetButtonState( );
}

void CWndArea::UnloadUnit( )
{

    ASSERT_VALID( this );

    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_VALID( pUnit );
        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
            if ( ( (CVehicle*)pUnit )->GetCargoCount( ) > 0 )
            {
                CMsgUnloadCarrier _msg( (CVehicle*)pUnit );
                theGame.PostToClient( theGame.GetMe( ), &_msg, sizeof( _msg ) );
            }
    }

    SetButtonState( );
}

void CWndArea::RetreatUnit( )
{

    ASSERT_VALID( this );

    // find where to send them
    // 1: repair facility
    // 2: away
    CBuilding* pDest = NULL;
    POSITION   pos   = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );
        ASSERT_STRICT_VALID( pBldg );
        if ( pBldg->GetOwner( )->IsMe( ) )
            if ( pBldg->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
            {
                pDest = pBldg;
                break;
            }
    }

    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_VALID( pUnit );
        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
        {
            pUnit->ResumeUnit( );
            if ( ( pDest != NULL ) && ( ( (CVehicle*)pUnit )->GetDamagePer( ) != 100 ) )
            {
                ( (CVehicle*)pUnit )->SetDest( pDest->GetHex( ) );
                ( (CVehicle*)pUnit )->SetEvent( CVehicle::repair_self );
            }
            else
            {
                TRAP( );
                CHexCoord _hex( ( (CVehicle*)pUnit )->GetHexHead( ) );
                _hex.X( ) += RandNum( 20 ) - 10;
                _hex.Y( ) += RandNum( 20 ) - 10;
                _hex.Wrap( );
                ( (CVehicle*)pUnit )->SetDest( _hex );
            }
        }
    }

    SetButtonState( );
}

CUnit* CWndArea::GetUnitOn( CSubHex& _sub )
{

    _sub.Wrap( );
    CUnit* pUnit = theBuildingHex._GetBuilding( _sub );
    if ( pUnit != NULL )
        return ( pUnit );

    return ( theVehicleHex._GetVehicle( _sub ) );
}

// set button states
void CWndArea::SetButtonState( )
{

    // set the title
    if ( m_pUnit == NULL )
    {
        CString sTitle;
        if ( m_lstUnits.GetCount( ) <= 0 )
            sTitle.LoadString( IDS_TITLE_AREA_MAP );
        else
        {
            sTitle.LoadString( IDS_TITLE_AREA_MAP_MULTI );
            csPrintf( &sTitle, (const char*)IntToCString( m_lstUnits.GetCount( ) ) );
        }
        SetWindowText( sTitle );
    }
    else
    {
        // special for cranes
        if ( ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) &&
             ( ( (CVehicle*)m_pUnit )->GetData( )->GetType( ) == CTransportData::construction ) && ( m_iBuild > 0 ) )
        {
            CString sText =
                m_pUnit->GetData( )->GetDesc( ) + " - [" + theStructures.GetData( m_iBuild )->GetDesc( ) + "]";
            SetWindowText( sText );
        }
        else
            SetWindowText( m_pUnit->GetData( )->GetDesc( ) );
    }

    m_uFlags = 0;

    // find out what we have
    POSITION pos;
    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        ASSERT_STRICT_VALID( pUnit );

        if ( pUnit->GetUnitType( ) == CUnit::vehicle )
        {
            m_uFlags |= ( veh | can_stop );
            if ( ( (CVehicle*)pUnit )->GetData( )->IsBoat( ) )
                m_uFlags |= boat;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsCrane( ) )
                m_uFlags |= crane;
            else
                m_uFlags |= non_crane;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsTransport( ) )
                m_uFlags |= truck;
            else
                m_uFlags |= non_truck;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsCarrier( ) )
            {
                m_uFlags |= carrier;
                if ( ( (CVehicle*)pUnit )->GetCargoSize( ) > 0 )
                    m_uFlags |= loaded;
                if ( pUnit->GetDamagePer( ) != 100 )
                    m_uFlags |= veh_hurt;
            }
            else
                m_uFlags |= non_carrier;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsCarryable( ) )
                m_uFlags |= carryable;
            if ( ( (CVehicle*)pUnit )->GetData( )->IsLcCarryable( ) )
                m_uFlags |= lc_carryable;
            if ( ( ( (CVehicle*)pUnit )->GetData( )->IsRepairable( ) ) &&
                 ( pUnit->GetDamagePoints( ) < pUnit->GetData( )->GetDamagePoints( ) ) )
            {
                if ( ( (CVehicle*)pUnit )->GetData( )->IsBoat( ) )
                    m_uFlags |= sea_repairable;
                else
                    m_uFlags |= land_repairable;
            }
        }

        else
        {
            ASSERT_STRICT( pUnit->GetUnitType( ) == CUnit::building );
            m_uFlags |= ( bldg | non_crane | non_truck | non_carrier );
            if ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTvehicle )
                m_uFlags |= ( fac | can_stop );
            if ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTshipyard )
                m_uFlags |= ( fac | repair | can_stop );
            if ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
                m_uFlags |= ( repair | can_stop );
            if ( ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTmaterials ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTrepair ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTpower ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTresearch ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTmine ) ||
                 ( ( (CBuilding*)pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTfarm ) )
                m_uFlags |= can_stop;
        }

        if ( pUnit->GetData( )->_GetFireRate( ) > 0 )
            m_uFlags |= attk;
        if ( pUnit->GetFlags( ) & CUnit::destroying )
            m_uFlags |= destroying;
    }

    int const* pID = asNoneID;
    if ( m_uFlags != 0 )
    {
        // crane(s) only
        if ( ( m_uFlags & ( bldg | non_crane ) ) == 0 )
            pID = asCraneID;
        else
            // truck(s) only
            if ( ( m_uFlags & ( bldg | non_truck ) ) == 0 )
            pID = asTruckID;
        else
            // unloadable vehicles - or all carriers
            if ( ( m_uFlags & loaded ) || ( ( m_uFlags & ( bldg | non_carrier ) ) == 0 ) )
            pID = asUnloadID;
        else
            // vehicles only
            if ( !( m_uFlags & bldg ) )
            pID = asVehID;
        else
            // factory
            if ( ( m_uFlags & fac ) && ( m_lstUnits.GetCount( ) == 1 ) )
            pID = asFacID;
        else
            // bldg only
            if ( !( m_uFlags & veh ) )
            pID = asBldgID;
        else
            // something
            if ( m_lstUnits.GetCount( ) > 0 )
            pID = asUnitID;
    }

    // handle static text width
    int iStatusStart;
    if ( pID == asCraneID )
        iStatusStart = m_WndStatic.m_iStatusCraneStrt;
    else
        iStatusStart = m_WndStatic.m_iStatusNoCraneStrt;
    if ( m_WndStatic.m_iStatusStrt != iStatusStart )
    {
        m_WndStatic.m_iStatusStrt = iStatusStart;
        m_WndStatic.SizeStatus( );
    }

    // set the buttons in pID enabled, other disabled
    int const* pIDon = pID;
    for ( int iOn = ORD_OFFSET; iOn < ORD_OFFSET + NUM_ORD_BUTTONS; iOn++, pIDon++ )
        if ( ( abID[iOn] != IDC_UNIT_RESUME ) && ( abID[iOn] != IDC_UNIT_CANCEL_BUILD ) &&
             ( abID[iOn] != IDC_UNIT_CANCEL_REPAIR ) && ( abID[iOn] != IDC_UNIT_CANCEL_ROAD ) )
        {
            // if reg turned off then so is cancel
            if ( *pIDon != enableID )
            {
                if ( abID[iOn] == IDC_UNIT_STOP )
                {
                    EnableButton( IDC_UNIT_RESUME, FALSE );
                    ShowButton( IDC_UNIT_RESUME, FALSE );
                }
                if ( abID[iOn] == IDC_UNIT_BUILD )
                {
                    EnableButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                }
                if ( abID[iOn] == IDC_UNIT_REPAIR )
                {
                    EnableButton( IDC_UNIT_CANCEL_REPAIR, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_REPAIR, FALSE );
                }
                if ( abID[iOn] == IDC_UNIT_ROAD )
                {
                    EnableButton( IDC_UNIT_CANCEL_ROAD, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_ROAD, FALSE );
                }
            }

            if ( *pIDon == disableID )
            {
                EnableButton( abID[iOn], FALSE );
                ShowButton( abID[iOn], TRUE );
            }
            else if ( *pIDon != enableID )
            {
                EnableButton( abID[iOn], FALSE );
                ShowButton( abID[iOn], FALSE );
            }
            else
                // disable unload
                if ( abID[iOn] == IDC_UNIT_UNLOAD )
            {
                EnableButton( IDC_UNIT_UNLOAD, ( m_uFlags & loaded ) );
                ShowButton( IDC_UNIT_UNLOAD, TRUE );
            }
            else
                // retreat (repair)
                if ( abID[iOn] == IDC_UNIT_RETREAT )
            {
                EnableButton( IDC_UNIT_RETREAT, ( m_uFlags & veh_hurt ) );
                ShowButton( IDC_UNIT_RETREAT, TRUE );
            }
            else
                // cancel road
                if ( abID[iOn] == IDC_UNIT_ROAD )
            {
                BOOL bBuilding = ( ( m_iMode == road_begin ) || ( m_iMode == road_set ) );
                EnableButton( IDC_UNIT_CANCEL_ROAD, bBuilding );
                ShowButton( IDC_UNIT_CANCEL_ROAD, bBuilding );
                EnableButton( IDC_UNIT_ROAD, !bBuilding );
                ShowButton( IDC_UNIT_ROAD, !bBuilding );
            }
            else
                // cancel repair
                if ( abID[iOn] == IDC_UNIT_REPAIR )
            {
                EnableButton( IDC_UNIT_REPAIR, m_iMode != repair_bldg );
                ShowButton( IDC_UNIT_REPAIR, m_iMode != repair_bldg );
                EnableButton( IDC_UNIT_CANCEL_REPAIR, m_iMode == repair_bldg );
                ShowButton( IDC_UNIT_CANCEL_REPAIR, m_iMode == repair_bldg );
            }
            else
                // only 1 crane
                if ( ( abID[iOn] == IDC_UNIT_BUILD ) && ( m_uFlags & crane ) )
            {
                if ( m_lstUnits.GetCount( ) != 1 )
                {
                    EnableButton( IDC_UNIT_BUILD, FALSE );
                    ShowButton( IDC_UNIT_BUILD, TRUE );
                    EnableButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                    ShowButton( IDC_UNIT_CANCEL_BUILD, FALSE );
                }
                else
                {
                    EnableButton( IDC_UNIT_BUILD, m_iMode != build_ready );
                    ShowButton( IDC_UNIT_BUILD, m_iMode != build_ready );
                    EnableButton( IDC_UNIT_CANCEL_BUILD, m_iMode == build_ready );
                    ShowButton( IDC_UNIT_CANCEL_BUILD, m_iMode == build_ready );
                }
            }
            else
                // build -> cancel
                if ( abID[iOn] == IDC_UNIT_BUILD )
            {
                BOOL bBuilding;
                if ( ( m_iMode == build_ready ) ||
                     ( ( m_pUnit ) && ( m_pUnit->GetUnitType( ) == CUnit::building ) &&
                       ( !( (CBuilding*)m_pUnit )->IsConstructing( ) ) &&
                       ( ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTvehicle ) ||
                         ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) == CStructureData::UTshipyard ) ) &&
                       ( ( (CVehicleBuilding*)m_pUnit )->GetBldUnt( ) != NULL ) ) )
                    bBuilding = TRUE;
                else
                    bBuilding = FALSE;
                EnableButton( IDC_UNIT_BUILD, !bBuilding );
                ShowButton( IDC_UNIT_BUILD, !bBuilding );
                EnableButton( IDC_UNIT_CANCEL_BUILD, bBuilding );
                ShowButton( IDC_UNIT_CANCEL_BUILD, bBuilding );
            }
            else
                // stop/resume
                if ( abID[iOn] == IDC_UNIT_STOP )
            {
                // if nothing can be stopped - hide it
                if ( !( m_uFlags & can_stop ) )
                {
                    EnableButton( IDC_UNIT_STOP, FALSE );
                    ShowButton( IDC_UNIT_STOP, FALSE );
                    EnableButton( IDC_UNIT_RESUME, FALSE );
                    ShowButton( IDC_UNIT_RESUME, FALSE );
                }
                else
                {
                    POSITION pos;
                    BOOL     bStop = TRUE;
                    for ( pos = m_lstUnits.GetHeadPosition( ); pos != NULL; )
                    {
                        CUnit* pUnit = m_lstUnits.GetNext( pos );
                        ASSERT_STRICT_VALID( pUnit );
                        if ( pUnit->IsPaused( ) )
                        {
                            bStop = FALSE;
                            break;
                        }
                    }
                    EnableButton( IDC_UNIT_STOP, bStop );
                    ShowButton( IDC_UNIT_STOP, bStop );
                    EnableButton( IDC_UNIT_RESUME, !bStop );
                    ShowButton( IDC_UNIT_RESUME, !bStop );
                }
            }
            // any other button
            else
            {
                EnableButton( abID[iOn], TRUE );
                ShowButton( abID[iOn], TRUE );
            }
        }

    StatUnit( m_pUnit );
}

void CWndArea::SetMouseState( )
{

    static int iNum = 0;
    if ( iNum != 0 )
    {
        ::SetCursor( m_hCurReg );
        return;
    }

    m_uMouseMode = lmb_nothing;

    // if RMB down its move
    if ( m_bRBtnDown )
    {
        ::SetCursor( m_hCurMove[m_iMoveCur] );
        return;
    }

    // for these cases its not location dependent
    switch ( m_iMode )
    {
    case rocket_ready:
    case rocket_pos:
    case build_ready:
    case build_loc:
        ::SetCursor( NULL );
        return;

    case road_begin:
        ::SetCursor( m_hCurRoadBgn[m_aa.m_iZoom] );
        return;
    case road_set:
        ::SetCursor( m_hCurRoadSet[m_aa.m_iZoom] );
        return;
    case veh_route:
        ::SetCursor( m_hCurRoute );
        return;

    case repair_bldg: {
        CPoint pt;
        ::GetCursorPos( &pt );
        ScreenToClient( &pt );
        CHexCoord hex = m_aa.WindowToHex( pt );
        hex.Wrap( );
        CBuilding* pBldg = theBuildingHex._GetBuilding( hex );
        if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) &&
             ( ( pBldg->GetDamagePer( ) < 100 ) || ( pBldg->IsConstructing( ) ) ) )
            ::SetCursor( m_hCurRepair );
        else
            ::SetCursor( m_hCurNoRepair );
        return;
    }

    case normal:
    case normal_select:
        break;

    default:
        ::SetCursor( m_hCurReg );
        return;
    }

    // get the mouse position
    CPoint pt;
    ::GetCursorPos( &pt );
    ScreenToClient( &pt );

    // if we are selecting - that has absolute pri
    if ( m_iMode == normal_select )
        if ( ( abs( pt.x - m_ptLMB.x ) >= theMap.HexWid( m_aa.m_iZoom ) / 2 ) ||
             ( abs( pt.y - m_ptLMB.y ) >= theMap.HexHt( m_aa.m_iZoom ) / 2 ) )
        {
            ::SetCursor( m_hCurReg );
            m_uMouseMode = lmb_select;
            return;
        }

    // see if flags force it
    if ( m_lstUnits.GetCount( ) > 0 )
    {
        int iShift = GetKeyState( VK_SHIFT ) & ~1;
        int iCtrl  = GetKeyState( VK_CONTROL ) & ~1;
        if ( iShift & iCtrl )
        {
            if ( m_uFlags & attk )
            {
                ::SetCursor( m_hCurTarget[m_aa.m_iZoom] );
                m_uMouseMode = lmb_attack;
            }
            else
            {
                ::SetCursor( m_hCurReg );
                m_uMouseMode = lmb_nothing;
            }
            return;
        }
        if ( ( iCtrl ) && ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) ) )
        {
            ::SetCursor( m_hCurGoto[m_aa.m_iZoom] );
            m_uMouseMode = lmb_goto;
            return;
        }
    }

    CHitInfo  hitinfo = m_aa.GetHit( pt );
    CHexCoord hexcoord( hitinfo._GetHexCoord( ) );
    hexcoord.Wrap( );
    CUnit*       pUnitOn = hitinfo.GetUnit( );
    CBridgeUnit* pBu     = hitinfo.GetBridge( );

    ASSERT_STRICT_VALID_OR_NULL( pUnitOn );

    // if not visible then it's not there
    if ( pUnitOn != NULL )
        if ( !pUnitOn->IsVisible( ) )
            pUnitOn = NULL;
    if ( pUnitOn != NULL )
        if ( pUnitOn->GetUnitType( ) == CUnit::vehicle )
            if ( !theMap.GetHex( hexcoord )->GetVisibility( ) )
                pUnitOn = NULL;

    // if nothing is selected its select or nothing
    if ( m_lstUnits.GetCount( ) == 0 )
    {
        if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->IsMe( ) ) )
        {
            ::SetCursor( m_hCurSelect[m_aa.m_iZoom] );
            m_uMouseMode = lmb_select;
        }
        else
        {
            ::SetCursor( m_hCurReg );
            m_uMouseMode = lmb_nothing;
        }
        return;
    }

    // step -2 - finish a bridge
    if ( pBu != NULL )
        if ( ( !( m_uFlags & non_crane ) ) && ( m_uFlags & crane ) && ( !pBu->GetParent( )->IsBuilt( ) ) &&
             ( pBu->IsExit( ) ) )
        {
            ::SetCursor( m_hCurRepair );
            m_uMouseMode = lmb_repair_bldg;
            return;
        }

    // if its mine we can select it
    if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->IsMe( ) ) )
    {
        // step -1 - if we are a crane and this building needs help => repair it
        if ( ( !( m_uFlags & non_crane ) ) && ( m_uFlags & crane ) )
            if ( ( pUnitOn->GetUnitType( ) == CUnit::building ) &&
                 ( ( pUnitOn->GetDamagePer( ) < 100 ) || ( ( (CBuilding*)pUnitOn )->IsConstructing( ) ) ) )
            {
                ::SetCursor( m_hCurRepair );
                m_uMouseMode = lmb_repair_bldg;
                return;
            }

        // step -1.5 - if we are a truck and over one of our buildings - AND NOT A repair center - it's a goto
        if ( ( !( m_uFlags & non_truck ) ) && ( m_uFlags & truck ) && ( pUnitOn->GetUnitType( ) == CUnit::building ) &&
             ( ( (CBuilding*)pUnitOn )->GetData( )->GetType( ) != CStructureData::repair ) )
        {
            ::SetCursor( m_hCurGoto[m_aa.m_iZoom] );
            m_uMouseMode = lmb_goto;
            return;
        }

        // step 0 - find out if we are on a carrier or repair facility
        BOOL bCarrier = FALSE, bLcCarrier = FALSE, bLandRepair = FALSE, bSeaRepair = FALSE;
        if ( ( pUnitOn->GetUnitType( ) == CUnit::building ) && ( !( (CBuilding*)pUnitOn )->IsConstructing( ) ) )
        {
            if ( ( (CBuilding*)pUnitOn )->GetData( )->GetUnionType( ) == CStructureData::UTrepair )
                bLandRepair = TRUE;
            else if ( ( (CBuilding*)pUnitOn )->GetData( )->GetUnionType( ) == CStructureData::UTshipyard )
                bSeaRepair = TRUE;
        }
        else if ( ( pUnitOn->GetUnitType( ) == CUnit::vehicle ) &&
                  ( ( (CVehicle*)pUnitOn )->GetData( )->IsCarrier( ) ) &&
                  ( !( (CVehicle*)pUnitOn )->GetData( )->IsTransport( ) ) )
        {
            bCarrier = TRUE;
            if ( ( (CVehicle*)pUnitOn )->GetData( )->IsBoat( ) )
                bLcCarrier = TRUE;
        }

        // if repairable - that's it
        if ( ( ( ( m_uFlags & land_repairable ) != 0 ) && ( bLandRepair ) ) ||
             ( ( ( m_uFlags & sea_repairable ) != 0 ) && ( bSeaRepair ) ) )
        {
            ::SetCursor( m_hCurRepair );
            m_uMouseMode = lmb_repair_self;
            return;
        }

        // if it can be loaded on
        if ( ( bCarrier ) &&
             ( ( (CVehicle*)pUnitOn )->GetCargoSize( ) < ( (CVehicle*)pUnitOn )->GetData( )->GetPeopleCarry( ) ) )
            if ( ( m_uFlags & carryable ) || ( ( bLcCarrier ) && ( m_uFlags & lc_carryable ) ) )
            {
                ::SetCursor( m_hCurLoad[m_aa.m_iZoom] );
                m_uMouseMode = lmb_load;
                return;
            }

        // if it can be unloaded AND is selected (must be on land)
        if ( ( bCarrier ) && ( ( (CVehicle*)pUnitOn )->GetCargoCount( ) > 0 ) &&
             ( ( (CVehicle*)pUnitOn )->IsSelected( ) ) )
            if ( ( !theMap._GetHex( ( (CVehicle*)pUnitOn )->GetPtHead( ) )->IsWater( ) ) ||
                 ( !theMap._GetHex( ( (CVehicle*)pUnitOn )->GetPtTail( ) )->IsWater( ) ) )
            {
                ::SetCursor( m_hCurUnload[m_aa.m_iZoom] );
                m_uMouseMode = lmb_unload;
                return;
            }

        ::SetCursor( m_hCurSelect[m_aa.m_iZoom] );
        m_uMouseMode = lmb_select;
        return;
    }

    // if alliance & truck - it's a goto
    if ( ( pUnitOn != NULL ) && ( pUnitOn->GetOwner( )->GetTheirRelations( ) == RELATIONS_ALLIANCE ) )
        if ( ( !( m_uFlags & non_truck ) ) && ( m_uFlags & truck ) && ( pUnitOn->GetUnitType( ) == CUnit::building ) )
        {
            ::SetCursor( m_hCurGoto[m_aa.m_iZoom] );
            m_uMouseMode = lmb_goto;
            return;
        }

    // if nothing under or friendly its goto - unless we are buildings only
    if ( ( pUnitOn == NULL ) || ( pUnitOn->GetOwner( )->GetRelations( ) <= RELATIONS_PEACE ) )
    {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) == CUnit::vehicle ) )
            if ( ( pUnitOn == NULL ) || ( pUnitOn->GetUnitType( ) != CUnit::building ) ||
                 ( ( m_pUnit != NULL ) && ( ( (CVehicle*)m_pUnit )->CanEnterBldg( (CBuilding*)pUnitOn ) ) ) )
            {
                m_uMouseMode = lmb_goto;
                ::SetCursor( m_hCurGoto[m_aa.m_iZoom] );
                return;
            }

        m_uMouseMode = lmb_nothing;
        ::SetCursor( m_hCurReg );
        return;
    }

    // ok - its a target (and in range for a building)
    if ( m_uFlags & attk )
    {
        if ( ( m_pUnit == NULL ) || ( m_pUnit->GetUnitType( ) != CUnit::building ) )
        {
            ::SetCursor( m_hCurTarget[m_aa.m_iZoom] );
            m_uMouseMode = lmb_attack;
            return;
        }

        // building must be in range
        int iLOS = theMap.LineOfSight( m_pUnit, pUnitOn );
        if ( abs( iLOS ) <= m_pUnit->GetRange( ) )
        {
            ::SetCursor( m_hCurTarget[m_aa.m_iZoom] );
            m_uMouseMode = lmb_attack;
            return;
        }
    }

    m_uMouseMode = lmb_nothing;
    ::SetCursor( m_hCurReg );
}

BOOL CWndArea::OnMouseWheel( UINT nFlags, short zDelta, CPoint pt )
{
    // accumulate wheel deltas; require two notches (2 * WHEEL_DELTA) to trigger a zoom
    s_areaWheelAccum += (int)zDelta;

    const int needed = 2 * WHEEL_DELTA;

    if ( s_areaWheelAccum >= needed )
    {
        // scroll in by 2 clicks => zoom in
        s_areaWheelAccum = 0;
        ZoomIn( );
        return TRUE;  // handled
    }
    else if ( s_areaWheelAccum <= -needed )
    {
        // scroll out by 2 clicks => zoom out
        s_areaWheelAccum = 0;
        ZoomOut( );
        return TRUE;  // handled
    }

    // not enough accumulation yet; let base class handle any default behavior
    return CWndAnim::OnMouseWheel( nFlags, zDelta, pt );
}

void CWndArea::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{

    // handle changes in CTRL & SHIFT which changes the cursor
    if ( ( nChar == VK_CONTROL ) || ( nChar == VK_SHIFT ) )
        SetMouseState( );

    CWndAnim::OnKeyUp( nChar, nRepCnt, nFlags );
}

void CWndArea::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{

    // handle changes in CTRL & SHIFT which changes the cursor
    if ( ( nChar == VK_CONTROL ) || ( nChar == VK_SHIFT ) )
        SetMouseState( );

    // if not a number we don't care about it
    if ( ( nChar < '0' ) || ( '9' < nChar ) )
    {
        CWndAnim::OnKeyDown( nChar, nRepCnt, nFlags );
        return;
    }

    CArray<DWORD, DWORD>* paSave = &( m_aSaveSel[nChar - '0'] );

    // see if it's a save
    if ( GetKeyState( VK_CONTROL ) & ~1 )
    {
        if ( !( GetKeyState( VK_SHIFT ) & ~1 ) )
            paSave->RemoveAll( );
        else
            TRAP( );
        int iOn = paSave->GetSize( );
        paSave->SetSize( m_lstUnits.GetCount( ) + iOn );
        POSITION pos = m_lstUnits.GetHeadPosition( );
        for ( ; pos != NULL; iOn++ )
        {
            CUnit* pUnit = m_lstUnits.GetNext( pos );
            paSave->SetAtGrow( iOn, pUnit->GetID( ) );
        }
        return;
    }

    // it's a restore
    // do we kill the old?
    if ( !( GetKeyState( VK_SHIFT ) & ~1 ) )
        m_lstUnits.RemoveAllUnits( TRUE );

    theApp.m_wndBldgs.m_ListBox.SetRedraw( FALSE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( FALSE );

    for ( int iOn = 0; iOn < paSave->GetSize( ); iOn++ )
    {
        CUnit* pUnit = ::GetUnit( paSave->GetAt( iOn ) );
        if ( pUnit != NULL )
            m_lstUnits.AddUnit( pUnit, TRUE );
    }

    theApp.m_wndBldgs.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndVehicles.m_ListBox.SetRedraw( TRUE );
    theApp.m_wndBldgs.m_ListBox.InvalidateRect( NULL, FALSE );
    theApp.m_wndVehicles.m_ListBox.InvalidateRect( NULL, FALSE );

    // update the screen
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;

    // set button states
    SetButtonState( );

    // repaint it
    InvalidateStatus( );
    InvalidateSound( );
}

void CWndArea::ReCenter( )
{

    TRAP( );
}

CWnd* CWndArea::GetExpand( )
{

    TRAP( );
    return ( NULL );
}

// called when a unit is dtor'ed
void CWndArea::UnitDying( CUnit* pUnit )
{

    BOOL bRedraw = FALSE;

    // remove from selected units list
    POSITION pos = m_lstUnits.Find( pUnit );
    if ( pos != NULL )
    {
        m_lstUnits.RemoveAt( pos );
        bRedraw = TRUE;
    }

    // remove as selected unit
    if ( m_pUnit == pUnit )
    {
        if ( m_iMode == build_loc )
            SelectOff( );
        m_pUnit = NULL;
        m_iMode = normal;
        bRedraw = TRUE;
    }

    // remove it's info window
    if ( ( m_pWndInfo != NULL ) && ( m_pWndInfo->m_hWnd != NULL ) && ( m_pWndInfo->GetUnit( ) == pUnit ) )
    {
        m_pWndInfo->DestroyWindow( );
        bRedraw = TRUE;
    }

    // remove status windows
    if ( GetStaticUnit( ) == pUnit )
    {
        StatUnit( NULL );
        bRedraw = TRUE;
    }

    // redraw if changed
    if ( bRedraw )
    {
        InvalidateStatus( );
        SetButtonState( );
    }
}

void CWndArea::UpdateSound( )
{

    m_bNewPos = FALSE;
    if ( !theMusicPlayer.SoundsPlaying( ) )
        return;

    theMusicPlayer.ClrBackgroundSounds( );

    CRect rect;
    GetClientRect( &rect );

    int iOldDir = xiDir;
    xiDir       = m_aa.m_iDir;

    // terrain sounds
    if ( m_aa.m_iZoom <= 1 )
    {
        CSize     size  = m_aa.m_dibwnd.GetWinSize( );
        CHexCoord hexTL = m_aa._WindowToHex( CPoint( 0, 0 ) );
        CHexCoord hexBL = m_aa._WindowToHex( CPoint( 0, size.cy - 1 ) );

        CViewHexCoord viewhexTL( hexTL );
        CViewHexCoord viewhexBL( hexBL );

        int iW = ( size.cx + MAX_HEX_HT - 1 ) >> ( HEX_HT_PWR + 1 - m_aa.m_iZoom );
        int iH = viewhexBL.y - viewhexTL.y + 1;

        int aMul[4];
        theMap.DirMult( m_aa.m_iDir, aMul );

        int       iTotal = 0, iTrees = 0, iSwamp = 0, iRiver = 0, iOcean = 0;
        CHexCoord hexLeft( hexTL );
        hexLeft.Wrap( );

        for ( int x = 0; x < iW; x++ )
        {
            if ( x & 1 )
                hexLeft.Y( hexLeft.Y( ) + aMul[3] );
            else
                hexLeft.X( hexLeft.X( ) + aMul[2] );
            CHexCoord hexOn( hexLeft );
            hexOn.Wrap( );

            for ( int y = 0; y < iH; y++ )
            {
                iTotal++;
                switch ( theMap._GetHex( hexOn )->GetType( ) )
                {
                case CHex::forest:
                    iTrees++;
                    break;
                case CHex::swamp:
                    iSwamp++;
                    break;
                case CHex::river:
                    iRiver++;
                    break;
                case CHex::ocean:
                    iOcean++;
                    break;
                }

                hexOn.X( ) += aMul[0];
                hexOn.Y( ) += aMul[1];
                hexOn.Wrap( );
            }
        }

        // ok, play them if loud enough
        // BUGBUG - do we need to pan?
        if ( iTotal > 0 ) { // only run this calculation if iTotal isn't 0
            int iTreeVol = (iTrees * 100) / iTotal;
            if (iTreeVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::trees, SFXPRIORITY::terrain_pri, 64, iTreeVol);
            int iSwampVol = (iSwamp * 100) / iTotal;
            if (iSwampVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::swamp, SFXPRIORITY::terrain_pri, 64, iSwampVol);
            int iRiverVol = (iRiver * 100) / iTotal;
            if (iRiverVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::river, SFXPRIORITY::terrain_pri, 64, iRiverVol);
            int iOceanVol = (iOcean * 100) / iTotal;
            if (iOceanVol > 20)
                theMusicPlayer.QueueBackgroundSound(SOUNDS::ocean, SFXPRIORITY::terrain_pri, 64, iOceanVol);
        }
    }

    // walk the buildings for construction, burning, selected
    POSITION pos;
    pos = theBuildingMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD      dwID;
        CBuilding* pBldg;
        theBuildingMap.GetNextAssoc( pos, dwID, pBldg );

        // must be in the window and visible
        //   OR ours & damaged
        if ( ( pBldg->GetOwner( )->IsMe( ) ) || ( theMap._GetHex( pBldg->GetHex( ) )->GetVisible( ) ) )
        {
            // use iVol to see if it's in the window for construction
            int iPan, iVol;
            GetPanAndVol( pBldg, iPan, iVol );

            // on fire - visible or us
            if ( ( iVol >= 100 ) || ( pBldg->GetOwner( )->IsMe( ) ) )
            {
                if ( !pBldg->GetOwner( )->IsMe( ) )
                    iVol = 60;

                if ( pBldg->GetDamage( ) == 3 )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::damage3, SFXPRIORITY::damage_pri, iPan, iVol );
                else if ( pBldg->GetDamage( ) == 4 )
                {
                    TRAP( );
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::damage4, SFXPRIORITY::damage_pri, iPan, iVol );
                }
            }

            // construction - must be visible, quieter if not ours
            if ( ( iVol >= 100 ) && ( pBldg->IsConstructing( ) ) )
            {
                if ( !pBldg->GetOwner( )->IsMe( ) )
                    iVol = 60;

                if ( !pBldg->IsFoundationComplete( ) )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::const1, SFXPRIORITY::const_pri, iPan, iVol );
                else if ( !pBldg->IsSkltnComplete( ) )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::const2, SFXPRIORITY::const_pri, iPan, iVol );
                else
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::const3, SFXPRIORITY::const_pri, iPan, iVol );
            }
        }
    }

    // walk the vehicles for burning, selected
    pos = theVehicleMap.GetStartPosition( );
    while ( pos != NULL )
    {
        DWORD     dwID;
        CVehicle* pVeh;
        theVehicleMap.GetNextAssoc( pos, dwID, pVeh );

        // must be in the window and visible
        if ( ( pVeh->GetOwner( )->IsMe( ) ) || ( theMap._GetHex( pVeh->GetHexHead( ) )->GetVisible( ) ) )
        {
            int iPan, iVol;
            GetPanAndVol( pVeh, iPan, iVol );

            if ( iVol >= 100 )
            {
                if ( !pVeh->GetOwner( )->IsMe( ) )
                    iVol = 60;

                // on fire
                if ( pVeh->GetDamage( ) == 3 )
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::damage3, SFXPRIORITY::damage_pri, iPan, iVol );
                else if ( pVeh->GetDamage( ) == 4 )
                {
                    TRAP( );
                    theMusicPlayer.QueueBackgroundSound( SOUNDS::damage4, SFXPRIORITY::damage_pri, iPan, iVol );
                }

                // if it's selected
                if ( pVeh->IsSelected( ) )
                {
                    if ( ( pVeh->GetRouteMode( ) == CVehicle::stop ) ||
                         ( pVeh->GetRouteMode( ) == CVehicle::blocked ) ||
                         ( pVeh->GetRouteMode( ) == CVehicle::traffic ) )
                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundIdle( ),
                                                             SFXPRIORITY::selected_pri, iPan, iVol );
                    else
                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundRun( ),
                                                             SFXPRIORITY::selected_pri, iPan, iVol );
                }
                else

                    // if it's moving - ours are louder
                    if ( pVeh->GetRouteMode( ) == CVehicle::moving )
                {
                    if ( pVeh->GetOwner( )->IsMe( ) )
                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundRun( ),
                                                             SFXPRIORITY::selected_pri, iPan, 60 );
                    else

                        theMusicPlayer.QueueBackgroundSound( pVeh->GetData( )->GetSoundRun( ),
                                                             SFXPRIORITY::selected_pri, iPan, 40 );
                }
            }
        }
    }

    xiDir = iOldDir;

    theMusicPlayer.UpdateBackgroundSounds( );
}


/////////////////////////////////////////////////////////////////////////////
// CWndInfo

CWndInfo::CWndInfo( )
{

    m_pUnit = NULL;
    m_pdib  = NULL;
}

CWndInfo::~CWndInfo( )
{

    delete m_pdib;
}


BEGIN_MESSAGE_MAP( CWndInfo, CWndBase )
//{{AFX_MSG_MAP(CWndInfo)
ON_WM_PAINT( )
ON_WM_SIZE( )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )


/////////////////////////////////////////////////////////////////////////////
// CWndInfo message handlers

const DWORD dwStyle = WS_CHILD;

CWndInfo* CWndInfo::Create( CPoint& pt, CUnit* pUnit, CWndArea* pPar )
{
#ifdef LOGGINGON
    OutputDebugStringA( "CWndInfo::Create" );
#endif

    ASSERT_STRICT_VALID( pUnit );

    m_pUnit = pUnit;

    // figure the size
    CRect rect;
    rect.left = rect.top = 0;
    rect.right           = __max( m_pUnit->GetData( )->GetDesc( ).GetLength( ) + 2, 20 ) * theApp.TextWid( );
    rect.bottom          = FigureHt( );

    // stop clipping to the right/below (above/left impossible)
    CRect rectWin( pt, CSize( rect.Width( ), rect.Height( ) ) );
    pPar->GetClientRect( rect );
    if ( rectWin.right > rect.right )
        rectWin.OffsetRect( -( rectWin.right - rect.right ), 0 );
    if ( rectWin.bottom > rect.bottom )
        rectWin.OffsetRect( 0, -( rectWin.bottom - rect.bottom ) );

    if ( m_pdib != NULL )
        m_pdib->Resize( rectWin.Width( ), rectWin.Height( ) );
    else
        m_pdib = new CDIB( ptrthebltformat->GetColorFormat( ), CBLTFormat::DIB_MEMORY,
                           ptrthebltformat->GetMemDirection( ), rectWin.Width( ), rectWin.Height( ) );

    return ( (CWndInfo*)CWndBase::Create( NULL, pUnit->GetData( )->GetDesc( ), dwStyle, rectWin, pPar, 100, NULL ) );
}

static void _DrawText( CDC* pDc, CRect& rect, CString const& sText, BOOL bRed = FALSE );

static void _DrawText( CDC* pDc, CRect& rect, CString const& sText, BOOL bRed )
{

    rect.OffsetRect( 1, 1 );
    pDc->SetTextColor( RGB( 128, 128, 128 ) );
    pDc->DrawText( sText, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER );

    rect.OffsetRect( -1, -1 );
    if ( bRed )
        pDc->SetTextColor( RGB( 255, 50, 27 ) );
    else
        pDc->SetTextColor( RGB( 0, 0, 0 ) );
    pDc->DrawText( sText, -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER );
}

void CWndInfo::OnPaint( )
{

    ASSERT_STRICT_VALID( this );
    CPaintDC dc( this );  // device context for painting
    thePal.Paint( dc.m_hDC );

    // set it up
    CRect rect;
    CWndBase::GetClientRect( &rect );

    // draw the background
    PaintBorder( m_pdib, rect, TRUE );

    // set up the DC for text
    CDC* pDc = CDC::FromHandle( m_pdib->GetDC( ) );
    if ( pDc == NULL )
    {
        thePal.EndPaint( dc.m_hDC );
        return;
    }
    thePal.Paint( pDc->m_hDC );
    pDc->SetBkMode( TRANSPARENT );
    CFont* pOldFont = pDc->SelectObject( &theApp.TextFont( ) );

    // draw the name
    CDIB* pdibHorz = theBitmaps.GetByIndex( DIB_BORDER_HORZ );
    CDIB* pdibVert = theBitmaps.GetByIndex( DIB_BORDER_VERT );
    rect.top       = pdibHorz->GetHeight( ) + theApp.FlatDimen( );
    rect.left      = pdibVert->GetWidth( ) + theApp.FlatDimen( );
    rect.right -= theApp.FlatDimen( );
    rect.bottom = rect.top + theApp.TextHt( );
    _DrawText( pDc, rect, m_pUnit->GetData( )->GetDesc( ) );

    // draw the state
    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
    {
        CVehicle* pVeh = (CVehicle*)m_pUnit;
        if ( pVeh->GetData( )->IsTransport( ) )
        {
            CString sText;
            if ( !pVeh->IsHpControl( ) )
                sText = CTransportData::m_sAuto;
            else if ( pVeh->GetEvent( ) == CVehicle::route )
                sText = CTransportData::m_sRoute;

            CBuilding* pBldg = theBuildingHex.GetBuilding( pVeh->GetPtHead( ) );
            if ( ( pBldg == NULL ) || ( pVeh->GetHexOwnership( ) ) )
                pBldg = theBuildingHex.GetBuilding( pVeh->GetHexDest( ) );
            if ( ( pBldg != NULL ) && ( pBldg->GetOwner( )->IsMe( ) ) )
                sText += "[" + pBldg->GetData( )->GetDesc( ) + "]";
            else if ( pVeh->GetData( )->IsTransport( ) )
            {
                if ( pVeh->GetRouteMode( ) == CVehicle::stop )
                    sText += CTransportData::m_sIdle;
                else
                    sText += CTransportData::m_sTravel;
            }

            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom = rect.top + theApp.TextHt( );
            _DrawText( pDc, rect, sText );
        }
    }

    // draw the damage
    rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
    rect.bottom = rect.top + theApp.TextHt( );
    CString sText;
    sText.LoadString( IDS_INFO_DAMAGE );
    csPrintf( &sText, (const char*)IntToCString( __min( 99, 100 - m_pUnit->GetDamagePer( ) ) ) );
    _DrawText( pDc, rect, sText, m_pUnit->GetDamagePer( ) < 50 );

    // if building draw it's status
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
        switch ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UThousing:
        case CStructureData::UTpower:
        case CStructureData::UTresearch:
        case CStructureData::UTrepair:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTshipyard:
            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom = rect.top + theApp.TextHt( );
            ( (CBuilding*)m_pUnit )->ShowStatusText( sText );
            _DrawText( pDc, rect, sText, m_pUnit->GetDamagePer( ) < 50 );
            break;
        }

    // draw the materials
    for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
    {
        int iNeed = 0;
        if ( ( iOn < CMaterialTypes::GetNumBuildTypes( ) ) && ( m_pUnit->GetUnitType( ) == CUnit::building ) )
            iNeed = ( (CBuilding*)m_pUnit )->GetBldgResReq( iOn, FALSE );
        if ( ( m_pUnit->GetStore( iOn ) != 0 ) || ( iNeed > 0 ) )
        {
            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom   = rect.top + theApp.TextHt( );
            CString sText = CMaterialTypes::GetDesc( iOn ) + ": " + IntToCString( m_pUnit->GetStore( iOn ), 10, TRUE );
            _DrawText( pDc, rect, sText );

            if ( iNeed > 0 )
            {
                CRect rectNum( rect );
                pDc->DrawText( sText, -1, &rectNum, DT_CALCRECT | DT_LEFT | DT_SINGLELINE | DT_VCENTER );
                rectNum.left  = rectNum.right + theApp.FlatDimen( );
                rectNum.right = rect.right;
                sText         = "(" + IntToCString( iNeed, 10, TRUE ) + ")";
                _DrawText( pDc, rectNum, sText, TRUE );
            }
        }
    }

    // if a carrier list units onboard
    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
    {
        POSITION pos = ( (CVehicle*)m_pUnit )->GetCargoHeadPosition( );
        while ( pos != NULL )
        {
            CVehicle* pVeh = ( (CVehicle*)m_pUnit )->GetCargoNext( pos );
            rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
            rect.bottom = rect.top + theApp.TextHt( );
            _DrawText( pDc, rect, pVeh->GetData( )->GetDesc( ) );
        }
    }

    // vehicles inside building
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
    {
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( !pVeh->GetHexOwnership( ) ) &&
                 ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == m_pUnit ) )
            {
                rect.top += theApp.TextHt( ) + theApp.FlatDimen( );
                rect.bottom = rect.top + theApp.TextHt( );
                _DrawText( pDc, rect, pVeh->GetData( )->GetDesc( ) );
            }
        }
    }

    // paint it
    m_pdib->BitBlt( dc.m_hDC, m_pdib->GetRect( ), CPoint( 0, 0 ) );

    pDc->SelectObject( pOldFont );
    thePal.EndPaint( pDc->m_hDC );
    if ( m_pdib->IsBitmapSelected( ) )
        m_pdib->ReleaseDC( );

    thePal.EndPaint( dc.m_hDC );
    // Do not call CWndBase::OnPaint() for painting messages
}

void CWndInfo::OnSize( UINT nType, int cx, int cy )
{

    CWndBase::OnSize( nType, cx, cy );

    m_pdib->Resize( cx, cy );
}

int CWndInfo::FigureHt( )
{

    CRect rect;
    rect.left = rect.top = 0;
    rect.right           = __max( m_pUnit->GetData( )->GetDesc( ).GetLength( ) + 2, 20 ) * theApp.TextWid( );
    rect.bottom          = 2;

    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
        if ( ( (CVehicle*)m_pUnit )->GetData( )->IsTransport( ) )
            rect.bottom++;

    // if building draw it's status
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
        switch ( ( (CBuilding*)m_pUnit )->GetData( )->GetUnionType( ) )
        {
        case CStructureData::UTvehicle:
        case CStructureData::UThousing:
        case CStructureData::UTpower:
        case CStructureData::UTresearch:
        case CStructureData::UTrepair:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTshipyard:
            rect.bottom++;
            break;
        }

    if ( ( m_pUnit->GetUnitType( ) == CUnit::building ) ||
         ( ( (CVehicle*)m_pUnit )->GetData( )->GetMaxMaterials( ) != 0 ) )
        for ( int iOn = 0; iOn < CMaterialTypes::GetNumTypes( ); iOn++ )
        {
            int iNeed = 0;
            if ( ( iOn < CMaterialTypes::GetNumBuildTypes( ) ) && ( m_pUnit->GetUnitType( ) == CUnit::building ) )
                iNeed = ( (CBuilding*)m_pUnit )->GetBldgResReq( iOn, FALSE );
            if ( ( m_pUnit->GetStore( iOn ) != 0 ) || ( iNeed > 0 ) )
                rect.bottom++;
        }

    // vehicles we're carrying
    if ( m_pUnit->GetUnitType( ) == CUnit::vehicle )
        rect.bottom += ( (CVehicle*)m_pUnit )->GetCargoCount( );

    // vehicles inside us
    if ( m_pUnit->GetUnitType( ) == CUnit::building )
    {
        POSITION pos = theVehicleMap.GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD     dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
            if ( ( pVeh->GetOwner( )->IsMe( ) ) && ( !pVeh->GetHexOwnership( ) ) &&
                 ( theBuildingHex._GetBuilding( pVeh->GetPtHead( ) ) == m_pUnit ) )
                rect.bottom++;
        }
    }

    rect.bottom = 2 * theApp.FlatDimen( ) + rect.bottom * ( theApp.TextHt( ) + theApp.FlatDimen( ) );

    // border
    rect.bottom += 2 * __min( 6, theBitmaps.GetByIndex( DIB_BORDER_HORZ )->GetHeight( ) );

    AdjustWindowRect( &rect, dwStyle, FALSE );

    return ( rect.Height( ) );
}

void CWndInfo::Refigure( )
{

    CRect rect;
    GetWindowRect( &rect );

    int iHt = FigureHt( );
    if ( iHt == rect.Height( ) )
    {
        InvalidateRect( NULL );
        return;
    }

    SetWindowPos( NULL, 0, 0, rect.Width( ), iHt, SWP_NOMOVE | SWP_NOZORDER );

    InvalidateRect( NULL );
}

BOOL CWndArea::OnCommand( WPARAM wParam, LPARAM lParam )
{

    switch ( LOWORD( wParam ) )
    {
    case IDA_SAVE:
        GetParent( )->SendMessage( WM_COMMAND, wParam, lParam );
        return ( TRUE );
    }

    return ( CWndAnim::OnCommand( wParam, lParam ) );
}

void CWndArea::OnDeselect( )
{

    if ( ( m_iMode == rocket_ready ) || ( m_iMode == rocket_pos ) || ( m_iMode == rocket_wait ) )
        return;

    BldgCurOff( );

    m_lstUnits.RemoveAllUnits( TRUE );
    m_pUnit      = NULL;
    m_uFlags     = 0;
    m_uMouseMode = lmb_nothing;

    SetButtonState( );
    InvalidateStatus( );
    InvalidateSound( );
}

void CWndArea::OnCloseWin( )
{

    ShowWindow( SW_HIDE );
}

void CWndArea::BldgCurOff( )
{

    theMap.ClrBldgCur( );
    m_iBuild = -1;
    m_iMode  = normal;
    ::SetCursor( m_hCurReg );
}

// add this unit to the list of selected units
void CWndArea::AddSelectUnit( CUnit* pUnit )
{

    m_lstUnits.AddUnit( pUnit, TRUE );
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;
    MaterialChange( pUnit );
}

// add this unit to the list of selected units
void CWndArea::SubSelectUnit( CUnit* pUnit )
{

    m_lstUnits.RemoveUnit( pUnit );
    if ( m_lstUnits.GetCount( ) == 1 )
        m_pUnit = m_lstUnits.GetHead( );
    else
        m_pUnit = NULL;
    MaterialChange( pUnit );
}

// make this the only selected unit
void CWndArea::OnlySelectUnit( CUnit* pUnit )
{

    m_lstUnits.RemoveAllUnits( TRUE );
    m_lstUnits.AddUnit( pUnit, TRUE );
    m_pUnit = pUnit;
    MaterialChange( pUnit );
}

HBRUSH CWndArea::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
    HBRUSH hbr = CWndAnim::OnCtlColor( pDC, pWnd, nCtlColor );

    // TODO: Change any attributes of the DC here

    // TODO: Return a different brush if the default is not desired
    return hbr;
}

void CWndArea::ClrRoadIcons( )
{

    // reset sprites
    CHexCoord* pHexOn     = m_phexRoadPath;
    CSprite**  ppSpriteOn = m_ppUnderSprite;
    while ( m_iNumRoadHex-- )
    {
        pHexOn->SetInvalidated( );
        theMap._GetHex( *pHexOn++ )->m_psprite = *ppSpriteOn++;
    }

    // free it up
    delete[] m_phexRoadPath;
    delete[] m_ppUnderSprite;
    m_phexRoadPath  = NULL;
    m_ppUnderSprite = NULL;
    m_iNumRoadHex   = 0;
}

void CWndArea::SetRoadIcons( CHexCoord hexEnd )
{

    ClrRoadIcons( );

    // no path or starting on water
    if ( ( m_hexRoadStart == hexEnd ) || ( !theMap._GetHex( m_hexRoadStart )->CanRoad( ) ) )
        return;

    // alloc the space
    delete[] m_phexRoadPath;
    delete[] m_ppUnderSprite;
    m_iNumRoadHex   = 0;
    int x           = abs( CHexCoord::Diff( hexEnd.X( ) - m_hexRoadStart.X( ) ) );
    int y           = abs( CHexCoord::Diff( hexEnd.Y( ) - m_hexRoadStart.Y( ) ) );
    int iSize       = __max( x, y ) + 3 + MAX_SPAN;
    m_phexRoadPath  = new CHexCoord[iSize];
    m_ppUnderSprite = new CSprite*[iSize];
    iSize--;

    CSprite*   pSprRoad = theTerrain.GetSprite( CHex::road, CHex::r_path );
    CHexCoord  _hexOn( m_hexRoadStart );
    CHexCoord* pHexOn     = m_phexRoadPath;
    CSprite**  ppSpriteOn = m_ppUnderSprite;
    BOOL       bOnWater   = FALSE;
    int        iSpan;

    while ( iSize-- > 0 )
    {
        CHex* pHex = theMap._GetHex( _hexOn );

        // hit a bridge - stop
        if ( pHex->GetUnits( ) & CHex::bridge )
            return;

        // check for water
        if ( ( !bOnWater ) && ( !pHex->CanRoad( ) ) )
        {
            // if can't bridge - done
            if ( !theGame.GetMe( )->CanBridge( ) )
                return;

            // ok - over the water
            x = CHexCoord::Diff( hexEnd.X( ) - _hexOn.X( ) );
            y = CHexCoord::Diff( hexEnd.Y( ) - _hexOn.Y( ) );
            if ( abs( x ) >= abs( y ) )
            {
                x = __minmax( -1, 1, x );
                y = 0;
            }
            else
            {
                x = 0;
                y = __minmax( -1, 1, y );
            }
            bOnWater = TRUE;
            iSpan    = 0;
        }

        if ( pHex->m_psprite != pSprRoad )
        {
            *pHexOn++     = _hexOn;
            *ppSpriteOn++ = pHex->m_psprite;
            m_iNumRoadHex++;
            pHex->m_psprite = pSprRoad;
            _hexOn.SetInvalidated( );
        }

        // check out span
        if ( bOnWater )
        {
            if ( pHex->IsWater( ) )
            {
                iSpan++;
                if ( iSpan > MAX_SPAN )
                    return;
            }
            else
            {
                iSpan = 0;
                if ( pHex->CanRoad( ) )
                    bOnWater = FALSE;
            }
        }

        if ( _hexOn == hexEnd )
            break;

        // move closer on the longest one (so we go diaganol)
        if ( !bOnWater )
        {
            x = CHexCoord::Diff( hexEnd.X( ) - _hexOn.X( ) );
            y = CHexCoord::Diff( hexEnd.Y( ) - _hexOn.Y( ) );
        }

        // to the next hex
        if ( abs( x ) >= abs( y ) )
        {
            if ( x > 0 )
                _hexOn.Xinc( );
            else
                _hexOn.Xdec( );
        }
        else
        {
            if ( y > 0 )
                _hexOn.Yinc( );
            else
                _hexOn.Ydec( );
        }
    }
}

void CWndArea::GiveSelectedUnits( CPlayer* pPlr )
{

    // count for message
    int      iBldgs = 0, iVehs = 0;
    POSITION pos = m_lstUnits.GetHeadPosition( );
    while ( pos != NULL )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );

        if ( pUnit->GetUnitType( ) == CUnit::building )
        {
            CBuilding* pBldg = (CBuilding*)pUnit;
            if ( ( !pBldg->IsConstructing( ) ) && ( pBldg->GetData( )->GetType( ) != CStructureData::rocket ) )
            {
                iBldgs++;
                // add any vehicles inside
                POSITION pos = theVehicleMap.GetStartPosition( );
                while ( pos != NULL )
                {
                    DWORD     dwID;
                    CVehicle* pVeh;
                    theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                    ASSERT_STRICT_VALID( pVeh );
                    if ( ( !pVeh->GetHexOwnership( ) ) && ( pVeh->GetOwner( ) == pPlr ) )
                    {
                        int xDif = CHexCoord::Diff( pVeh->GetHexHead( ).X( ) - pBldg->GetHex( ).X( ) );
                        if ( ( 0 <= xDif ) && ( xDif < m_cx ) )
                        {
                            int yDif = CHexCoord::Diff( pVeh->GetHexHead( ).Y( ) - pBldg->GetHex( ).Y( ) );
                            if ( ( 0 <= yDif ) && ( yDif < m_cy ) )
                                iVehs++;
                        }
                    }
                }
            }
        }

        else
            iVehs += 1 + ( (CVehicle*)pUnit )->GetCargoCount( );
    }

    // create prompt
    CString sSure, sNumB, sNumV;
    sSure.LoadString( IDS_GIVE_UNITS );
    sNumB = IntToCString( iBldgs, 10, TRUE );
    sNumV = IntToCString( iVehs, 10, TRUE );
    csPrintf( &sSure, (char const*)sNumB, (char const*)sNumV, (char const*)pPlr->GetName( ) );
    if ( AfxMessageBox( sSure, MB_YESNO | MB_ICONQUESTION ) != IDYES )
    {
        TRAP( );
        return;
    }

    // ok hand them over
    pos = m_lstUnits.GetHeadPosition( );
    while ( pos != NULL )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        if ( pUnit->GetDamagePer( ) <= 50 )
            continue;

        if ( pUnit->GetUnitType( ) == CUnit::building )
        {
            CBuilding* pBldg = (CBuilding*)pUnit;
            if ( ( !pBldg->IsConstructing( ) ) && ( pBldg->GetData( )->GetType( ) != CStructureData::rocket ) )
            {
                CMsgGiveUnit msg( pUnit, pPlr );
                theGame.PostToAll( &msg, sizeof( msg ) );
                pUnit->_SetOwner( pPlr );

                // add any vehicles inside
                POSITION pos = theVehicleMap.GetStartPosition( );
                while ( pos != NULL )
                {
                    DWORD     dwID;
                    CVehicle* pVeh;
                    theVehicleMap.GetNextAssoc( pos, dwID, pVeh );
                    ASSERT_STRICT_VALID( pVeh );
                    if ( ( !pVeh->GetHexOwnership( ) ) && ( pVeh->GetOwner( ) == pPlr ) )
                    {
                        int xDif = CHexCoord::Diff( pVeh->GetHexHead( ).X( ) - pBldg->GetHex( ).X( ) );
                        if ( ( 0 <= xDif ) && ( xDif < m_cx ) )
                        {
                            int yDif = CHexCoord::Diff( pVeh->GetHexHead( ).Y( ) - pBldg->GetHex( ).Y( ) );
                            if ( ( 0 <= yDif ) && ( yDif < m_cy ) )
                            {
                                TRAP( );
                                CMsgGiveUnit msg( pVeh, pPlr );
                                theGame.PostToAll( &msg, sizeof( msg ) );
                                pVeh->_SetOwner( pPlr );
                            }
                        }
                    }
                }
            }
        }

        else
        {
            CMsgGiveUnit msg( pUnit, pPlr );
            theGame.PostToAll( &msg, sizeof( msg ) );
            pUnit->_SetOwner( pPlr );

            CVehicle* pVeh = (CVehicle*)pUnit;
            POSITION  pos  = pVeh->GetCargoHeadPosition( );
            while ( pos != NULL )
            {
                TRAP( );
                CVehicle*    pVehOn = pVeh->GetCargoNext( pos );
                CMsgGiveUnit msg( pVehOn, pPlr );
                theGame.PostToAll( &msg, sizeof( msg ) );
                pVehOn->_SetOwner( pPlr );
            }
        }
    }

    // we have nothing selected
    SelectNone( );
}

int CWndArea::NumGiveable( ) const
{

    int      iCount = 0;
    POSITION pos    = m_lstUnits.GetHeadPosition( );
    while ( pos != NULL )
    {
        CUnit* pUnit = m_lstUnits.GetNext( pos );
        if ( pUnit->GetDamagePer( ) > 50 )
        {
            if ( pUnit->GetUnitType( ) == CUnit::building )
            {
                CBuilding* pBldg = (CBuilding*)pUnit;
                if ( ( !pBldg->IsConstructing( ) ) && ( pBldg->GetData( )->GetType( ) != CStructureData::rocket ) )
                    iCount++;
            }
            else
                iCount += 1 + ( (CVehicle*)pUnit )->GetCargoCount( );
        }
    }

    return ( iCount );
}
